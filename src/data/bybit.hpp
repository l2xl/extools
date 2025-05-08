// Scratcher project
// Copyright (c) 2024-2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef BYBIT_HPP
#define BYBIT_HPP

#include <mutex>
#include <memory>
#include <deque>
#include <shared_mutex>
#include <iostream>

#include <boost/container/flat_map.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <nlohmann/json.hpp>

#include "scheduler.hpp"
#include "data_provider.hpp"
#include "currency.hpp"

class Config;

namespace scratcher::bybit {

class Config
{
public:
    virtual ~Config() = default;

    virtual const std::string& HttpHost() const = 0;
    virtual const std::string& HttpPort() const = 0;

    virtual const std::string& StreamHost() const = 0;
    virtual const std::string& StreamPort() const = 0;
};

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
    explicit SchedulerParamMismatch(std::string&& what) noexcept: SchedulerError(move(what)) {}
};

class PublicStreamClosed : public std::runtime_error
{
public:
    explicit PublicStreamClosed() noexcept: std::runtime_error("ByBit Public Stream is closed") {}
};

class WrongServerData : public std::runtime_error
{
public:
    WrongServerData(std::string&& what) noexcept : std::runtime_error(move(what)) {}
};

using std::chrono::seconds;
using std::chrono::milliseconds;

typedef std::chrono::utc_clock::time_point time;

typedef std::array<double, 4> kline_type;
typedef std::deque<kline_type> kline_sequence_type;
typedef kline_sequence_type::iterator kline_iterator;
typedef kline_type::const_iterator const_kline_iterator;

struct ByBitSubscription;
struct ByBitDataManager;

class ByBitStream;

class ByBitApi: public std::enable_shared_from_this<ByBitApi>
{
public:
    friend class ByBitStream;

private:
    const std::shared_ptr<Config> mConfig;

    std::shared_ptr<AsioScheduler> mScheduler;

    boost::asio::ip::tcp::resolver::results_type m_resolved_http_host;
    std::atomic_bool m_is_http_resolved = false;
    boost::asio::ip::tcp::resolver::results_type m_resolved_websock_host;
    std::atomic_bool m_is_websock_resolved = false;

    milliseconds m_request_halftrip = milliseconds(0);
    std::optional<milliseconds> m_server_time_delta;

    boost::container::flat_map<std::string, std::shared_ptr<ByBitSubscription>> m_subscriptions;
    std::mutex m_subscriptions_mutex;

    std::shared_ptr<ByBitStream> m_public_spot_stream;
    boost::lockfree::spsc_queue<std::string> m_data_queue;
    boost::asio::strand<boost::asio::any_io_executor> m_data_queue_strand;

    void Resolve();

    static boost::asio::awaitable<boost::asio::ip::tcp::resolver::results_type> coResolve(boost::asio::ip::tcp::resolver resolver, std::string host, std::string port);

    static boost::asio::awaitable<nlohmann::json> coRequestServer(std::shared_ptr<ByBitApi> self, std::string_view request_string);

    static boost::asio::awaitable<void> DoPing(std::shared_ptr<ByBitApi> self);

    static boost::asio::awaitable<nlohmann::json> coGetInstrumentInfo(std::shared_ptr<ByBitApi> self, std::shared_ptr<ByBitSubscription> subscription);

    void SubscribePublicStream(const std::shared_ptr<ByBitSubscription>& subscription);

    static void HandleConnectionData(std::weak_ptr<ByBitApi> ref, std::string&& data);
    static void HandleConnectionError(std::weak_ptr<ByBitApi> ref, boost::system::error_code ec);

    void CalcServerTime(time server_time, time request_time, time response_time);

    struct EnsurePrivate {};
public:
    explicit ByBitApi(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler, EnsurePrivate);
    static std::shared_ptr<ByBitApi> Create(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler);

    const std::shared_ptr<AsioScheduler>& Scheduler() const
    { return mScheduler; }

    std::shared_ptr<ByBitSubscription> Subscribe(const std::string& symbol, std::shared_ptr<ByBitDataManager> manager);
    void Unsubscribe(const std::string& symbol);
};

}

#endif //BYBIT_HPP
