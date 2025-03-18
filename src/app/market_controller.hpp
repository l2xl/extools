// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#ifndef MARKET_CONTROLLER_HPP
#define MARKET_CONTROLLER_HPP

#include <memory>

namespace scratcher {

class MarketWidget;
class DataProvider;

class MarketController {
    std::weak_ptr<MarketWidget> mWidget;
    std::shared_ptr<DataProvider> mDataProvider;
public:
    MarketController(std::shared_ptr<MarketWidget> widget, std::shared_ptr<DataProvider> dataProvider)
        : mWidget(move(widget)), mDataProvider(dataProvider)
    {}
};

}


#endif //MARKET_CONTROLLER_HPP
