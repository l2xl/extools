// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef DATA_COLLECTOR_HPP
#define DATA_COLLECTOR_HPP

#include <string>
#include <memory>
#include <deque>
#include <list>
#include <boost/asio/detail/socket_option.hpp>

#include <boost/system/system_error.hpp>
#include <boost/container/flat_map.hpp>

#include <nlohmann/json.hpp>

#include "data_provider.hpp"
#include "currency.hpp"


namespace scratcher {

class Scratcher;

namespace bybit {

class ByBitApi;
class SubscriptionTopic;


class ByBitDataManager: public IDataProvider, public std::enable_shared_from_this<ByBitDataManager>
{
    const std::string m_symbol;
    std::shared_ptr<ByBitApi> mApi;

    std::optional<currency<uint64_t>> m_price_point;
    std::optional<currency<uint64_t>> m_price_precision;
    std::optional<currency<uint64_t>> m_volume_point;
    std::optional<currency<uint64_t>> m_volume_precision;
    std::optional<currency<uint64_t>> m_min_volume; // Min order volume
    std::optional<currency<uint64_t>> m_max_volume; // Max order volume
    std::optional<currency<uint64_t>> m_min_amount; // Min order amount/cost
    std::optional<currency<uint64_t>> m_max_amount; // Max order amount/cost

    pubtrade_cache_t m_pubtrade_cache;
    mutable std::mutex m_pubtrade_mutex;

    boost::container::flat_map<uint64_t, uint64_t> m_order_book_bids;
    boost::container::flat_map<uint64_t, uint64_t> m_order_book_asks;

    std::list<std::function<void()>> m_instrument_handlers;
    std::list<std::function<void()>> m_marketdata_handlers;

    struct EnsurePrivate {};
public:
    ByBitDataManager(std::string symbol, std::shared_ptr<ByBitApi> api, EnsurePrivate);
    ~ByBitDataManager() override = default;

    static std::shared_ptr<ByBitDataManager> Create(std::string symbol, std::shared_ptr<ByBitApi> api);

    currency<uint64_t> PricePoint() const override
    { return m_price_point.value_or(currency<uint64_t>(0, 0)); }

    const pubtrade_cache_t& PublicTradeCache() const override
    { return m_pubtrade_cache; }

    std::mutex& PublicTradeMutex() const override
    { return m_pubtrade_mutex; }

    const boost::container::flat_map<uint64_t, uint64_t>& Bids() const override
    { return m_order_book_bids; }

    const boost::container::flat_map<uint64_t, uint64_t>& Asks() const override
    { return m_order_book_asks; }

    void HandleInstrumentData(const nlohmann::json& data);
    bool IsReadyHandleData() const
    { return m_price_point && m_volume_point; }

    void HandleData(const SubscriptionTopic& topic, const std::string& type, const nlohmann::json& data);
    void HandleError(boost::system::error_code ec);

    void AddInsctrumentDataUpdateHandler(std::function<void()> h) override;
    void AddMarketDataUpdateHandler(std::function<void()> h) override;

    std::shared_ptr<Scratcher> MakePriceRulerScratcher() const override;
    std::shared_ptr<Scratcher> MakeQuoteGraphScratcher() const override;
    std::shared_ptr<Scratcher> MakeOrdersSpreadScratcher() const override;

};

} // scratcher::bybit
}

#endif //DATA_COLLECTOR_HPP
