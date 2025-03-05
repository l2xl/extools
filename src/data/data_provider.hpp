// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef DATA_PROVIDER_HPP
#define DATA_PROVIDER_HPP

#include "currency.hpp"

#include <functional>
#include <chrono>

namespace scratcher {

// using std::chrono::seconds;
// using std::chrono::milliseconds;
//
typedef std::chrono::utc_clock::time_point time;

template <typename trading_traits>
struct Trade
{
    std::string symbol;
    std::string id;
    time trade_time;

    typename trading_traits::price_type price;
    typename trading_traits::volume_type volume;
};

//template <typename trading_traits>
class DataProvider {
public:
    virtual ~DataProvider() = default;

    //virtual void SubscribeOrders() = 0;
    //virtual void SubscribeKLines() = 0;
    virtual void SubscribePublicTrades() = 0;
    //virtual void SubscribeOrderBook() = 0;


};

// template <typename trading_traits>
// void SubscribePublicTrades(std::shared_ptr<DataProvider/*<trading_traits>*/> dataProvider, std::function<void(const Trade<trading_traits>&)>)
// {
//     dataProvider->SubscribePublicTrades();
// }

} // scratcher

#endif //DATA_PROVIDER_HPP
