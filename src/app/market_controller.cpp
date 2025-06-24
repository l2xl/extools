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
#include <iomanip>
#include <ctime>

#include "market_controller.hpp"

#include <QDateTime>

#include "data_provider.hpp"
#include "scratch_widget.h"
#include "quote_scratcher.hpp"


namespace scratcher {


MarketViewController::MarketViewController(size_t id, std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider, std::shared_ptr<AsioScheduler> scheduler, EnsurePrivate)
    : ViewController(id), mWidget(widget), mDataProvider(dataProvider)
    , m_end_gap(minutes(10))
    , m_trade_group_time(seconds(60))
{
    auto now = std::chrono::utc_clock::now().time_since_epoch();
    uint64_t start = std::chrono::duration_cast<milliseconds>(now - minutes(20)).count();

    QDateTime qstart;
    qstart.setMSecsSinceEpoch(start);

    std::clog << "Window start TS: " << std::string(qstart.toString(widget->locale().dateFormat(QLocale::ShortFormat)).toUtf8().data()) << std::endl;

    uint64_t length = std::chrono::duration_cast<milliseconds>(minutes(30)).count();

    widget->DataViewRectChanged(Rectangle{start, std::numeric_limits<uint64_t>::max(), length, 1});

}

std::shared_ptr<MarketViewController> MarketViewController::Create(size_t id, std::shared_ptr<DataScratchWidget> widget, std::shared_ptr<IDataProvider> dataProvider, std::shared_ptr<AsioScheduler> scheduler)
{
    auto res = std::make_shared<MarketViewController>(id, widget, move(dataProvider), move(scheduler), EnsurePrivate{});

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

                widget->AddScratcher(self->mPriceRuler = std::make_shared<PriceRuler>(self->mDataProvider->PricePoint()));
                widget->AddScratcher(self->mQuoteGraph = std::make_shared<QuoteScratcher>(self->mDataProvider, self->m_trade_group_time), 0);

                widget->update();
            }
        }
    });
    res->mDataProvider->AddMarketDataUpdateHandler([ref]() {
        if (auto self = ref.lock()) {
            self->OnMarketDataUpdate();
        }
    });

    // if (widget->isVisible())
    //     widget->update();

    connect(res.get(), &MarketViewController::UpdateDataViewRect, widget.get(), &DataScratchWidget::DataViewRectChanged, Qt::ConnectionType::QueuedConnection);
    return res;
}

void MarketViewController::OnDataViewChange(uint64_t view_start, uint64_t view_end)
{
    if (auto w = mWidget.lock()) {
        std::clog << "OnDataViewChange(" << w->GetDataViewRect().x_start() << ", " << w->GetDataViewRect().x_end() << ")" << std::endl;


        if (!mDataProvider->PublicTradeCache().empty()) {
            view_end = duration_cast<milliseconds>(mDataProvider->PublicTradeCache().front().trade_time.time_since_epoch()).count();
        }

        if (view_start < view_end) {

        }
    }
}

void MarketViewController::OnMarketDataUpdate()
{
    // auto last_ts = duration_cast<milliseconds>(m_last_trade_time.time_since_epoch()).count();
    // if (last_ts < widget->GetDataViewRect().x_end()) {
    //     Rectangle rect = widget->GetDataViewRect();
        // {
        //     std::unique_lock lock(mDataProvider->PublicTradeMutex());
        //     m_last_trade_time = mDataProvider->PublicTradeCache().back().trade_time;
        // }
        // rect.x += duration_cast<milliseconds>(m_last_trade_time.time_since_epoch()).count() - last_ts;

}

void MarketViewController::Update()
{
    if (auto widget = mWidget.lock()) {
        time_point now = std::chrono::utc_clock::now();
        Rectangle rect = widget->GetDataViewRect();

        uint64_t new_end = duration_cast<milliseconds>(now.time_since_epoch() + m_end_gap).count();
        if (new_end > rect.x_end()) {
            rect.x = new_end - rect.w;

            if (mQuoteGraph)
                mQuoteGraph->CalculatePaint(rect);

            UpdateDataViewRect(rect);
        }
    }
}

}
