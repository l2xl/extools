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

#include <iostream>
#include <vector>
#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "buoy_candle.hpp"
#include "core_common.hpp"

using namespace scratcher;
using namespace std::chrono;

// Mock trade structure for testing
struct MockTrade {
    system_clock::time_point trade_time;
    uint64_t price_points;
    uint64_t volume_points;
};

TEST_CASE("SimpleEqual")
{
    BuoyCandleQuotes quotes(1000); // 1 second candle duration
    
    // Create mock trade data
    std::vector<MockTrade> trades;
    auto base_time = system_clock::from_time_t(1);
    
    // Add some sample trades
    trades.push_back({base_time, 100, 10});
    trades.push_back({base_time + milliseconds(1000), 100, 10});
    trades.push_back({base_time + milliseconds(2000), 1000, 100});

    Rectangle dataRect;
    dataRect.x = duration_cast<milliseconds>(base_time.time_since_epoch()).count();
    dataRect.w = 2000; // 2 seconds window
    
    quotes.PrepairData(dataRect, trades);
    
    // Check that data was processed
    CHECK(quotes.first_timestamp() <= duration_cast<milliseconds>(base_time.time_since_epoch()).count());
    CHECK(quotes.buoy_data().size() == 2);

    CHECK(quotes.buoy_data().front().volume == 10);
    CHECK(quotes.buoy_data().front().min == 100);
    CHECK(quotes.buoy_data().front().max == 100);
    CHECK(quotes.buoy_data().front().mean == 100);

    CHECK(quotes.buoy_data().back().volume == 10);
    CHECK(quotes.buoy_data().back().min == 100);
    CHECK(quotes.buoy_data().back().max == 100);
    CHECK(quotes.buoy_data().back().mean == 100);
}

TEST_CASE("SimpleIncrease")
{
    BuoyCandleQuotes quotes(1000); // 1 second candle duration

    // Create mock trade data
    std::vector<MockTrade> trades;
    auto base_time = system_clock::from_time_t(1);

    // Add some sample trades
    trades.push_back({base_time, 100, 10});
    trades.push_back({base_time + milliseconds(1000), 200, 10});
    trades.push_back({base_time + milliseconds(2000), 1000, 100});

    Rectangle dataRect;
    dataRect.x = duration_cast<milliseconds>(base_time.time_since_epoch()).count();
    dataRect.w = 2000; // 2 seconds window

    quotes.PrepairData(dataRect, trades);

    // Check that data was processed
    CHECK(quotes.first_timestamp() <= duration_cast<milliseconds>(base_time.time_since_epoch()).count());
    CHECK(quotes.buoy_data().size() == 2);

    CHECK(quotes.buoy_data().front().volume == 10);
    CHECK(quotes.buoy_data().front().min == 100);
    CHECK(quotes.buoy_data().front().max == 100);
    CHECK(quotes.buoy_data().front().mean == 100);

    CHECK(quotes.buoy_data().back().volume == 10);
    CHECK(quotes.buoy_data().back().min == 100);
    CHECK(quotes.buoy_data().back().max == 200);
    CHECK(quotes.buoy_data().back().mean == 200);
}

