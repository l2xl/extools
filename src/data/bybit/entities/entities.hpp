// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef BYBIT_ENTITIES_HPP
#define BYBIT_ENTITIES_HPP

// Include all ByBit entity definitions
#include "enums.hpp"
#include "response.hpp"
#include "instrument.hpp"
#include "public_trade.hpp"

namespace scratcher::bybit {

// Type aliases for common response types
using TimeResponse = ApiResponse<TimeResult>;
using InstrumentResponse = ApiResponse<PaginatedResult<InstrumentInfo>>;
using PublicTradeResponse = ApiResponse<PaginatedResult<PublicTrade>>;

} // namespace scratcher::bybit

#endif // BYBIT_ENTITIES_HPP
