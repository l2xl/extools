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

#ifndef QUOTE_SCRATCHER_HPP
#define QUOTE_SCRATCHER_HPP

#include <atomic>
#include <QPainter>

#include "timedef.hpp"
#include "data_controller.hpp"
#include "scratcher.hpp"
#include "buoy_candle.hpp"

namespace scratcher {


class QuoteScratcher: public Scratcher {
    BuoyCandleQuotes mQuotes;

    std::shared_ptr<const IDataProvider> mDataManager;

    IDataProvider::pubtrade_cache_t::const_iterator m_first_shown_trade_it;
    uint64_t m_pixel_duration;

public:
    explicit QuoteScratcher(std::shared_ptr<const IDataProvider> dataManager, duration group_time)
        : mQuotes(duration_cast<milliseconds>(group_time).count())
        , mDataManager(move(dataManager))
        , m_first_shown_trade_it(mDataManager->PublicTradeCache().end())
        , m_pixel_duration(1)
    {}
    ~QuoteScratcher() override= default;

    BuoyCandleQuotes::candle_t GetActiveCandle() const
    { return mQuotes.active_candle(); }

    const BuoyCandleQuotes::quotes_t& GetQuotes() const
    { return mQuotes.quotes(); }

    void CalculateSize(DataScratchWidget& w) override;
    void CalculatePaint(Rectangle& rect) override;
    void Paint(DataScratchWidget& w) const override;

};

}

#endif //QUOTE_SCRATCHER_HPP
