// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "config.hpp"

#include "bybit.hpp"
#include "scheduler.hpp"

#include "nlohmann/json.hpp"

#include <openssl/hmac.h>
#include <boost/lexical_cast.hpp>


#include <chrono>
#include <thread>
#include <sstream>

namespace scratcher::bybit {

namespace {

const char* const REQ_TIME = "/v5/market/time";
const char* const REQ_KLINE = "/v5/market/kline";

std::string generateSignature(const std::string &message, const std::string &secret)
{
    unsigned char* digest = HMAC(EVP_sha256(), secret.c_str(), secret.length(), (unsigned char*)message.c_str(), message.length(), NULL, NULL);

    char mdString[SHA256_DIGEST_LENGTH*2+1];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        sprintf(&mdString[i*2], "%02x", static_cast<unsigned int>(digest[i]));

    return mdString;
}

}

class bybit_error_category_impl : public boost::system::error_category
{
public:
    virtual ~bybit_error_category_impl() = default;

    static bybit_error_category_impl instance;
    const char* name() const noexcept override { return "bybit"; }
    std::string message(int ev) const override { return "ByBit error: " + std::to_string(ev); }
    char const* message(int ev, char* buffer, std::size_t len) const noexcept override
    {
        std::snprintf( buffer, len, "ByBit error: %d", ev);
        return buffer;
    }
};

bybit_error_category_impl bybit_error_category_impl::instance;

boost::system::error_category& bybit_error_category()
{ return bybit_error_category_impl::instance; }

struct ByBitSubscriber
{
    const std::string symbol;
    time start_time;
    seconds tick_seconds;
    const_kline_iterator last_read;
    std::shared_ptr<ByBitDataCache> cache;

    long start_timestamp() const
    { return std::chrono::duration_cast<milliseconds>(start_time.time_since_epoch()).count(); }
    long end_timestamp(uint32_t tick_count) const
    { return std::chrono::duration_cast<milliseconds>((start_time + tick_count * tick_seconds).time_since_epoch()).count(); }
};

struct ByBitDataCache
{
    const std::string symbol;
    mutable std::shared_mutex mutex;
    time first_tick_time;
    seconds tick_duration;
    kline_sequence_type klines;

    void FillKlines(kline_sequence_type& out, time start_time, seconds tick_duration, size_t tick_count);
};

namespace ip = boost::asio::ip;
namespace websock = boost::beast::websocket;


ByBitApi::ByBitApi(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler)
    : m_host(config->Host()), m_port(config->Port())
    , mScheduler(std::move(scheduler))
{
}

std::shared_ptr<ByBitApi> ByBitApi::Create(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler)
{
    auto self = std::make_shared<ByBitApi>(config, scheduler);
    self->Spawn([=](yield_context yield) { self->DoResolve(yield); });
    self->Spawn([=](yield_context yield) { self->DoPing(yield); });

    return self;
}


void ByBitApi::Spawn(std::function<void(yield_context yield)> task)
{
    auto self_ref = weak_from_this();
    spawn(mScheduler->io(),
          [=](yield_context yield) {
              boost::system::error_code status;
              task(yield[status]);
              if (status) {
                  std::cerr << status.message() << std::endl;
                  status.clear();

                  if (auto self = self_ref.lock()) {

                      boost::asio::steady_timer t(self->mScheduler->io(), milliseconds(500));
                      t.async_wait(yield[status]);

                      if (status) {
                          std::cerr << "Repeat timer error: " << status.message() << std::endl;
                          throw status;
                      }

                      if (auto self1 = self_ref.lock())
                          self1->Spawn(task);
                  }
              }
          },
          [](std::exception_ptr ex) { if (ex) std::rethrow_exception(ex); });
}

void ByBitApi::RequestServer(std::string_view request_string, std::function<void(const nlohmann::json&)> proc_body, yield_context &yield)
{
    if (m_resolved_host.empty()) {
        *yield.ec_ = xscratcher_error_code(error::no_host_name);
        return;
    }

    boost::beast::ssl_stream<boost::beast::tcp_stream> sock(mScheduler->io(), mScheduler->ssl());

    auto start_time = std::chrono::utc_clock::now();

    get_lowest_layer(sock).async_connect(m_resolved_host, yield);
    if (*yield.ec_) {
        std::cerr << "connect error: ";
        return;
    }

    if (!SSL_set_tlsext_host_name(sock.native_handle(), m_host.c_str()))
        throw std::ios_base::failure( "Failed to set SNI Hostname");

    sock.async_handshake(ssl::stream_base::client, yield);
    if (*yield.ec_) {
        std::cerr << "handshake error: ";
        return;
    }

    boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, request_string, 11);
    req.set(boost::beast::http::field::host, m_host);
    req.prepare_payload();

    boost::beast::http::async_write(sock, req, yield);
    if (*yield.ec_) {
        std::cerr << "request error: ";
        return;
    }

    boost::beast::flat_buffer buf;
    boost::beast::http::response<boost::beast::http::string_body> resp;
    boost::beast::http::async_read(sock, buf, resp, yield);
    if (*yield.ec_) {
        std::cerr << "read error: ";
        return;
    }

    if (resp.result() == boost::beast::http::status::ok) {
        std::clog << "resp body: " << resp.body() << std::endl;
        auto resp_json = nlohmann::json::parse(resp.body().begin(), resp.body().end());

        if (resp_json["retCode"] == 0) {
            auto end_time = std::chrono::utc_clock::now();
            std::chrono::utc_clock::time_point server_time = std::chrono::time_point<std::chrono::utc_clock>(milliseconds(resp_json["time"].get<long>()));
            CalcServerTime(server_time, start_time, end_time);

            proc_body(resp_json);
        }
        else {
            std::cerr << "bybit returned error: " << resp_json["retMsg"] << std::endl;
            *yield.ec_ = boost::system::error_code(resp_json["retCode"].get<int>(), bybit_error_category());
        }
    }
    else {
        std::cerr << "http returned error: " << resp.reason() << std::endl;
        *yield.ec_ = boost::system::error_code(resp.result_int(), bybit_error_category());
    }
}

void ByBitApi::DoResolve(yield_context &yield)
{
    std::clog << "Trying to resolve" << std::endl;

    ip::tcp::resolver resolver(mScheduler->io());
    m_resolved_host = resolver.async_resolve(m_host, m_port, yield);
    if (*yield.ec_) {
        std::cerr << "host resolutoion error";
    }
    std::clog << "Host resolved" << std::endl;
}

void ByBitApi::DoPing(yield_context &yield)
{
    std::clog << "Trying to connect" << std::endl;
    RequestServer(REQ_TIME, [](const nlohmann::json& ) {/*do nothing*/}, yield);
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

void ByBitApi::DoSubscribe(std::shared_ptr<ByBitSubscriber> subscriber, std::optional<uint32_t> tick_count, yield_context &yield)
{
    std::clog << "Trying to subscribe" << std::endl;

    if (!m_server_time_delta) {
        *yield.ec_ = xscratcher_error_code(error::no_time_sync);
        return;
    }

    long end = tick_count ? subscriber->end_timestamp(*tick_count) : std::chrono::duration_cast<milliseconds>((std::chrono::utc_clock::now() + *m_server_time_delta + m_request_halftrip).time_since_epoch()).count();

    std::clog << "start: " << subscriber->start_time << "\nend:   " << end << "\ninterval: " << subscriber->tick_seconds.count() << std::endl;

    boost::beast::ssl_stream<boost::beast::tcp_stream> sock{ mScheduler->io(), mScheduler->ssl() };

    //get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    get_lowest_layer(sock).async_connect(m_resolved_host, yield);
    if (*yield.ec_) {
        std::cerr << "connect error: ";
        return;
    }

    if (!SSL_set_tlsext_host_name(sock.native_handle(), m_host.c_str()))
        throw std::ios_base::failure( "Failed to set SNI Hostname");

    //get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    sock.async_handshake(ssl::stream_base::client, yield);
    if (*yield.ec_) {
        std::cerr << "handshake error: ";
        return;
    }

    //get_lowest_layer(sock).expires_never();

    // Set a decorator to change the User-Agent of the handshake
    // sock.set_option(websock::stream_base::decorator(
    //     [](websock::request_type& req) {
    //         req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
    //     }));

    std::ostringstream target;
    target << REQ_KLINE << '?' << "category=spot&symbol=" << subscriber->symbol <<"&interval=" << std::chrono::duration_cast<std::chrono::minutes>(subscriber->tick_seconds).count() << "&start=" << subscriber->start_timestamp() << "&end=" << end;
    std::string target_str = target.str();

    std::clog << "req params: " << target_str << std::endl;

    boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, target_str, 11);
    req.set(boost::beast::http::field::host, m_host);
    req.prepare_payload();

    std::clog << "request: " << req << std::endl;
    boost::beast::http::async_write(sock, req, yield);
    if (*yield.ec_) {
        std::cerr << "request error: ";
        return;
    }

    boost::beast::flat_buffer buf;
    boost::beast::http::response<boost::beast::http::string_body> resp;
    boost::beast::http::read(sock, buf, resp);

    std::clog << "response: " << resp.body() << std::endl;

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

    //std::thread([this]{ io_ctx.run(); });
}

ByBitApi::subscriber_ref ByBitApi::Subscribe(const subscriber_ref &s, const std::string& symbol, time from, seconds tick_seconds, std::optional<uint32_t> tick_count)
{
    auto subscriber = s.lock();
    if (!subscriber) {
        subscriber = std::make_shared<ByBitSubscriber>(symbol);
        m_subscribers.emplace_back(subscriber);
    }
    else {
        if (subscriber->symbol != symbol) throw SchedulerParamMismatch("symbol");
    }
    if (!subscriber->cache) {
        std::unique_lock lock(m_data_cache_mutex);
        auto cache_it = std::find_if(m_data_cache.begin(), m_data_cache.end(),
                                     [&](const auto& c) {
                                         if (auto cache = c.lock())
                                             return cache->symbol == symbol;
                                         return false;
                                     });
        if (cache_it == m_data_cache.end()) {
            subscriber->cache = std::make_shared<ByBitDataCache>(symbol);
            m_data_cache.emplace_back(subscriber->cache);
        }
    }

    subscriber->start_time = from + *m_server_time_delta;
    subscriber->tick_seconds = tick_seconds;

    auto self = shared_from_this();
    Spawn([=](yield_context yield) { self->DoSubscribe(std::move(subscriber), std::move(tick_count), yield); });

    return subscriber;
}

void ByBitApi::Unsubscribe(subscriber_ref s)
{
    if (s.expired()) return;
    if (auto subscriber = s.lock()) {
        auto s_it = std::find(m_subscribers.begin(), m_subscribers.end(), subscriber);
        if (s_it != m_subscribers.end())
            m_subscribers.erase(s_it);
    }
    {
        std::unique_lock lock(m_data_cache_mutex);
        for (auto it = m_data_cache.begin(); it != m_data_cache.end(); it->expired() ? it = m_data_cache.erase(it) : ++it) ; //do nothing
    }
}

}

#include <iostream>
