// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include <iostream>

#include "market_controller.hpp"

#include "data_provider.hpp"
#include "widget/scratch_widget.h"

namespace scratcher {

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::days;
using std::chrono::hours;


MarketController::MarketController(std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider, EnsurePrivate)
    : mWidget(widget), mDataProvider(dataProvider)
{
    auto now = time_point::clock::now().time_since_epoch();
    uint64_t start = std::chrono::duration_cast<milliseconds>(now).count();
    uint64_t length = std::chrono::duration_cast<milliseconds>(hours(1)).count();

    //widget->SetDataFieldRect(Rectangle{start,0,length,1000});
    widget->SetDataViewRect(Rectangle{start,250,length,500});

}

std::shared_ptr<MarketController> MarketController::Create(std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider)
{
    auto res = std::make_shared<MarketController>(widget, move(dataProvider), EnsurePrivate{});

    widget->AddScratcher(std::make_shared<TimeRuler>());

    std::weak_ptr ref = res;
    res->mDataProvider->AddInsctrumentDataUpdateHandler([ref]() {
        if (auto self = ref.lock()) {
            if (auto widget = self->mWidget.lock()) {
                if (self->mPriceRuler) widget->RemoveScratcher(self->mPriceRuler);
                if (self->mQuoteGraph) widget->RemoveScratcher(self->mQuoteGraph);

                widget->AddScratcher(self->mPriceRuler = self->mDataProvider->MakePriceRulerScratcher());
                widget->AddScratcher(self->mQuoteGraph = self->mDataProvider->MakeQuoteGraphScratcher());

                widget->update();
            }
        }
    });
    res->mDataProvider->AddMarketDataUpdateHandler([ref]() {
        if (auto self = ref.lock()) {
            if (auto widget = self->mWidget.lock()) {
                widget->update();
            }
        }
    });

    if (widget->isVisible())
        widget->update();

    return res;
}

}
