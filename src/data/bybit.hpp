// Scratcher project
// Copyright (c) 2024-2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef BYBIT_HPP
#define BYBIT_HPP

#include <mutex>


#include <memory>
#include <deque>
#include <list>
#include <shared_mutex>
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

typedef trading_trait<currency<uint64_t, dec<8>>, currency<uint64_t, dec<2>>> BTCUSDC;

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

typedef std::chrono::utc_clock::time_point time;

typedef std::array<double, 4> kline_type;
typedef std::deque<kline_type> kline_sequence_type;
typedef kline_sequence_type::iterator kline_iterator;
typedef kline_type::const_iterator const_kline_iterator;

struct ByBitSubscription;
//struct ByBitDataCache;

class ByBitStream;

class ByBitApi: public std::enable_shared_from_this<ByBitApi>
{
public:
    friend class ByBitStream;
    typedef std::weak_ptr<ByBitSubscription> subscription_ref;

private:
    //typedef std::list<std::weak_ptr<ByBitDataCache>> data_list_type;

    const std::shared_ptr<Config> mConfig;

    std::shared_ptr<AsioScheduler> mScheduler;

    boost::asio::ip::tcp::resolver::results_type m_resolved_http_host;
    boost::asio::ip::tcp::resolver::results_type m_resolved_websock_host;

    milliseconds m_request_halftrip = milliseconds(0);
    std::optional<milliseconds> m_server_time_delta;

    std::list<std::shared_ptr<ByBitStream>> m_streams;

    std::list<std::shared_ptr<ByBitSubscription>> m_subscriptions;

    //std::mutex m_data_cache_mutex;
    //data_list_type m_data_cache;

    void Resolve();

    void Spawn(std::function<void(yield_context yield)>);

    void DoRequestServer(std::string_view, std::function<void(const nlohmann::json&)>, yield_context &yield);

    void DoHttpRequest(std::shared_ptr<ByBitSubscription> subscriber, std::optional<uint32_t> tick_count, yield_context &yield);

    void DoPing(yield_context &yield);

    void DoSubscribePublicTrades(const subscription_ref &subscription, yield_context &yield);

    void CalcServerTime(time server_time, time request_time, time response_time);
public:
    explicit ByBitApi(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler);
    static std::shared_ptr<ByBitApi> Create(std::shared_ptr<Config> config, std::shared_ptr<AsioScheduler> scheduler);

    subscription_ref Subscribe(const subscription_ref &subscription, const std::string& symbol, time from, seconds tick_seconds, std::optional<uint32_t> tick_count = {});
    void Unsubscribe(subscription_ref subscription);
};

class ByBitDataProvider: public DataProvider
{
    const std::string m_symbol;
    std::shared_ptr<ByBitApi> mApi;

    std::shared_ptr<ByBitStream> m_public_trade_stream;

    std::deque<Trade<BTCUSDC>> m_public_trade_cache;
public:
    ByBitDataProvider(std::string_view symbol, std::shared_ptr<ByBitApi> api)
        : DataProvider()
        , m_symbol(symbol), mApi(move(api))
    { }

    void SubscribePublicTrades() override;
};

}

#endif //BYBIT_HPP
