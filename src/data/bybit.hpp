// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef MARKET_DATA_SINK_HPP
#define MARKET_DATA_SINK_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/container/flat_map.hpp>


#include <memory>
#include <deque>
#include <list>
#include <shared_mutex>

class Config;

class ByBitError : public std::runtime_error
{
public:
    explicit ByBitError(const std::string& what) noexcept: std::runtime_error(what) {}
    explicit ByBitError(std::string&& what) noexcept: std::runtime_error(move(what)) {}
};
class ByBitSubscriberExpired : public ByBitError
{
public:
    explicit ByBitSubscriberExpired() noexcept: ByBitError("expired") {}
};
class ByBitParamMismatch : public ByBitError
{
public:
    explicit ByBitParamMismatch(std::string&& what) noexcept: ByBitError(what) {}
};

namespace ssl = boost::asio::ssl;
namespace ip = boost::asio::ip;
namespace websock = boost::beast::websocket;

namespace bybit {

using std::chrono::seconds;
using std::chrono::milliseconds;

typedef std::chrono::utc_clock::time_point time;
typedef std::array<double, 4> kline_type;
typedef std::deque<kline_type> kline_sequence_type;
typedef kline_sequence_type::iterator kline_iterator;
typedef kline_type::const_iterator const_kline_iterator;

struct ByBitSubscriber;
struct ByBitDataCache;

class ByBitApi
{
public:

    typedef std::weak_ptr<ByBitSubscriber> subscriber_ref;

private:
    typedef std::list<std::weak_ptr<ByBitDataCache>> data_list_type;

    std::string m_host;
    std::string m_port;

    boost::asio::io_context io_ctx;
    ssl::context ssl_ctx;

    ip::tcp::resolver::results_type m_resolved_host;

    milliseconds m_request_halftrip;
    milliseconds m_server_time_delta;


    std::list<std::shared_ptr<ByBitSubscriber>> m_subscribers;

    std::mutex m_data_cache_mutex;
    data_list_type m_data_cache;


    void DoSubscribe(std::shared_ptr<ByBitSubscriber> subscriber, std::optional<uint32_t> tick_count);
public:
    explicit ByBitApi(std::shared_ptr<Config> config);

    subscriber_ref Subscribe(subscriber_ref subscriber, const std::string& symbol, time from, seconds tick_seconds, std::optional<uint32_t> tick_count = {});
    void Unsubscribe(subscriber_ref subscriber);

};

}

#endif //MARKET_DATA_SINK_HPP
