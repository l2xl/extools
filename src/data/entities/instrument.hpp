// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the Intellectual Property Reserve License (IPRL)
// -----BEGIN PGP PUBLIC KEY BLOCK-----
//
// mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
// lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b25tYWlsLmNvbT6IkAQTFggA
// OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
// AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
// 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
// =KKu7
// -----END PGP PUBLIC KEY BLOCK-----

#ifndef DATA_ENTITIES_INSTRUMENT_HPP
#define DATA_ENTITIES_INSTRUMENT_HPP

#include <string>
#include "currency.hpp"

namespace scratcher::data {

struct Instrument {
    std::string symbol;                 // Symbol name (BTCUSDT)
    std::string base_coin;              // Base coin (BTC)
    std::string quote_coin;             // Quote coin (USDT)
    
    // Common trading parameters
    currency<uint64_t> price_point;
    currency<uint64_t> price_precision;
    currency<uint64_t> volume_point;
    currency<uint64_t> volume_precision;
    currency<uint64_t> min_volume;   // Minimum order quantity
    currency<uint64_t> max_volume;   // Maximum order quantity
    currency<uint64_t> min_amount;   // Minimum order amount
    currency<uint64_t> max_amount;   // Maximum order amount
    
    // Equality operators for comparison
    bool operator==(const Instrument&) const = default;
    bool operator!=(const Instrument&) const = default;
};

} // namespace scratcher::data

#endif // DATA_ENTITIES_INSTRUMENT_HPP
