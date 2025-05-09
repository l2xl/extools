// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef QUOTE_SCRATCHER_HPP
#define QUOTE_SCRATCHER_HPP

#include "data_provider.hpp"
#include "scratcher.hpp"

namespace scratcher {

class QuoteScratcher: public Scratcher {
    std::shared_ptr<const IDataProvider> mDataManager;
    mutable IDataProvider::pubtrade_cache_t::const_iterator m_first_shown_trade_it;
public:
    explicit QuoteScratcher(std::shared_ptr<const IDataProvider> dataManager)
        : mDataManager(move(dataManager))
        , m_first_shown_trade_it(mDataManager->PublicTradeCache().end()){}
    ~QuoteScratcher() override= default;

    void BeforePaint(DataScratchWidget& w) const override;
    void Paint(DataScratchWidget& w) const override;
};

}

#endif //QUOTE_SCRATCHER_HPP
