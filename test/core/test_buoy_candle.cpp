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


// Mock trade structure for testing
struct MockTrade {
    time_point trade_time;
    uint64_t price_points;
    uint64_t volume_points;
};

TEST_CASE("SingleBuoyAppend")
{
    BuoyCandleQuotes quotes(1000); // 1 second candle duration

    REQUIRE(quotes.buoy_duration() == 1000);
    
    // Create mock trade data
    std::vector<MockTrade> trades;
    time_point base_time = time_point();
    
    // Add some sample trades
    trades.push_back({base_time, 100, 10});
    trades.push_back({base_time + milliseconds(1000), 100, 10});
    trades.push_back({base_time + milliseconds(2000), 1000, 100});

    quotes.AppendTrades(trades, 2500, 50);
    
    // Check that data was processed
    //CHECK(quotes.first_buoy_timestamp() <= duration_cast<milliseconds>(base_time.time_since_epoch()).count());
    CHECK(quotes.buoy_data().size() == 2);

    CHECK(quotes.buoy_data().front().volume == 10);
    CHECK(quotes.buoy_data().front().min == 50);
    CHECK(quotes.buoy_data().front().max == 100);
    CHECK(quotes.buoy_data().front().mean == 100);

    CHECK(quotes.buoy_data().back().volume == 10);
    CHECK(quotes.buoy_data().back().min == 100);
    CHECK(quotes.buoy_data().back().max == 100);
    CHECK(quotes.buoy_data().back().mean == 100);

    CHECK(quotes.active_buoy().volume == 100);
    CHECK(quotes.active_buoy().min == 100);
    CHECK(quotes.active_buoy().max == 1000);
    CHECK(quotes.active_buoy().mean == 1000);
}

TEST_CASE("TwoTradesAppend")
{
    BuoyCandleQuotes quotes(1000); // 1 second candle duration

    // Create mock trade data
    std::vector<MockTrade> trades;
    time_point base_time = time_point();

    // Add some sample trades
    trades.push_back({base_time + milliseconds(1000), 200, 10});
    trades.push_back({base_time + milliseconds(1500), 300, 100});

    quotes.AppendTrades(trades, 2000, 100);

    // Check that data was processed
    CHECK(quotes.buoy_data().size() == 1);

    CHECK(quotes.buoy_data().back().volume == 110);
    CHECK(quotes.buoy_data().back().min == 100);
    CHECK(quotes.buoy_data().back().max == 300);
    CHECK(quotes.buoy_data().back().mean == 290);

    CHECK(quotes.active_buoy().volume == 0);
    CHECK(quotes.active_buoy().min == 300);
    CHECK(quotes.active_buoy().max == 300);
    CHECK(quotes.active_buoy().mean == 300);
}

TEST_CASE("SimpleActiveCandle")
{
    BuoyCandleQuotes quotes(1000); // 1 second candle duration

    std::vector<MockTrade> trades;
    time_point base_time = time_point() + milliseconds(1000);

    // Add some sample trades
    trades.push_back({base_time, 100, 10});
    trades.push_back({base_time + milliseconds(1000), 200, 20});
    trades.push_back({base_time + milliseconds(2000), 300, 30});


    quotes.AppendTrades(trades, 2000, 100);

    CHECK(quotes.first_buoy_timestamp() == 1000);
    CHECK(quotes.first_trade_timestamp() == 1000);
    CHECK(quotes.last_trade_timestamp() == 3000);

    CHECK(quotes.buoy_data().size() == 2);

    CHECK(quotes.buoy_data().front().volume == 10);
    CHECK(quotes.buoy_data().front().min == 100);
    CHECK(quotes.buoy_data().front().max == 100);
    CHECK(quotes.buoy_data().front().mean == 100);

    CHECK(quotes.buoy_data().back().volume == 20);
    CHECK(quotes.buoy_data().back().min == 100);
    CHECK(quotes.buoy_data().back().max == 200);
    CHECK(quotes.buoy_data().back().mean == 200);

    CHECK(quotes.active_buoy().volume == 30);
    CHECK(quotes.active_buoy().min == 200);
    CHECK(quotes.active_buoy().max == 300);
    CHECK(quotes.active_buoy().mean == 300);
}

