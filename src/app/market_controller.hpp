// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef MARKET_CONTROLLER_HPP
#define MARKET_CONTROLLER_HPP

#include <memory>

namespace scratcher {

class DataScratchWidget;
class IDataProvider;
class Scratcher;

class MarketController {
    std::weak_ptr<DataScratchWidget> mWidget;
    std::shared_ptr<IDataProvider> mDataProvider;

    std::shared_ptr<Scratcher> mPriceRuler;
    std::shared_ptr<Scratcher> mQuoteGraph;

    struct EnsurePrivate {};
public:
    MarketController(std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider, EnsurePrivate);

    static std::shared_ptr<MarketController> Create(std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider);
};

}


#endif //MARKET_CONTROLLER_HPP
