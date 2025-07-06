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

#include <algorithm>
#include <iostream>
#include <limits>

#include "quote_scratcher.hpp"

#include <mutex>

#include "scratch_widget.h"

namespace scratcher {

void QuoteScratcher::CalculateSize(DataScratchWidget &w)
{
    m_pixel_duration = static_cast<uint64_t>(std::floor(w.GetDataViewRect().w / w.GetClientRect().width())) + 1;
}

void QuoteScratcher::CalculatePaint(Rectangle& rect)
{
    if (!mDataManager->PublicTradeCache().empty()) {
        uint64_t cache_first_ts = get_timestamp(mDataManager->PublicTradeCache().front().trade_time);
        uint64_t cache_first_buoy_ts = cache_first_ts - cache_first_ts % mQuotes.buoy_duration();
        uint64_t rect_first_buoy_ts = rect.x_start() - rect.x_start() % mQuotes.buoy_duration();
        uint64_t start_ts = std::max(cache_first_buoy_ts, rect_first_buoy_ts);
        auto start = (start_ts == cache_first_buoy_ts)
                ? mDataManager->PublicTradeCache().begin()
                : std::lower_bound(mDataManager->PublicTradeCache().begin(), mDataManager->PublicTradeCache().end(), start_ts,
                                    [](const auto& t, uint64_t x) { return get_timestamp(t.trade_time) < x; });

        uint64_t cache_last_ts = get_timestamp(mDataManager->PublicTradeCache().back().trade_time);
        uint64_t cache_last_buoy_ts = cache_last_ts - cache_last_ts % mQuotes.buoy_duration() + mQuotes.buoy_duration();
        uint64_t rect_last_buoy_ts = rect.x_end() - rect.x_end() % mQuotes.buoy_duration();
        uint64_t end_ts = std::min(cache_last_buoy_ts, rect_last_buoy_ts);
        auto end = std::lower_bound(start, mDataManager->PublicTradeCache().end(), end_ts,
            [](const auto& t, uint64_t x) { return get_timestamp(t.trade_time) < x; });

        if (start_ts < mQuotes.first_trade_timestamp().value_or(0)) {
            mQuotes.Reset();
        }
        else if (mQuotes.last_trade_timestamp()){
            start = std::upper_bound(start, end, *mQuotes.last_trade_timestamp(),
                [](uint64_t x, const auto& t) { return x < get_timestamp(t.trade_time); });
        }

        uint64_t last_price = 0;
        if (start != mDataManager->PublicTradeCache().end()) {
            if (start != mDataManager->PublicTradeCache().begin())
                last_price = (start-1)->price_points;
            else
                last_price = start->price_points;
        }

        uint64_t now_ts = get_timestamp(std::chrono::utc_clock::now());
        mQuotes.AppendTrades(std::ranges::subrange(start, end), now_ts, last_price);

        uint64_t max = 0;
        uint64_t min = std::numeric_limits<uint64_t>::max();
        for (const auto& buoy: mQuotes.quotes()) {
            max = std::max(max, buoy.max);
            min = std::min(min, buoy.min);
        }
        max = std::max(max, mQuotes.active_candle().max);
        min = std::min(min, mQuotes.active_candle().min);

        rect.y = min;
        rect.h = max - min;
//        std::clog << "Price range [" << rect.y_start() << ", " << rect.y_end() << "]" << std::endl;
    }
}


namespace {

void PaintBuoy(const BuoyCandleQuotes& quotes, uint64_t buoy_ts, const auto& buoy, auto prev_buoy, DataScratchWidget &w)
{
    QPainter p(&w);
    QPen greenPen(Qt::green, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    QPen redPen(Qt::red, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    uint64_t mean_time = buoy_ts + quotes.buoy_duration()/2;
    QPoint maxPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(buoy.max));
    QPoint meanPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(buoy.mean));
    QPoint minPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(buoy.min));

    p.setPen(greenPen);
    p.setBrush(Qt::green);

    if (prev_buoy.max <= buoy.max)
        p.drawLine(maxPt, meanPt);
    if (prev_buoy.min <= buoy.min)
        p.drawLine(minPt, meanPt);

    p.setPen(redPen);
    p.setBrush(Qt::red);

    if (prev_buoy.max > buoy.max)
        p.drawLine(maxPt, meanPt);
    if (prev_buoy.min > buoy.min)
        p.drawLine(minPt, meanPt);

    if (prev_buoy.mean <= buoy.mean) {
        p.setPen(greenPen);
        p.setBrush(Qt::green);
    }
    p.drawLine(QPoint(w.DataXToWidgetX(buoy_ts), w.DataYToWidgetY(buoy.mean)), QPoint(w.DataXToWidgetX(buoy_ts+quotes.buoy_duration()-1), w.DataYToWidgetY(buoy.mean)));
}

}

void QuoteScratcher::Paint(DataScratchWidget &w) const
{
    QPainter p(&w);
    QPen greenPen(Qt::green, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    QPen redPen(Qt::red, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    if (mQuotes.first_buoy_timestamp()) {
        uint64_t buoy_time = *mQuotes.first_buoy_timestamp();
        auto last_buoy_it = mQuotes.quotes().end();
        for (auto buoy_it = mQuotes.quotes().begin(); buoy_it != mQuotes.quotes().end(); ++buoy_it) {
            if (buoy_time >= w.GetDataViewRect().x_start()) {
                if (buoy_time + mQuotes.buoy_duration() > w.GetDataViewRect().x_end()) break;

                PaintBuoy(mQuotes, buoy_time, *buoy_it, last_buoy_it == mQuotes.quotes().end() ? *buoy_it : *last_buoy_it, w);
            }

            buoy_time += mQuotes.buoy_duration();
            last_buoy_it = buoy_it;
        }

        if (buoy_time >= w.GetDataViewRect().x_start() && buoy_time + mQuotes.buoy_duration() < w.GetDataViewRect().x_end()) {
            BuoyCandleQuotes::candle_t activeCandle = mQuotes.active_candle();
            if (mQuotes.quotes().empty())
                PaintBuoy(mQuotes, buoy_time, activeCandle, activeCandle , w);
            else
                PaintBuoy(mQuotes, buoy_time, activeCandle, mQuotes.quotes().back(), w);
        }
    }
}


}
