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

typedef std::chrono::utc_clock::time_point time;

enum class TradeSide:uint8_t { SELL, BUY };

struct Trade
{
    std::string id;
    time trade_time;

    uint64_t price_points;
    uint64_t volume_points;

    TradeSide side;
};

class DataProvider {
};


} // scratcher

#endif //DATA_PROVIDER_HPP
