// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include <algorithm>
#include <iostream>

#include "quote_scratcher.hpp"

#include <mutex>

#include "scratch_widget.h"

namespace scratcher {

void QuoteScratcher::BeforePaint(DataScratchWidget &w) const
{
    if (!(w.forceScale || mDataManager->PublicTradeCache().empty())) {
        std::unique_lock lock(mDataManager->PublicTradeMutex());

        m_first_shown_trade_it = std::lower_bound(mDataManager->PublicTradeCache().begin(), mDataManager->PublicTradeCache().end(),
            w.GetDataViewRect().x_start(), [=](const Trade& t, const uint64_t& x) {
                return duration_cast<milliseconds>(t.trade_time.time_since_epoch()).count() < x;
            });

        if (m_first_shown_trade_it != mDataManager->PublicTradeCache().end()) {
            uint64_t max_price = 0;
            uint64_t min_price = std::numeric_limits<uint64_t>::max();
            for(auto trade_it = m_first_shown_trade_it; trade_it != mDataManager->PublicTradeCache().end(); ++trade_it) {
                if (duration_cast<milliseconds>(trade_it->trade_time.time_since_epoch()).count() > w.GetDataViewRect().x_end()) break;

                if (trade_it->price_points > max_price) max_price = trade_it->price_points;
                if (trade_it->price_points < min_price) min_price = trade_it->price_points;
            }

            w.mDataViewRect.y = min_price;
            w.mDataViewRect.h = max_price - min_price;

            currency<uint64_t> min = mDataManager->PricePoint();
            min.set_raw(min_price);

            currency<uint64_t> max = mDataManager->PricePoint();
            max.set_raw(max_price);

            std::clog << "Price measure: [" << min.to_string() << ", " << max.to_string() << "]" << std::endl;
        }
    }
}

void QuoteScratcher::Paint(DataScratchWidget &w) const
{
    QPainter p(&w);
    QPen greenPen(Qt::green, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    QPen redPen(Qt::red, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    std::unique_lock <std::mutex> lock(mDataManager->PublicTradeMutex());

    QPoint prev;
    bool first = true;

    for(auto trade_it = m_first_shown_trade_it; trade_it != mDataManager->PublicTradeCache().end(); ++trade_it) {
        auto t = trade_it->trade_time.time_since_epoch();
        if (duration_cast<milliseconds>(t).count() > w.GetDataViewRect().x_end()) break;

        QPoint cur(w.DataXToWidgetX(duration_cast<milliseconds>(t).count()), w.DataYToWidgetY(trade_it->price_points));

        if (!first) {
            if (prev.y() >= cur.y()) {
                p.setPen(greenPen);
                p.setBrush(Qt::green);
            }
            else {
                p.setPen(redPen);
                p.setBrush(Qt::red);
            }

            p.drawLine(prev, cur);
        }

        first = false;
        prev = cur;
    }

}

}
