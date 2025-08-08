// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef BYBIT_RESPONSE_HPP
#define BYBIT_RESPONSE_HPP

#include <string>
#include <deque>

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

// For paginated responses
template<typename T>
struct PaginatedResult {
    std::string category;               // Product category
    std::deque<T> list;                // List of items
    std::string nextPageCursor;         // Cursor for next page
};

// Time API response
struct TimeResult {
    uint64_t timeSecond{0};             // Server time in seconds
    uint64_t timeNano{0};               // Nanosecond part
};

} // namespace scratcher::bybit

#endif // BYBIT_RESPONSE_HPP
