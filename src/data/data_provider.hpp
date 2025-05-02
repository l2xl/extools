// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef DATA_PROVIDER_HPP
#define DATA_PROVIDER_HPP


#include <functional>
#include <chrono>
#include <deque>

#include <boost/container/flat_map.hpp>

#include "currency.hpp"
#include "timedef.hpp"

namespace scratcher {


enum class TradeSide:uint8_t { SELL, BUY };

struct Trade
{
    std::string id;
    time_point trade_time;

    uint64_t price_points;
    uint64_t volume_points;

    TradeSide side;
};

struct Scratcher;

struct IDataProvider {
    virtual ~IDataProvider() = default;
    virtual void AddInsctrumentDataUpdateHandler(std::function<void()> h) = 0;
    virtual void AddMarketDataUpdateHandler(std::function<void()> h) = 0;

    virtual currency<uint64_t> PricePoint() const = 0;

    virtual const std::deque<Trade>& PublicTradeCache() const = 0;
    virtual const boost::container::flat_map<uint64_t, uint64_t>& Bids() const = 0;
    virtual const boost::container::flat_map<uint64_t, uint64_t>& Asks() const = 0;

    virtual std::shared_ptr<Scratcher> MakePriceRulerScratcher() const = 0;
    virtual std::shared_ptr<Scratcher> MakeQuoteGraphScratcher() const = 0;
    virtual std::shared_ptr<Scratcher> MakeOrdersSpreadScratcher() const = 0;
};


} // scratcher

#endif //DATA_PROVIDER_HPP
