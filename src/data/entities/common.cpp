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

#include "common.hpp"
#include <stdexcept>

namespace scratcher::data {

// Define static array of instances
const std::array<OrderSide, 2> OrderSide::instances_ = {{
    OrderSide{"Buy", 0},
    OrderSide{"Sell", 1}
}};

// Define named references
const OrderSide& OrderSide::BUY = instances_[0];
const OrderSide& OrderSide::SELL = instances_[1];

std::string to_string(const OrderSide& side) {
    return std::string(side.name());
}

std::string to_string(OrderType type) {
    switch (type) {
        case OrderType::Market: return "Market";
        case OrderType::Limit: return "Limit";
    }
    throw std::invalid_argument("Invalid OrderType");
}

OrderType order_type_from_string(const std::string& str) {
    if (str == "Market") return OrderType::Market;
    if (str == "Limit") return OrderType::Limit;
    throw std::invalid_argument("Invalid OrderType string: " + str);
}

} // namespace scratcher::data
