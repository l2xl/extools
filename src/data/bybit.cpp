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
    std::weak_ptr ref{self};
    self->Resolve();

    return self;
}


void ByBitApi::Spawn(std::function<void(yield_context yield)> task)
{
    spawn(mScheduler->io(),
        [ref = weak_from_this(), task](yield_context yield) {
            boost::system::error_code status;
            while (true) {
                try {
                    task(yield[status]);
                    if (status)
                        throw status;

                    return;
                }
                catch (boost::system::error_code &e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
                catch (std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }

                if (auto self = ref.lock()) {
                    boost::system::error_code timer_error;
                    boost::asio::steady_timer t(yield.get_executor(), milliseconds(500));
                    t.async_wait(yield[timer_error]);

                    if (timer_error) {
                        std::cerr << "Repeat timer error: " << timer_error.message() << std::endl;
                        throw status;
                    }
                }
            }
        },
        [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });
}

nlohmann::json ByBitApi::DoRequestServer(std::string_view request_string, yield_context &yield)
{
    if (m_resolved_http_host.empty()) throw xscratcher_error_code(error::no_host_name);

    boost::system::error_code error;
    boost::beast::ssl_stream<boost::beast::tcp_stream> sock(mScheduler->io(), mScheduler->ssl());

    auto start_time = std::chrono::utc_clock::now();

    get_lowest_layer(sock).async_connect(m_resolved_http_host, yield[error]);
    if (error) throw error;

    if (!SSL_set_tlsext_host_name(sock.native_handle(), mConfig->HttpHost().c_str()))
        throw std::ios_base::failure( "Failed to set SNI Hostname");

    sock.async_handshake(ssl::stream_base::client, yield[error]);
    if (error) throw error;

    boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, request_string, 11);
    req.set(boost::beast::http::field::host, mConfig->HttpHost());
    req.prepare_payload();

    boost::beast::http::async_write(sock, req, yield[error]);
    if (error) throw error;

    boost::beast::flat_buffer buf;
    boost::beast::http::response<boost::beast::http::string_body> resp;
    boost::beast::http::async_read(sock, buf, resp, yield[error]);
    if (error) throw error;

    if (resp.result() == boost::beast::http::status::ok) {
        std::clog << "resp body: " << resp.body() << std::endl;
        auto resp_json = nlohmann::json::parse(resp.body().begin(), resp.body().end());

        if (resp_json["retCode"] == 0) {
            auto end_time = std::chrono::utc_clock::now();
            std::chrono::utc_clock::time_point server_time = std::chrono::time_point<std::chrono::utc_clock>(milliseconds(resp_json["time"].get<long>()));
            CalcServerTime(server_time, start_time, end_time);

            return resp_json;
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
    std::weak_ptr<ByBitApi> self_ref = weak_from_this();

    std::clog << "Trying to resolve" << std::endl;

    Spawn([self_ref](yield_context yield) {
        if (auto self = self_ref.lock()) {
            ip::tcp::resolver resolver(yield.get_executor()/*self->mScheduler->io()*/);
            self->m_resolved_http_host = resolver.async_resolve(self->mConfig->HttpHost(), self->mConfig->HttpPort(), yield);
        }
    });

    Spawn([self_ref](yield_context yield) {
        if (auto self = self_ref.lock()) {
            ip::tcp::resolver resolver(yield.get_executor());
            self->m_resolved_websock_host = resolver.async_resolve(self->mConfig->StreamHost(), self->mConfig->StreamPort(), yield);
        }
    });
}

void ByBitApi::DoPing(yield_context &yield)
{
    std::clog << "Trying to connect: " << REQ_TIME << std::endl;
    DoRequestServer(REQ_TIME, yield);
}

void ByBitApi::DoGetInstrumentInfo(const std::shared_ptr<ByBitSubscription> &subscription, yield_context &yield)
{
    std::ostringstream buf;
    buf << REQ_INSTRUMENT << "?category=spot&symbol=" << subscription->symbol;
    auto resp = DoRequestServer(buf.str(), yield);

    if (resp["result"].is_object()) {
        subscription->dataManager->HandleInstrumentData(resp["result"]);
    }
    else throw WrongServerData("InstrumentsInfo response contains no \"result\" section");
}

void ByBitApi::SpawnStream(std::shared_ptr<ByBitStream> stream, const std::string& symbol)
{
    stream->Spawn();
    stream->Message(stream->SubscribeMessage(
        std::array {
            SubscriptionTopic{"publicTrade", symbol},
            SubscriptionTopic{"orderbook", 50, symbol},
        }, true));
}

void ByBitApi::SubscribePublicStream(const std::shared_ptr<ByBitSubscription>& subscription)
{
    if (m_public_spot_stream) {
        if (m_public_spot_stream->Status() == ByBitStream::status::STALE)
            throw std::runtime_error("Stale public stream");

        m_public_spot_stream->SubscribeTopics(
            std::array {
                SubscriptionTopic{"publicTrade", subscription->symbol},
                SubscriptionTopic{"orderbook", 50, subscription->symbol},
            });
    }
    else {
        std::weak_ptr<ByBitApi> ref = weak_from_this();
        m_public_spot_stream = std::make_shared<ByBitStream>(shared_from_this(), STREAM_PUBLIC_SPOT,
            [ref](std::string&& data) { HandleConnectionData(ref, move(data)); },
            [ref](boost::system::error_code ec) { HandleConnectionError(ref, ec); });

        SpawnStream(m_public_spot_stream, subscription->symbol);
    }
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
                        if (auto self = ref.lock()) {
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
                                    }
                                }
                                else {
                                    std::cerr << "Unhandled server data: " << self->m_data_queue.front() << std::endl;
                                    self->m_data_queue.pop();
                                }
                            }
                        }
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

// void ByBitApi::DoHttpRequest(std::shared_ptr<ByBitSubscription> subscriber, std::optional<uint32_t> tick_count, yield_context &yield)
// {
    // if (!m_server_time_delta) {
    //     *yield.ec_ = xscratcher_error_code(error::no_time_sync);
    //     return;
    // }
    //
    // long end = tick_count ? subscriber->end_timestamp(*tick_count) : std::chrono::duration_cast<milliseconds>((std::chrono::utc_clock::now() + *m_server_time_delta + m_request_halftrip).time_since_epoch()).count();
    //
    // std::clog << "start: " << subscriber->start_time << "\nend:   " << end << "\ninterval: " << subscriber->tick_seconds.count() << std::endl;
    //
    // boost::beast::ssl_stream<boost::beast::tcp_stream> sock{ mScheduler->io(), mScheduler->ssl() };
    //
    // //get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    // get_lowest_layer(sock).async_connect(m_resolved_http_host, yield);
    // if (*yield.ec_) {
    //     std::cerr << "connect error: ";
    //     return;
    // }
    //
    // if (!SSL_set_tlsext_host_name(sock.native_handle(), mConfig->HttpHost().c_str()))
    //     throw std::ios_base::failure( "Failed to set SNI Hostname");
    //
    // //get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    // sock.async_handshake(ssl::stream_base::client, yield);
    // if (*yield.ec_) {
    //     std::cerr << "handshake error: ";
    //     return;
    // }
    //
    // //get_lowest_layer(sock).expires_never();
    //
    // // Set a decorator to change the User-Agent of the handshake
    // // sock.set_option(websock::stream_base::decorator(
    // //     [](websock::request_type& req) {
    // //         req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
    // //     }));
    //
    // std::ostringstream target;
    // target << REQ_KLINE << '?' << "category=spot&symbol=" << subscriber->symbol <<"&interval=" << std::chrono::duration_cast<std::chrono::minutes>(subscriber->tick_seconds).count() << "&start=" << subscriber->start_timestamp() << "&end=" << end;
    // std::string target_str = target.str();
    //
    // std::clog << "req params: " << target_str << std::endl;
    //
    // boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, target_str, 11);
    // req.set(boost::beast::http::field::host, mConfig->HttpHost());
    // req.prepare_payload();
    //
    // std::clog << "request: " << req << std::endl;
    // boost::beast::http::async_write(sock, req, yield);
    // if (*yield.ec_) {
    //     std::cerr << "request error: ";
    //     return;
    // }
    //
    // boost::beast::flat_buffer buf;
    // boost::beast::http::response<boost::beast::http::string_body> resp;
    // boost::beast::http::read(sock, buf, resp);
    //
    // std::clog << "response: " << resp.body() << std::endl;

    // boost::asio::post([=, this] {
    //
    //     time_t start = subscriber->start_time;
    //     time_t end = tick_count ? (start + *tick_count * std::chrono::milliseconds(subscriber->tick_seconds).count()) : std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    //     time_t mins = subscriber->tick_seconds.count();
    //
    //     std::clog << "start: " << start << "\nend:   " << end << "\ninterval: " << mins << std::endl;
    //
    //     websock::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> sock{ io_ctx, ssl_ctx };
    //
    //     get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    //     get_lowest_layer(sock).connect(m_resolved_host);
    //
    //     if (!SSL_set_tlsext_host_name(sock.next_layer().native_handle(), m_host.c_str()))
    //         throw std::ios_base::failure( "Failed to set SNI Hostname");
    //
    //     get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    //     sock.next_layer().handshake(ssl::stream_base::client);
    //
    //     get_lowest_layer(sock).expires_never();
    //
    //     // Set suggested timeout settings for the websocket
    //     sock.set_option(websock::stream_base::timeout::suggested(boost::beast::role_type::client));
    //
    //     // Set a decorator to change the User-Agent of the handshake
    //     sock.set_option(websock::stream_base::decorator(
    //         [](websock::request_type& req) {
    //             req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
    //         }));
    //
    //     // Perform the websocket handshake
    //     sock.handshake(m_host + ":" + m_port, REQ_KLINE/*"/"*/);
    //     // sock.write(boost::beast::net::buffer(R"({"category":"spot", "symbol":"BTCUSDT", "interval":"1", "start":"", "end": ""})"));
    //     //
    //
    // });

//}


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

    Spawn([subscription, ref=weak_from_this()](yield_context yield) {
        if (auto self = ref.lock()) {
            self->DoGetInstrumentInfo(subscription, yield);
            self->SubscribePublicStream(subscription);
        }
    });

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
