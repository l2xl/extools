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
    if (!mDataManager->PublicTradeCache().empty()) {
        std::unique_lock lock(mDataManager->PublicTradeMutex());

        mQuotes.PrepairData(w.mDataViewRect, mDataManager->PublicTradeCache());
    }
}

void QuoteScratcher::Paint(DataScratchWidget &w) const
{
    QPainter p(&w);
    QPen greenPen(Qt::green, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
    QPen redPen(Qt::red, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    uint64_t bouy_time = mQuotes.first_timestamp();
    auto last_bouy_it = mQuotes.buoy_data().end();
    for (auto bouy_it = mQuotes.buoy_data().begin(); bouy_it != mQuotes.buoy_data().end(); ++bouy_it) {
        if (bouy_time > w.GetDataViewRect().x_start()) {
            if (bouy_time + mQuotes.buoy_duration() > w.GetDataViewRect().x_end()) break;

            uint64_t mean_time = bouy_time + mQuotes.buoy_duration()/2;
            QPoint maxPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(bouy_it->max));
            QPoint meanPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(bouy_it->mean));
            QPoint minPt(w.DataXToWidgetX(mean_time), w.DataYToWidgetY(bouy_it->min));

            p.setPen(greenPen);
            p.setBrush(Qt::green);

            if (last_bouy_it == mQuotes.buoy_data().end() || last_bouy_it->max <= bouy_it->max)
                p.drawLine(maxPt, meanPt);
            if (last_bouy_it == mQuotes.buoy_data().end() || last_bouy_it->min <= bouy_it->min)
                p.drawLine(minPt, meanPt);

            p.setPen(redPen);
            p.setBrush(Qt::red);

            if (last_bouy_it != mQuotes.buoy_data().end() && last_bouy_it->max > bouy_it->max)
                p.drawLine(maxPt, meanPt);
            if (last_bouy_it != mQuotes.buoy_data().end() && last_bouy_it->min > bouy_it->min)
                p.drawLine(minPt, meanPt);

            if (last_bouy_it == mQuotes.buoy_data().end() || last_bouy_it->mean <= bouy_it->mean) {
                p.setPen(greenPen);
                p.setBrush(Qt::green);
            }
            p.drawLine(QPoint(w.DataXToWidgetX(bouy_time), w.DataYToWidgetY(bouy_it->mean)), QPoint(w.DataXToWidgetX(bouy_time+mQuotes.buoy_duration()-1), w.DataYToWidgetY(bouy_it->mean)));
        }

        bouy_time += mQuotes.buoy_duration();
        last_bouy_it = bouy_it;
    }
}

}
