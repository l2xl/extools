// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef BYBIT_INSTRUMENT_HPP
#define BYBIT_INSTRUMENT_HPP

#include <string>

namespace scratcher::bybit {

// Helper structures for nested JSON objects
struct PriceFilter {
    std::string tickSize;               // Tick size for price
    std::string minPrice;               // Minimum price (optional)
    std::string maxPrice;               // Maximum price (optional)
};

struct LotSizeFilter {
    std::string basePrecision;          // Base coin precision
    std::string quotePrecision;         // Quote coin precision
    std::string minOrderQty;            // Minimum order quantity
    std::string maxOrderQty;            // Maximum order quantity
    std::string minOrderAmt;            // Minimum order amount
    std::string maxOrderAmt;            // Maximum order amount
};

struct RiskParameters
{
    std::string priceLimitRatioX;
    std::string priceLimitRatioY;
};


// Raw instrument data from ByBit API
struct InstrumentInfo {
    std::string symbol;                 // Symbol name
    std::string baseCoin;               // Base coin
    std::string quoteCoin;              // Quote coin
    std::string innovation;             // Innovation zone token ("0" or "1")
    std::string status;                 // Instrument status ("Trading", etc.)
    std::string marginTrading;          // Margin trading support ("utaOnly", etc.)
    std::string stTag;                  // Special treatment label ("0" or "1")
    PriceFilter priceFilter;            // Price filter
    LotSizeFilter lotSizeFilter;        // Lot size filter
    RiskParameters riskParameters;
};



} // namespace scratcher::bybit

#endif // BYBIT_INSTRUMENT_HPP
