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
