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

#include "quote_scratcher.hpp"

#include <mutex>

#include "scratch_widget.h"

namespace scratcher {

void QuoteScratcher::BeforePaint(DataScratchWidget &w)
{
    m_buoy_data.clear();

    if (!mDataManager->PublicTradeCache().empty()) {
        std::unique_lock lock(mDataManager->PublicTradeMutex());

        m_first_shown_trade_it = std::lower_bound(mDataManager->PublicTradeCache().begin(), mDataManager->PublicTradeCache().end(),
            w.GetDataViewRect().x_start() - m_buoy_timelen, [=](const Trade& t, const uint64_t& x) {
                return duration_cast<milliseconds>(t.trade_time.time_since_epoch()).count() < x;
            });

        if (m_first_shown_trade_it != mDataManager->PublicTradeCache().end()) {

            uint64_t first_trade_timestamp = duration_cast<milliseconds>(m_first_shown_trade_it->trade_time.time_since_epoch()).count();
            m_first_buoy_timestamp =  first_trade_timestamp - (first_trade_timestamp % m_buoy_timelen);
            uint64_t next_bouy_timestamp = m_first_buoy_timestamp + m_buoy_timelen;

            uint64_t window_max_price = 0;
            uint64_t window_min_price = std::numeric_limits<uint64_t>::max();

            BuoyData curBuoy = {0, std::numeric_limits<uint64_t>::max(), 0, 0};

            for (auto trade_it = m_first_shown_trade_it; trade_it != mDataManager->PublicTradeCache().end(); ++trade_it) {

                uint64_t trade_timestamp = duration_cast<milliseconds>(trade_it->trade_time.time_since_epoch()).count();

                while (trade_timestamp > next_bouy_timestamp) {
                    m_buoy_data.emplace_back(curBuoy);
                    curBuoy = {trade_it->price_points, trade_it->price_points, trade_it->price_points, 0};
                    next_bouy_timestamp += m_buoy_timelen;
                }

                if (trade_timestamp > w.GetDataViewRect().x_end()) break;

                window_max_price = std::max(trade_it->price_points, window_max_price);
                window_min_price = std::min(trade_it->price_points, window_min_price);

                uint64_t sum_volume = curBuoy.volume + trade_it->volume_points;

                curBuoy.max = std::max(trade_it->price_points, curBuoy.max);
                curBuoy.min = std::min(trade_it->price_points, curBuoy.min);
                curBuoy.mean = (curBuoy.mean * curBuoy.volume + trade_it->price_points * trade_it->volume_points) / sum_volume;
                curBuoy.volume = sum_volume;

                assert(curBuoy.max >= curBuoy.mean);
                assert(curBuoy.min <= curBuoy.mean);
            }

            w.mDataViewRect.y = window_min_price;
            w.mDataViewRect.h = window_max_price - window_min_price;

            currency<uint64_t> min = mDataManager->PricePoint();
            min.set_raw(window_min_price);

            currency<uint64_t> max = mDataManager->PricePoint();
            max.set_raw(window_max_price);

            std::clog << "Price measure: [" << min.to_string() << ", " << max.to_string() << "]" << std::endl;
        }
    }
}

void QuoteScratcher::Paint(DataScratchWidget &w) const
{
    QPainter p(&w);
    QPen greenPen(Qt::green, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    QPen redPen(Qt::red, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    uint64_t bouy_time = m_first_buoy_timestamp;
    auto last_bouy_it = m_buoy_data.end();
    for (auto bouy_it = m_buoy_data.begin(); bouy_it != m_buoy_data.end(); ++bouy_it) {
        if (bouy_time > w.GetDataViewRect().x_start()) {
            if (bouy_time + m_buoy_timelen > w.GetDataViewRect().x_end()) break;

            uint64_t mean_time = bouy_time + m_buoy_timelen/2;
            QPoint maxPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(bouy_it->max));
            QPoint meanPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(bouy_it->mean));
            QPoint minPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(bouy_it->min));

            p.setPen(greenPen);
            p.setBrush(Qt::green);

            if (last_bouy_it == m_buoy_data.end() || last_bouy_it->max <= bouy_it->max)
                p.drawLine(maxPt, meanPt);
            if (last_bouy_it == m_buoy_data.end() || last_bouy_it->min <= bouy_it->min)
                p.drawLine(minPt, meanPt);

            p.setPen(redPen);
            p.setBrush(Qt::red);

            if (last_bouy_it != m_buoy_data.end() && last_bouy_it->max > bouy_it->max)
                p.drawLine(maxPt, meanPt);
            if (last_bouy_it != m_buoy_data.end() && last_bouy_it->min > bouy_it->min)
                p.drawLine(minPt, meanPt);

            if (last_bouy_it == m_buoy_data.end() || last_bouy_it->mean <= bouy_it->mean) {
                p.setPen(greenPen);
                p.setBrush(Qt::green);
            }
            p.drawLine(QPoint(w.DataXToWidgetX(bouy_time), w.DataYToWidgetY(bouy_it->mean)), QPoint(w.DataXToWidgetX(bouy_time+m_buoy_timelen-1), w.DataYToWidgetY(bouy_it->mean)));
        }

        bouy_time += m_buoy_timelen;
        last_bouy_it = bouy_it;
    }
}

}
