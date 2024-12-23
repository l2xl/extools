// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef MARKET_DATA_SINK_HPP
#define MARKET_DATA_SINK_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/container/flat_map.hpp>


#include <chrono>
#include <memory>
#include <deque>
#include <list>
#include <shared_mutex>

#include "mainwindow.h"

class Config;

namespace sceduler {
class AsioScheduler;
}

namespace bybit {

class SchedulerError : public std::runtime_error
{
public:
    explicit SchedulerError(const std::string& what) noexcept: std::runtime_error(what) {}
    explicit SchedulerError(std::string&& what) noexcept: std::runtime_error(move(what)) {}
};
class SchedulerSubscriberExpired : public SchedulerError
{
public:
    explicit SchedulerSubscriberExpired() noexcept: SchedulerError("expired") {}
};
class SchedulerParamMismatch : public SchedulerError
{
public:
    explicit SchedulerParamMismatch(std::string&& what) noexcept: SchedulerError(what) {}
};


using std::chrono::seconds;
using std::chrono::milliseconds;

typedef std::chrono::system_clock::time_point time;
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

    std::shared_ptr<scratcher::AsioScheduler> mScheduler;

    boost::beast::error_code m_status;
    boost::asio::ip::tcp::resolver::results_type m_resolved_host;

    milliseconds m_request_halftrip;
    milliseconds m_server_time_delta;


    std::list<std::shared_ptr<ByBitSubscriber>> m_subscribers;

    std::mutex m_data_cache_mutex;
    data_list_type m_data_cache;

    void DoPing(boost::asio::io_context& io, boost::asio::ssl::context& ssl, boost::asio::yield_context yield);
    void DoSubscribe(std::shared_ptr<ByBitSubscriber> subscriber, std::optional<uint32_t> tick_count,
        boost::asio::io_context& io, boost::asio::ssl::context& ssl, boost::asio::yield_context yield);
public:
    explicit ByBitApi(std::shared_ptr<Config> config, std::shared_ptr<scratcher::AsioScheduler> scheduler);

    subscriber_ref Subscribe(subscriber_ref subscriber, const std::string& symbol, time from, seconds tick_seconds, std::optional<uint32_t> tick_count = {});
    void Unsubscribe(subscriber_ref subscriber);

};

}

#endif //MARKET_DATA_SINK_HPP
