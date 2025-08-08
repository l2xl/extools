// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef BYBIT_ENUMS_HPP
#define BYBIT_ENUMS_HPP

namespace scratcher::bybit {

enum class InstrumentStatus {
    PreLaunch,
    Trading,
    Settling,
    Delivering,
    Closed
};

enum class ExecType {
    Trade,
    AdlTrade,
    Funding,
    BustTrade,
    Settle,
    BlockTrade,
    MovePosition
};

enum class Category {
    Spot,
    Linear,
    Inverse,
    Option
};

enum class OrderSide {
    Buy,
    Sell
};

} // namespace scratcher::bybit

#endif // BYBIT_ENUMS_HPP
