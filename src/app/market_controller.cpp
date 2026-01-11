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

#include "data_controller.hpp"
#include "scratch_widget.h"
#include "quote_scratcher.hpp"


namespace scratcher {


MarketViewController::MarketViewController(size_t id,
                                           std::shared_ptr<InstrumentDropBox> instrumentDropBox,
                                           std::shared_ptr<DataScratchWidget> marketWidget,
                                           std::shared_ptr<IDataController> dataController,
                                           std::shared_ptr<scheduler> scheduler,
                                           EnsurePrivate)
    : ViewController(id)
    , mInstrumentDropBox(instrumentDropBox)
    , mMarketWidget(marketWidget)
    , mDataController(std::move(dataController))
    , mScheduler(std::move(scheduler))
    , m_end_gap(minutes(10))
    , m_trade_group_time(seconds(60))
{
    auto now = std::chrono::utc_clock::now().time_since_epoch();
    uint64_t start = std::chrono::duration_cast<milliseconds>(now - minutes(50)).count();
    uint64_t length = std::chrono::duration_cast<milliseconds>(minutes(60)).count();

    marketWidget->DataViewRectChanged(Rectangle{start, std::numeric_limits<uint64_t>::max(), length, 1});
}

std::shared_ptr<MarketViewController> MarketViewController::Create(
    size_t id,
    std::shared_ptr<InstrumentDropBox> instrumentDropBox,
    std::shared_ptr<DataScratchWidget> marketWidget,
    std::shared_ptr<IDataController> dataController,
    std::shared_ptr<scheduler> scheduler)
{
    auto res = std::make_shared<MarketViewController>(
        id, instrumentDropBox, marketWidget,
        std::move(dataController), std::move(scheduler), EnsurePrivate{});

    // Setup base scratchers for market widget
    marketWidget->AddScratcher(std::make_shared<TimeRuler>());
    marketWidget->AddScratcher(std::make_shared<Margin>(0.05));

    std::weak_ptr ref = res;

    // Wire InstrumentDropBox selection to controller
    connect(instrumentDropBox.get(), &InstrumentDropBox::instrumentSelected,
            res.get(), &MarketViewController::onInstrumentSelected);

    // Listen for market widget view changes
    marketWidget->AddDataViewChangeListener([ref](DataScratchWidget &w) {
        if (auto self = ref.lock()) {
            const auto& dataView = w.GetDataViewRect();
            self->OnDataViewChange(dataView.x_start(), dataView.x_end());
        }
    });

    // Handle instrument data ready (metadata loaded)
    res->mDataController->AddInsctrumentDataHandler([ref](const std::string& symbol, SourceType source) {
        if (auto self = ref.lock()) {
            if (self->m_symbol && *self->m_symbol == symbol) {
                self->setupMarketScratchers();
            }
        }
    });

    // Handle new trade data
    res->mDataController->AddNewTradeHandler([ref](const std::string& symbol, SourceType source) {
        if (auto self = ref.lock()) {
            if (self->m_symbol && *self->m_symbol == symbol) {
                self->OnMarketDataUpdate();
            }
        }
    });

    // Wire controller signal to market widget
    connect(res.get(), &MarketViewController::UpdateDataViewRect,
            marketWidget.get(), &DataScratchWidget::DataViewRectChanged,
            Qt::ConnectionType::QueuedConnection);

    return res;
}

void MarketViewController::onInstrumentSelected(instrument_ptr instrument)
{
    if (!instrument) return;

    m_instrument = std::move(instrument);
    m_symbol = m_instrument->symbol;

    std::clog << "Instrument selected: " << *m_symbol << std::endl;

    // Clear existing scratchers for previous symbol
    if (auto widget = mMarketWidget.lock()) {
        if (mPriceRuler) widget->RemoveScratcher(mPriceRuler);
        if (mPriceIndicator) widget->RemoveScratcher(mPriceIndicator);
        if (mQuoteGraph) widget->RemoveScratcher(mQuoteGraph);
        mPriceRuler.reset();
        mPriceIndicator.reset();
        mQuoteGraph.reset();
    }

    // Check if data provider already has data ready
    auto dataProvider = mDataController->GetDataProvider(*m_symbol);
    if (dataProvider && dataProvider->IsReadyHandleData()) {
        setupMarketScratchers();
    }
    // Otherwise, wait for AddInsctrumentDataHandler callback
}

void MarketViewController::setupMarketScratchers()
{
    if (!m_symbol) return;

    auto widget = mMarketWidget.lock();
    if (!widget) return;

    auto dataProvider = mDataController->GetDataProvider(*m_symbol);
    if (!dataProvider || !dataProvider->IsReadyHandleData()) return;

    // Remove old scratchers if any
    if (mPriceRuler) widget->RemoveScratcher(mPriceRuler);
    if (mPriceIndicator) widget->RemoveScratcher(mPriceIndicator);
    if (mQuoteGraph) widget->RemoveScratcher(mQuoteGraph);

    // Add new scratchers for current symbol
    widget->AddScratcher(mPriceRuler = std::make_shared<PriceRuler>(dataProvider->PricePoint()));
    widget->AddScratcher(mPriceIndicator = std::make_shared<PriceIndicator>(dataProvider));
    widget->AddQuoteScratcher(mQuoteGraph = std::make_shared<QuoteScratcher>(dataProvider, m_trade_group_time));

    widget->update();
}

void MarketViewController::OnDataViewChange(uint64_t view_start, uint64_t view_end)
{
    if (auto w = mMarketWidget.lock()) {
        std::clog << "OnDataViewChange(" << w->GetDataViewRect().x_start() << ", " << w->GetDataViewRect().x_end() << ")" << std::endl;
    }
}

void MarketViewController::OnMarketDataUpdate()
{
    // Trigger widget update when new trade data arrives
    if (auto widget = mMarketWidget.lock()) {
        widget->update();
    }
}

void MarketViewController::Update()
{
    if (auto widget = mMarketWidget.lock()) {
        time_point now = std::chrono::utc_clock::now();
        Rectangle rect = widget->GetDataViewRect();

        uint64_t new_end = duration_cast<milliseconds>(now.time_since_epoch() + m_end_gap).count();
        if (new_end > rect.x_end()) {
            rect.x = new_end - rect.w;

            for (auto& s: widget->Scratchers()) {
                s->CalculatePaint(rect);
            }

            emit UpdateDataViewRect(rect);
        }
    }
}

}
