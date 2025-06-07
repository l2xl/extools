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

#include <iostream>

#include "market_controller.hpp"

#include <mutex>

#include "data_provider.hpp"
#include "widget/scratch_widget.h"

namespace scratcher {


MarketViewController::MarketViewController(size_t id, std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider, EnsurePrivate)
    : ViewController(id), mWidget(widget), mDataProvider(dataProvider), m_last_trade_time(time_point::clock::now())
{
    auto now = m_last_trade_time.time_since_epoch();
    uint64_t start = std::chrono::duration_cast<milliseconds>(now - minutes(20)).count();
    uint64_t length = std::chrono::duration_cast<milliseconds>(minutes(30)).count();

    widget->SetDataViewRect(Rectangle{start, 0, length, 1});

}

std::shared_ptr<MarketViewController> MarketViewController::Create(size_t id, std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider)
{
    auto res = std::make_shared<MarketViewController>(id, widget, move(dataProvider), EnsurePrivate{});

    widget->AddScratcher(std::make_shared<TimeRuler>());

    std::weak_ptr ref = res;

    widget->AddDataViewChangeListener([ref](DataScratchWidget &w) {
        if (auto self = ref.lock()) {
            const auto& dataView = w.GetDataViewRect();
            self->OnDataViewChange(dataView.x_start(), dataView.x_end());
        }
    });

    res->mDataProvider->AddInsctrumentDataUpdateHandler([ref]() {
        if (auto self = ref.lock()) {
            if (auto widget = self->mWidget.lock()) {
                if (self->mPriceRuler) widget->RemoveScratcher(self->mPriceRuler);
                if (self->mQuoteGraph) widget->RemoveScratcher(self->mQuoteGraph);

                widget->AddScratcher(self->mPriceRuler = self->mDataProvider->MakePriceRulerScratcher());
                widget->AddScratcher(self->mQuoteGraph = self->mDataProvider->MakeQuoteGraphScratcher(self->m_trade_group_time), 0);

                widget->update();
            }
        }
    });
    res->mDataProvider->AddMarketDataUpdateHandler([ref]() {
        if (auto self = ref.lock()) {
            self->OnMarketDataUpdate();
        }
    });

    if (widget->isVisible())
        widget->update();

    return res;
}

void MarketViewController::OnDataViewChange(uint64_t view_start, uint64_t view_end)
{
    if (auto w = mWidget.lock()) {
        std::clog << "OnDataViewChange(" << w->GetDataViewRect().x_start() << ", " << w->GetDataViewRect().x_end() << ")" << std::endl;


        if (!mDataProvider->PublicTradeCache().empty()) {
            std::unique_lock lock(mDataProvider->PublicTradeMutex());
            view_end = duration_cast<milliseconds>(mDataProvider->PublicTradeCache().front().trade_time.time_since_epoch()).count();
        }

        if (view_start < view_end) {

        }
    }
}

void MarketViewController::OnMarketDataUpdate()
{
    if (auto widget = mWidget.lock()) {
        //if (self->mDataProvider->PublicTradeCache().back().trade_time)
        widget->update();
    }

}

}
