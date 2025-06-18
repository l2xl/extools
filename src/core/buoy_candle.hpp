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

#ifndef BOUY_CANDLE_QUOTES_HPP
#define BOUY_CANDLE_QUOTES_HPP

#include <deque>
#include <ranges>
#include <concepts>
#include <limits>
#include <chrono>
#include <cassert>

#include "core_common.hpp"

using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace scratcher {
struct BuoyData
{
    uint64_t min;
    uint64_t max;
    uint64_t mean;
    uint64_t volume;
};



class BuoyCandleQuotes {
    const uint64_t m_buoy_duration;

    uint64_t m_first_buoy_timestamp;
    std::deque<BuoyData> m_buoy_data;

public:
    BuoyCandleQuotes(uint64_t candle_time)
        : m_buoy_duration(candle_time)
    {}

    uint64_t buoy_duration() const
    { return m_buoy_duration; }

    uint64_t first_timestamp() const
    { return m_first_buoy_timestamp; }

    const std::deque<BuoyData>& buoy_data() const
    { return m_buoy_data; }

    template <std::ranges::input_range Range>
    requires requires(std::ranges::range_value_t<Range> trade) {
        trade.trade_time;
        trade.price_points;
        trade.volume_points;
    }
    void PrepairData(Rectangle &dataRect, Range& trades)
    {
        m_buoy_data.clear();

        if (!std::ranges::empty(trades)) {
            auto end = std::ranges::end(trades);
            auto it = std::lower_bound(std::ranges::begin(trades), end,
                dataRect.x_start() - m_buoy_duration, [=](const auto& t, const uint64_t& x) {
                    return duration_cast<milliseconds>(t.trade_time.time_since_epoch()).count() < x;
                });

            uint64_t first_trade_timestamp = duration_cast<milliseconds>(it->trade_time.time_since_epoch()).count();
            m_first_buoy_timestamp = first_trade_timestamp - (first_trade_timestamp % m_buoy_duration);
            uint64_t next_bouy_timestamp = m_first_buoy_timestamp + m_buoy_duration;

            uint64_t window_max_price = 0;
            uint64_t window_min_price = std::numeric_limits<uint64_t>::max();

            BuoyData curBuoy = {std::numeric_limits<uint64_t>::max(), 0, 0, 0};

            auto last_trade_it = end;

            for (auto trade_it = it; trade_it != end; last_trade_it = trade_it++) {

                uint64_t trade_timestamp = duration_cast<milliseconds>(trade_it->trade_time.time_since_epoch()).count();

                if (trade_timestamp >= dataRect.x_end()) break;

                while (trade_timestamp >= next_bouy_timestamp) {
                    m_buoy_data.emplace_back(curBuoy);
                    if (last_trade_it != end)
                        curBuoy = {last_trade_it->price_points, last_trade_it->price_points, last_trade_it->price_points, 0};
                    else
                        curBuoy = {std::numeric_limits<uint64_t>::max(), 0, 0, 0};
                    next_bouy_timestamp += m_buoy_duration;
                }

                window_max_price = std::max(trade_it->price_points, window_max_price);
                window_min_price = std::min(trade_it->price_points, window_min_price);

                uint64_t sum_volume = curBuoy.volume + trade_it->volume_points;

                curBuoy.max = std::max(trade_it->price_points, curBuoy.max);
                curBuoy.min = std::min(trade_it->price_points, curBuoy.min);
                curBuoy.mean = (curBuoy.mean * curBuoy.volume + trade_it->price_points * trade_it->volume_points) / sum_volume;
                curBuoy.volume = sum_volume;
            }
            m_buoy_data.emplace_back(curBuoy);

            dataRect.y = window_min_price;
            dataRect.h = window_max_price - window_min_price;
        }
    }
};
}

#endif //BOUY_CANDLE_QUOTES_HPP
