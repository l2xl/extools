// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef QUOTE_SCRATCHER_HPP
#define QUOTE_SCRATCHER_HPP

#include "data_manager.hpp"
#include "widget/scratcher.hpp"

namespace scratcher::bybit {

class QuoteScratcher: public Scratcher {
    std::shared_ptr<const ByBitDataManager> mDataManager;
public:
    explicit QuoteScratcher(std::shared_ptr<const ByBitDataManager> dataManager) : mDataManager(move(dataManager)) {}
    ~QuoteScratcher() override= default;

    void BeforePaint(DataScratchWidget& w) const override;
    void Paint(DataScratchWidget& w) const override;
};

}

#endif //QUOTE_SCRATCHER_HPP
