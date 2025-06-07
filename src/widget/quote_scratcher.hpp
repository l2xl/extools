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

#include "timedef.hpp"
#include "data_provider.hpp"
#include "scratcher.hpp"

namespace scratcher {

struct BuoyData
{
    uint64_t max;
    uint64_t min;

    uint64_t mean;
    uint64_t volume;
};

class QuoteScratcher: public Scratcher {
    std::shared_ptr<const IDataProvider> mDataManager;

    const uint64_t m_buoy_timelen;

    IDataProvider::pubtrade_cache_t::const_iterator m_first_shown_trade_it;

    uint64_t m_first_buoy_timestamp;
    std::deque<BuoyData> m_buoy_data;
public:
    explicit QuoteScratcher(std::shared_ptr<const IDataProvider> dataManager, duration group_time)
        : mDataManager(move(dataManager))
        , m_buoy_timelen(duration_cast<milliseconds>(group_time).count())
        , m_first_shown_trade_it(mDataManager->PublicTradeCache().end()){}
    ~QuoteScratcher() override= default;

    void Resize(DataScratchWidget& w) override {}
    void BeforePaint(DataScratchWidget& w) override;
    void Paint(DataScratchWidget& w) const override;
};

}

#endif //QUOTE_SCRATCHER_HPP
