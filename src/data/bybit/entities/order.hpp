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

#ifndef BYBIT_ORDER_HPP
#define BYBIT_ORDER_HPP

#include <string>
#include <optional>
#include "enums.hpp"

namespace scratcher::bybit {

// Order information from ByBit Trade/OrderHistory API
struct Order {
    std::string orderId;                        // Order ID
    std::optional<std::string> orderLinkId;     // User customised order ID
    std::optional<std::string> blockTradeId;    // Block trade ID
    std::string symbol;                         // Symbol name
    std::string price;                          // Order price
    std::string qty;                            // Order qty
    OrderSide side;                             // Side (Buy/Sell)
    std::optional<std::string> isLeverage;      // Whether to borrow. Unified spot only. "0": false, "1": true
    std::optional<int> positionIdx;             // Position index. Used to identify positions in different position modes
    std::string orderStatus;                    // Order status
    std::optional<std::string> createType;      // Order create type (only for category=linear or inverse)
    std::optional<std::string> cancelType;      // Cancel type
    std::optional<std::string> rejectReason;    // Reject reason
    std::string avgPrice;                       // Average filled price
    std::optional<std::string> leavesQty;       // The remaining qty not executed
    std::optional<std::string> leavesValue;     // The estimated value not executed
    std::string cumExecQty;                     // Cumulative executed order qty
    std::optional<std::string> cumExecValue;    // Cumulative executed order value
    std::optional<std::string> cumExecFee;      // Cumulative executed trading fee
    std::string timeInForce;                    // Time in force
    std::string orderType;                      // Order type. "Market", "Limit"
    std::optional<std::string> stopOrderType;   // Stop order type
    std::optional<std::string> orderIv;         // Implied volatility
    std::optional<std::string> marketUnit;      // The unit for qty when create Spot market orders for UTA account
    std::optional<std::string> slippageToleranceType; // Spot and Futures market order slippage tolerance type
    std::optional<std::string> slippageTolerance; // Slippage tolerance value
    std::optional<std::string> triggerPrice;    // Trigger price
    std::optional<std::string> takeProfit;      // Take profit price
    std::optional<std::string> stopLoss;        // Stop loss price
    std::optional<std::string> tpslMode;        // TP/SL mode
    std::optional<std::string> ocoTriggerBy;    // The trigger type of Spot OCO order
    std::optional<std::string> tpLimitPrice;    // The limit order price when take profit price is triggered
    std::optional<std::string> slLimitPrice;    // The limit order price when stop loss price is triggered
    std::optional<std::string> tpTriggerBy;     // The price type to trigger take profit
    std::optional<std::string> slTriggerBy;     // The price type to trigger stop loss
    std::optional<int> triggerDirection;        // Trigger direction. 1: rise, 2: fall
    std::optional<std::string> triggerBy;       // The price type of trigger price
    std::optional<std::string> lastPriceOnCreated; // Last price when place the order
    std::optional<std::string> basePrice;       // Last price when place the order, Spot has this field only
    std::optional<bool> reduceOnly;             // Reduce only. true means reduce position size
    std::optional<bool> closeOnTrigger;         // Close on trigger
    std::optional<std::string> placeType;       // Place type, option used. "iv", "price"
    std::optional<std::string> smpType;         // SMP execution type
    std::optional<int> smpGroup;                // Smp group ID. If the UID has no group, it is 0 by default
    std::optional<std::string> smpOrderId;      // The counterparty's orderID which triggers this SMP execution
    std::string createdTime;                    // Order created timestamp (ms)
    std::string updatedTime;                    // Order updated timestamp (ms)
    std::optional<std::string> extraFees;       // Trading fee rate information
};



} // namespace scratcher::bybit

#endif // BYBIT_ORDER_HPP
