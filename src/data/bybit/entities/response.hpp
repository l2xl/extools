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

#ifndef BYBIT_RESPONSE_HPP
#define BYBIT_RESPONSE_HPP

#include <string>
#include <deque>
#include <optional>
#include "enums.hpp"

namespace scratcher::bybit {

struct RetExtInfo
{

};

// Base response structure for all ByBit API responses
template<typename T>
struct ApiResponse {
    int retCode{0};                     // Return code (0 for success)
    std::string retMsg;                 // Return message ("OK" for success)
    T result;                           // Result data
    RetExtInfo retExtInfo;             // Extended information
    uint64_t time{0};                   // Response timestamp (ms)
};

// For list responses (both paginated and non-paginated)
template<typename T>
struct ListResult {
    std::optional<Category> category;           // Product category (optional)
    std::deque<T> list;                        // List of items
    std::optional<std::string> nextPageCursor; // Cursor for next page (optional)
};

// Time API response
struct TimeResult {
    uint64_t timeSecond{0};             // Server time in seconds
    uint64_t timeNano{0};               // Nanosecond part
};

} // namespace scratcher::bybit

#endif // BYBIT_RESPONSE_HPP
