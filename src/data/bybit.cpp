// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "config.hpp"

#include "bybit.hpp"
#include "scheduler.hpp"

#include "nlohmann/json.hpp"

#include <openssl/hmac.h>
#include <boost/beast.hpp>
#include <boost/lexical_cast.hpp>


#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>

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
    std::string message(int ev) const override { return "error: " + std::to_string(ev); }
    char const* message(int ev, char* buffer, std::size_t len) const noexcept override
    {
        std::snprintf( buffer, len, "error: %d", ev);
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

    //std::cout << "response: " << resp << std::endl;

    // m_sock.handshake(m_host, REQ_TIME);
    // if (!m_sock.is_open()) throw std::ios_base::failure("ByBit connection error");
    //
    //sock.read(m_sock_buf);
    //
    // std::clog << boost::beast::buffers_to_string(m_sock_buf.data()) << std::endl;
    //
    // m_sock.close(websock::close_code::none);
    // m_sock_buf.clear();
}

std::shared_ptr<ByBitApi> ByBitApi::Create(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler)
{
    auto ptr = std::make_shared<ByBitApi>(config, scheduler);

    scheduler->SpawnSSL([=](io_context& io, ssl::context& ssl, yield_context yield) {
        ptr->DoResolveAndConnect(io, ssl, yield);
    });

    return ptr;
}

void ByBitApi::DoResolveAndConnect(io_context &io, ssl::context &ssl, yield_context yield)
{
    ip::tcp::resolver resolver(io);
    m_resolved_host = resolver.async_resolve(m_host, m_port, yield[m_status]);
    if (m_status) {
        std::cerr << m_status.message() << std::endl;
        //TODO: repeat name resolution
    }
    else {
        DoPing(io, ssl, yield);
    }
}

void ByBitApi::DoPing(io_context& io, ssl::context& ssl, yield_context yield)
{
    boost::beast::ssl_stream<boost::beast::tcp_stream> sock(io, ssl);

    get_lowest_layer(sock).async_connect(m_resolved_host, yield[m_status]);
    if (m_status) {
        std::cerr << "connect error: " <<m_status.message() << std::endl;
        return;
    }

    if (!SSL_set_tlsext_host_name(sock.native_handle(), m_host.c_str()))
        throw std::ios_base::failure( "Failed to set SNI Hostname");

    sock.async_handshake(ssl::stream_base::client, yield[m_status]);
    if (m_status) {
        std::cerr << "handshake error: " <<m_status.message() << std::endl;
        return;
    }

    boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, REQ_TIME, 11);
    req.set(boost::beast::http::field::host, m_host);
    req.prepare_payload();

    auto start_time = std::chrono::utc_clock::now();

    boost::beast::http::async_write(sock, req, yield[m_status]);
    if (m_status) {
        std::cerr << "request error: " <<m_status.message() << std::endl;
        return;
    }

    boost::beast::flat_buffer buf;
    boost::beast::http::response<boost::beast::http::string_body> resp;
    boost::beast::http::async_read(sock, buf, resp, yield[m_status]);
    if (m_status) {
        std::cerr << "read error: " <<m_status.message() << std::endl;
        return;
    }

    auto end_time = std::chrono::utc_clock::now();
    m_request_halftrip = std::chrono::duration_cast<milliseconds>(end_time - start_time) / 2;

    if (resp.result() == boost::beast::http::status::ok) {
        std::clog << "resp body: " << resp.body() << std::endl;
        auto resp_json = nlohmann::json::parse(resp.body().begin(), resp.body().end());
        if (resp_json["retCode"] == 0) {
            std::chrono::utc_clock::time_point server_time = std::chrono::time_point<std::chrono::utc_clock>(milliseconds(resp_json["time"].get<long>()));
            m_server_time_delta = std::chrono::duration_cast<milliseconds>(server_time - start_time + m_request_halftrip);

            std::clog << "server time: " << std::chrono::duration_cast<milliseconds>(server_time.time_since_epoch()).count() << std::endl;
            std::clog << "local time: " << std::chrono::duration_cast<milliseconds>((start_time + m_request_halftrip).time_since_epoch()).count() << std::endl;
            std::clog << "time delta: " << m_server_time_delta.count() << std::endl;
        }
        else {
            std::cerr << "bybit returned error: " << resp_json["retMsg"] << std::endl;
            m_status.assign(resp_json["retCode"].get<int>(), bybit_error_category());
        }
    }
    else {
        std::cerr << "http returned error: " << resp.reason() << std::endl;
        m_status = boost::system::error_code(resp.result_int(), bybit_error_category());
    }
}

void ByBitApi::DoSubscribe(std::shared_ptr<ByBitSubscriber> subscriber, std::optional<uint32_t> tick_count,
    io_context& io, ssl::context& ssl, yield_context yield)
{
    long end = tick_count ? subscriber->end_timestamp(*tick_count) : std::chrono::duration_cast<milliseconds>((std::chrono::utc_clock::now() + m_server_time_delta + m_request_halftrip).time_since_epoch()).count();

    std::clog << "start: " << subscriber->start_time << "\nend:   " << end << "\ninterval: " << subscriber->tick_seconds.count() << std::endl;

    boost::beast::ssl_stream<boost::beast::tcp_stream> sock{ io, ssl };

        //get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    get_lowest_layer(sock).async_connect(m_resolved_host, yield[m_status]);
    if (m_status) {
        std::cerr << "connect error: " <<m_status.message() << std::endl;
        return;
    }

    if (!SSL_set_tlsext_host_name(sock.native_handle(), m_host.c_str()))
        throw std::ios_base::failure( "Failed to set SNI Hostname");

    //get_lowest_layer(sock).expires_after(std::chrono::seconds(10));
    sock.async_handshake(ssl::stream_base::client, yield[m_status]);
    if (m_status) {
        std::cerr << "handshake error: " <<m_status.message() << std::endl;
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
    boost::beast::http::async_write(sock, req, yield[m_status]);
    if (m_status) {
        std::cerr << "request error: " <<m_status.message() << std::endl;
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

ByBitApi::subscriber_ref ByBitApi::Subscribe(subscriber_ref s, const std::string& symbol, time from, seconds tick_seconds, std::optional<uint32_t> tick_count)
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

    subscriber->start_time = from + m_server_time_delta;
    subscriber->tick_seconds = tick_seconds;

    auto self = shared_from_this();
    mScheduler->SpawnSSL([=](io_context& io, ssl::context& ssl, yield_context yield) {
        self->DoSubscribe(std::move(subscriber), std::move(tick_count), io, ssl, yield);
    });


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
