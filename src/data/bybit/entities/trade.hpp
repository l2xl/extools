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

#ifndef BYBIT_TRADE_HPP
#define BYBIT_TRADE_HPP

#include <string>
#include <deque>
#include <optional>
#include "enums.hpp"

namespace scratcher::bybit {

// Trade execution information from ByBit Trade/TradeHistory API
struct Trade {
    std::string symbol;                         // Symbol name
    std::string orderId;                        // Order ID
    std::optional<std::string> orderLinkId;     // User customized order ID
    OrderSide side;                             // Side (Buy/Sell)
    std::string orderPrice;                     // Order price
    std::string orderQty;                       // Order qty
    std::optional<std::string> leavesQty;       // The remaining qty not executed
    std::optional<std::string> createType;      // Order create type
    std::string orderType;                      // Order type. "Market", "Limit"
    std::optional<std::string> stopOrderType;   // Stop order type
    std::string execFee;                        // Executed trading fee
    std::optional<std::string> execFeeV2;       // Spot leg transaction fee, only works for execType=FutureSpread
    std::string execId;                         // Execution ID
    std::string execPrice;                      // Execution price
    std::string execQty;                        // Execution qty
    ExecType execType;                          // Executed type
    std::optional<std::string> execValue;       // Executed order value
    std::string execTime;                       // Executed timestamp (ms)
    std::optional<std::string> feeCurrency;     // Spot trading fee currency
    bool isMaker;                               // Is maker order. true: maker, false: taker
    std::optional<std::string> feeRate;         // Trading fee rate
    std::optional<std::string> tradeIv;         // Implied volatility. Valid for option
    std::optional<std::string> markIv;          // Implied volatility of mark price. Valid for option
    std::optional<std::string> markPrice;       // The mark price of the symbol when executing
    std::optional<std::string> indexPrice;      // The index price of the symbol when executing. Valid for option only
    std::optional<std::string> underlyingPrice; // The underlying price of the symbol when executing. Valid for option
    std::optional<std::string> blockTradeId;    // Paradigm block trade ID
    std::optional<std::string> closedSize;      // Closed position size
    std::optional<long> seq;                    // Cross sequence, used to associate each fill and each position update
    std::optional<std::string> extraFees;       // Trading fee rate information
};



} // namespace scratcher::bybit

#endif // BYBIT_TRADE_HPP