// Scratcher project
// Copyright (c) 2024-2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "config.hpp"

#include "bybit.hpp"
#include "scheduler.hpp"

#include "nlohmann/json.hpp"

#include <openssl/hmac.h>
#include <boost/lexical_cast.hpp>


#include <chrono>
#include <sstream>

#include "bybit/error.hpp"
#include "bybit/stream.hpp"
#include "bybit/subscription.hpp"
#include "bybit/data_manager.hpp"

namespace scratcher::bybit {

namespace ip = boost::asio::ip;

namespace {

const char* const REQ_TIME = "/v5/market/time";
const char* const REQ_INSTRUMENT = "/v5/market/instruments-info";
const char* const REQ_KLINE = "/v5/market/kline";

const char* const STREAM_PUBLIC_SPOT = "/v5/public/spot";

std::string generateSignature(const std::string &message, const std::string &secret)
{
    unsigned char* digest = HMAC(EVP_sha256(), secret.c_str(), secret.length(), (unsigned char*)message.c_str(), message.length(), NULL, NULL);

    char mdString[SHA256_DIGEST_LENGTH*2+1];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        sprintf(&mdString[i*2], "%02x", static_cast<unsigned int>(digest[i]));

    return mdString;
}

}

bybit_error_category_impl bybit_error_category_impl::instance;

//----------------------------------------------------------------------------------------------------------------------

ByBitApi::ByBitApi(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler, EnsurePrivate)
    : mConfig(move(config))
    , mScheduler(std::move(scheduler))
    , m_data_queue(10)
    , m_data_queue_strand(make_strand(mScheduler->io()))
{
}

std::shared_ptr<ByBitApi> ByBitApi::Create(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler)
{
    auto self = std::make_shared<ByBitApi>(config, scheduler, EnsurePrivate{});

    self->Resolve();

    return self;
}


awaitable<ip::tcp::resolver::results_type> ByBitApi::coResolve(boost::asio::ip::tcp::resolver resolver, std::string host, std::string port)
{
    std::clog << "Calling async_resolve ..." << std::endl;
    co_return co_await resolver.async_resolve(host, port, boost::asio::use_awaitable);
}

awaitable<nlohmann::json> ByBitApi::coRequestServer(std::shared_ptr<ByBitApi> self, std::string_view request_string)
{
    while (!self->m_is_http_resolved) {
        std::clog << "Waiting for resolve..." << std::endl;
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor, milliseconds(250));
        co_await t.async_wait(boost::asio::use_awaitable);
    }

    if (self->m_resolved_http_host.empty()) throw xscratcher_error_code(error::no_host_name);

    boost::beast::ssl_stream<boost::beast::tcp_stream> sock(co_await boost::asio::this_coro::executor, self->mScheduler->ssl());

    auto start_time = std::chrono::utc_clock::now();

    co_await get_lowest_layer(sock).async_connect(self->m_resolved_http_host, boost::asio::use_awaitable);

    if (!SSL_set_tlsext_host_name(sock.native_handle(), self->mConfig->HttpHost().c_str()))
        throw std::ios_base::failure( "Failed to set SNI Hostname");

    co_await sock.async_handshake(ssl::stream_base::client, boost::asio::use_awaitable);

    boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, request_string, 11);
    req.set(boost::beast::http::field::host, self->mConfig->HttpHost());
    req.prepare_payload();

    co_await boost::beast::http::async_write(sock, req, boost::asio::use_awaitable);

    boost::beast::flat_buffer buf;
    boost::beast::http::response<boost::beast::http::string_body> resp;
    co_await boost::beast::http::async_read(sock, buf, resp, boost::asio::use_awaitable);

    if (resp.result() == boost::beast::http::status::ok) {
        std::clog << "resp body: " << resp.body() << std::endl;
        auto resp_json = nlohmann::json::parse(resp.body().begin(), resp.body().end());

        if (resp_json["retCode"] == 0) {
            auto end_time = std::chrono::utc_clock::now();
            std::chrono::utc_clock::time_point server_time = std::chrono::time_point<std::chrono::utc_clock>(milliseconds(resp_json["time"].get<long>()));
            self->CalcServerTime(server_time, start_time, end_time);

            co_return resp_json;
        }
        else {
            std::cerr << "bybit returned error: " << resp_json["retMsg"] << std::endl;
            throw boost::system::error_code(resp_json["retCode"].get<int>(), bybit_error_category());
        }
    }
    else {
        std::cerr << "http returned error: " << resp.reason() << std::endl;
        throw boost::system::error_code(resp.result_int(), bybit_error_category());
    }
}

void ByBitApi::Resolve()
{
    std::clog << "Trying to resolve" << std::endl;

    co_spawn(mScheduler->io(), coResolve(ip::tcp::resolver(mScheduler->io()), mConfig->HttpHost(), mConfig->HttpPort()),
        [self = shared_from_this()](std::exception_ptr e, ip::tcp::resolver::results_type names) {
            if (e) {
                std::cerr << "Resolve error!!!" << std::endl;
            }
            self->m_resolved_http_host = move(names);
            self->m_is_http_resolved = true;
            std::clog << "HTTP Resolved!!!" << std::endl;
        });

    co_spawn(mScheduler->io(), coResolve(ip::tcp::resolver(mScheduler->io()), mConfig->StreamHost(), mConfig->StreamPort()),
        [self = shared_from_this()](std::exception_ptr e, ip::tcp::resolver::results_type names) {
            if (e) {
                std::cerr << "Resolve error!!!" << std::endl;
            }
            self->m_resolved_websock_host = move(names);
            self->m_is_websock_resolved = true;
            std::clog << "WebSock Resolved!!!" << std::endl;
        });
}

awaitable<void> ByBitApi::DoPing(std::shared_ptr<ByBitApi> self)
{
    std::clog << "Trying to connect: " << REQ_TIME << std::endl;
    co_await coRequestServer(move(self), REQ_TIME);
}

awaitable<nlohmann::json> ByBitApi::coGetInstrumentInfo(std::shared_ptr<ByBitApi> self, std::shared_ptr<ByBitSubscription> subscription)
{
    std::clog << "Creating Instrument request..." << std::endl;
    std::ostringstream buf;
    buf << REQ_INSTRUMENT << "?category=spot&symbol=" << subscription->symbol;
    std::clog << "Requesting instrument..." << std::endl;
    co_return co_await coRequestServer(move(self), buf.str());
}


void ByBitApi::SubscribePublicStream(const std::shared_ptr<ByBitSubscription>& subscription)
{
    if (m_public_spot_stream) {
        if (m_public_spot_stream->Status() == ByBitStream::status::STALE)
            throw std::runtime_error("Stale public stream");

    }
    else {
        auto ref = weak_from_this();

        m_public_spot_stream = ByBitStream::Create(shared_from_this(), STREAM_PUBLIC_SPOT,
            [ref](std::string&& data) { HandleConnectionData(ref, move(data)); },
            [ref](boost::system::error_code ec) { HandleConnectionError(ref, ec); });

    }
    m_public_spot_stream->SubscribeTopics(
        std::array {
            SubscriptionTopic{"publicTrade", subscription->symbol},
            //SubscriptionTopic{"orderbook", 50, subscription->symbol},
        }
    );
}

void ByBitApi::HandleConnectionData(std::weak_ptr<ByBitApi> ref, std::string&& data)
{
    if (auto self = ref.lock()) {
        self->m_data_queue.push(move(data));

        post(self->m_data_queue_strand, [ref]() {
            if (auto self = ref.lock()) {

                /*self->m_data_queue.consume_all([ref](std::string data)*/
                while (!self->m_data_queue.empty()) {
                    auto payload = nlohmann::json::parse(self->m_data_queue.front());

                    if (payload.find("op") != payload.end()) {
                        self->m_data_queue.pop();
                        if (payload["success"]) {
                            std::clog << payload["op"] << '/' <<payload["req_id"] << ": " << payload["success"] << std::endl;
                        }
                        else {
                            std::cerr << payload["op"] << '/' <<payload["req_id"] << ": " << payload["success"] << std::endl;
                        }
                    }
                    else if (payload.find("topic") != payload.end()) {
                        //if (auto self = ref.lock()) {
                            auto topic = SubscriptionTopic::Parse(payload["topic"]);
                            if (topic.Symbol()) {
                                auto subscript_it = self->m_subscriptions.find(std::string(*topic.Symbol()));
                                if (subscript_it != self->m_subscriptions.end()) {
                                    std::weak_ptr s = subscript_it->second;
                                    if (auto subscription = s.lock()) {
                                        if (subscription->IsReady()) {
                                            subscription->Handle(topic, payload["type"].get<std::string>(), payload["data"]);
                                            self->m_data_queue.pop();
                                        }
                                        else {
                                            break;
                                        }
                                    }
                                    else {
                                        std::cerr << "Unhandled server data: " << self->m_data_queue.front() << std::endl;
                                        self->m_data_queue.pop();
                                    }
                                }
                                else {
                                    std::cerr << "Unhandled server data: " << self->m_data_queue.front() << std::endl;
                                    self->m_data_queue.pop();
                                }
                            }
                            else {
                                std::cerr << "Unhandled server data: " << self->m_data_queue.front() << std::endl;
                                self->m_data_queue.pop();
                            }
                        // }
                        // else {
                        //     break;
                        // }
                    }
                    else {
                        std::cerr << "Unhandled server data: " << self->m_data_queue.front() << std::endl;
                        self->m_data_queue.pop();
                    }
                }
            }
        });
    }
}

void ByBitApi::HandleConnectionError(std::weak_ptr<ByBitApi> ref, boost::system::error_code ec)
{
    if (auto self = ref.lock()) {
        post(self->Scheduler()->io(), [ref, ec] {
            if (auto self = ref.lock()) {
                for (auto& s: self->m_subscriptions) {
                    s.second->HandleError(ec);
                }
            }
        });
    }
}


void ByBitApi::CalcServerTime(time server_time, time request_time, time response_time)
{
    m_request_halftrip = std::chrono::duration_cast<milliseconds>(response_time - request_time) / 2;
    m_server_time_delta = std::chrono::duration_cast<milliseconds>(server_time - request_time + m_request_halftrip);

    std::clog << "now (ms):          " << std::chrono::duration_cast<milliseconds>(time::clock::now().time_since_epoch()).count() << std::endl;
    std::clog << "request time (ms): " << std::chrono::duration_cast<milliseconds>(request_time.time_since_epoch()).count() << std::endl;
    std::clog << "server time (ms):  " << std::chrono::duration_cast<milliseconds>(server_time.time_since_epoch()).count() << std::endl;
    std::clog << "req halftrip (ms): " << m_request_halftrip.count() << std::endl;
    std::clog << "server delta (ms): " << m_server_time_delta->count() << std::endl;
}


std::shared_ptr<ByBitSubscription> ByBitApi::Subscribe(const std::string& symbol, std::shared_ptr<ByBitDataManager> dataManager)
{
    std::shared_ptr<ByBitSubscription> subscription;

    {
        std::unique_lock lock(m_subscriptions_mutex);
        auto subscription_it = m_subscriptions.find(symbol);

        if (subscription_it != m_subscriptions.end())
            subscription = subscription_it->second;

        if (!subscription) {
            subscription = std::make_shared<ByBitSubscription>(symbol, dataManager);
            m_subscriptions.emplace(symbol, subscription);
        }
        else {
            if (subscription->symbol != symbol) throw SchedulerParamMismatch("symbol");
        }
    }

    co_spawn(mScheduler->io(), coGetInstrumentInfo(shared_from_this(), subscription),
        [self = shared_from_this(), subscription](std::exception_ptr e, nlohmann::json resp) {
            if (e) {
                std::cerr << "Instrument error!!!" << std::endl;
            }
            if (resp["result"].is_object()) {
                subscription->dataManager->HandleInstrumentData(resp["result"]);
                std::clog << "Instrument data received" << std::endl;
            }
            else std::cerr << "InstrumentsInfo response contains no \"result\" section" << std::endl;
        });

    SubscribePublicStream(subscription);

    return subscription;
}

void ByBitApi::Unsubscribe(const std::string& symbol)
{
    std::unique_lock lock(m_subscriptions_mutex);

    if (auto subscription_it = m_subscriptions.find(symbol); subscription_it != m_subscriptions.end()) {
        m_subscriptions.erase(subscription_it);
        if (m_public_spot_stream->m_status == ByBitStream::status::STALE) {
            m_public_spot_stream.reset();
        }
        else {
            m_public_spot_stream->UnsubscribeTopics(std::array {
                SubscriptionTopic{"publicTrade", symbol},
                SubscriptionTopic{"orderbook", 50, symbol},
            });
        }
    }

    if (m_subscriptions.empty()) {
        m_public_spot_stream.reset();
    }
}


}
