// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the Intellectual Property Reserve License (IPRL)
// -----BEGIN PGP PUBLIC KEY BLOCK-----
//
// mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
// lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b21tYWlsLmNvbT6IkAQTFggA
// OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
// AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
// 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
// =KKu7
// -----END PGP PUBLIC KEY BLOCK-----

#ifndef BYBIT_INSTRUMENT_HPP
#define BYBIT_INSTRUMENT_HPP

#include <string>
#include <glaze/glaze.hpp>

#include "enums.hpp"

namespace scratcher::bybit {

// InstrumentInfo - canonical flattened representation for DAO storage
// This is the single source of truth for instrument data storage
// Uses C++23 auto-reflection for DAO - no glz::meta needed
struct InstrumentInfo {
    // Basic fields
    std::string symbol;                 // Symbol name (primary key)
    std::string baseCoin;               // Base coin
    std::string quoteCoin;              // Quote coin
    std::string symbolType;             // Geographic region classification (replaces innovation)
    std::string innovation;             // Innovation zone token ("0" or "1") - deprecated
    InstrumentStatus status;            // Instrument status
    std::string marginTrading;          // Margin trading support ("utaOnly", etc.)
    std::string stTag;                  // Special treatment label ("0" or "1")

    // Flattened priceFilter fields
    std::string tickSize;               // Tick size for price

    // Flattened lotSizeFilter fields
    std::string basePrecision;          // Base coin precision
    std::string quotePrecision;         // Quote coin precision
    std::string minOrderQty;            // Minimum order quantity (deprecated)
    std::string maxOrderQty;            // Maximum order quantity (deprecated)
    std::string minOrderAmt;            // Minimum order amount
    std::string maxOrderAmt;            // Maximum order amount (deprecated)
    std::string maxLimitOrderQty;       // Maximum limit order quantity
    std::string maxMarketOrderQty;      // Maximum market order quantity
    std::string postOnlyMaxLimitOrderSize;   // Maximum post-only/RPI order size

    // Flattened riskParameters fields
    std::string priceLimitRatioX;       // Price limit ratio X
    std::string priceLimitRatioY;       // Price limit ratio Y
};

// Helper structures for nested JSON deserialization
// These match the ByBit API nested structure
namespace detail {
    struct PriceFilter {
        std::string tickSize;
        std::string minPrice;
        std::string maxPrice;
    };

    struct LotSizeFilter {
        std::string basePrecision;
        std::string quotePrecision;
        std::string minOrderQty;
        std::string maxOrderQty;
        std::string minOrderAmt;
        std::string maxOrderAmt;
        std::string maxLimitOrderQty;
        std::string maxMarketOrderQty;
        std::string postOnlyMaxLimitOrderSize;
    };

    struct RiskParameters {
        std::string priceLimitRatioX;
        std::string priceLimitRatioY;
    };
}

// InstrumentInfoAPI - matches ByBit API JSON structure with nested objects
// Deserializes directly from API format, then converts to flat InstrumentInfo for DAO
struct InstrumentInfoAPI {
    std::string symbol;
    std::string baseCoin;
    std::string quoteCoin;
    std::string symbolType;
    std::string innovation;
    InstrumentStatus status;
    std::string marginTrading;
    std::string stTag;

    // Nested objects from ByBit API
    detail::PriceFilter priceFilter;
    detail::LotSizeFilter lotSizeFilter;
    detail::RiskParameters riskParameters;

    // Conversion operator to flat InstrumentInfo for DAO storage
    operator InstrumentInfo() const {
        return InstrumentInfo {
            symbol, baseCoin, quoteCoin, symbolType, innovation, status, marginTrading, stTag,
            priceFilter.tickSize,
            lotSizeFilter.basePrecision, lotSizeFilter.quotePrecision,
            lotSizeFilter.minOrderQty, lotSizeFilter.maxOrderQty,
            lotSizeFilter.minOrderAmt, lotSizeFilter.maxOrderAmt,
            lotSizeFilter.maxLimitOrderQty, lotSizeFilter.maxMarketOrderQty,
            lotSizeFilter.postOnlyMaxLimitOrderSize,
            riskParameters.priceLimitRatioX, riskParameters.priceLimitRatioY
        };
    }
};

} // namespace scratcher::bybit

#endif // BYBIT_INSTRUMENT_HPP
