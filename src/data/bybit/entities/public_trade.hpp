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

#ifndef BYBIT_PUBLIC_TRADE_HPP
#define BYBIT_PUBLIC_TRADE_HPP

#include <string>
#include "enums.hpp"

namespace scratcher::bybit {

struct PublicTrade {
    std::string exec_id;                // Execution ID
    std::string symbol;                 // Symbol name
    std::string price;                  // Execution price (string to preserve precision)
    std::string size;                   // Execution size (string to preserve precision)
    OrderSide side;                     // Trade side (Buy/Sell)
    uint64_t time{0};                   // Execution timestamp (ms)
    bool is_block_trade{false};         // Whether it's a block trade
    bool is_rpi_trade{false};           // Whether it's RPI trade
    std::string m_p;                    // Mark price (for options)
    std::string i_p;                    // Index price (for options)
    std::string m_iv;                   // Mark IV (for options)
    std::string iv;                     // IV (for options)
};

} // namespace scratcher::bybit

#endif // BYBIT_PUBLIC_TRADE_HPP
