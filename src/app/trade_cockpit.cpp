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

#include <boost/asio/co_spawn.hpp>

#include "trade_cockpit.h"
#include "ui_trade_cockpit.h"

#include "market_controller.hpp"
#include "content_frame.h"

#include "bybit.hpp"
#include "bybit/data_manager.hpp"

#include <chrono>

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::days;

typedef std::chrono::utc_clock::time_point time_point;

namespace this_coro =  boost::asio::this_coro;

TradeCockpitWindow::TradeCockpitWindow(std::shared_ptr<scratcher::AsioScheduler> scheduler, std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, QWidget *parent, EnsurePrivate)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::TradeCockpitWindow>())
    , mScheduler(std::move(scheduler)), m_ui_update_timer(std::make_shared<boost::asio::steady_timer>(mScheduler->io(), milliseconds(100)))
    , mMarketApi(std::move(marketApi))
{
    ui->setupUi(this);

    // Create the market view
    auto marketView = std::make_shared<scratcher::DataScratchWidget>(nullptr);
    marketView->setMinimumSize(640, 480);
    
    // Initialize market data and controller
    mMarketData = scratcher::bybit::ByBitDataManager::Create("BTCUSDC", mMarketApi);

    // Create the initial panel with the market view
    auto contentFrame= createGridCell(*ui->gridLayout, 0, 0);

    contentFrame->setContent(marketView);
    contentFrame.release();

    size_t panel_id = m_next_panel_id++;
    mControllers[panel_id] = std::static_pointer_cast<scratcher::ViewController>(scratcher::MarketViewController::Create(panel_id, marketView, mMarketData, std::move(scheduler)));

}


std::shared_ptr<TradeCockpitWindow> TradeCockpitWindow::Create(std::shared_ptr<scratcher::AsioScheduler> scheduler, std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, QWidget *parent)
{
    auto self = std::make_shared<TradeCockpitWindow>(scheduler, marketApi, parent, EnsurePrivate{});
    self->show();

    co_spawn(self->mScheduler->io(), coUpdate(self), boost::asio::detached);

    return self;
}


boost::asio::awaitable<void> TradeCockpitWindow::coUpdate(std::weak_ptr<TradeCockpitWindow> ref)
{
    while(true) {
        try {
            std::shared_ptr<boost::asio::steady_timer> timer;
            if (auto self = ref.lock()) {
                timer = self->m_ui_update_timer;
                for (const auto& controller: self->mControllers){
                    controller.second->Update();
                }
            }
            else break;

            //TODO: Add check for cancellation happens before this point
            co_await timer->async_wait(boost::asio::use_awaitable);
        }
        catch (boost::system::error_code& e) {
            if (e.value() == boost::asio::error::operation_aborted) break;
        }
        catch (...){}
    }
}

std::unique_ptr<ContentFrameWidget> TradeCockpitWindow::createGridCell(QGridLayout& gridLayout, int row, int col)
{
    auto tabWidget = std::make_unique<QTabWidget>();

    auto contentFrame = createTab(*tabWidget);
    tabWidget->setCurrentIndex(0);

    gridLayout.addWidget(tabWidget.release(), row, col);

    return contentFrame;
}

std::unique_ptr<ContentFrameWidget> TradeCockpitWindow::createTab(QTabWidget& tabWidget)
{
    auto tab = std::make_unique<QWidget>();
    auto layout = std::make_unique<QGridLayout>(tab.get());
    layout->setContentsMargins(0, 0, 0, 0); // Remove default margins

    auto contentFrame = createPanel(*layout);

    tabWidget.addTab(tab.release(), "Market Data");
    layout.release();

    return contentFrame;
}

std::unique_ptr<ContentFrameWidget> TradeCockpitWindow::createPanel(QLayout& layout)
{
    auto contentFrame = std::make_unique<ContentFrameWidget>();
    contentFrame->setTitle(QString::fromUtf8(mMarketData->Symbol()));
    contentFrame->setMinimumSize(QSize(640, 480));
    layout.addWidget(contentFrame.get());

    return contentFrame;
}

TradeCockpitWindow::~TradeCockpitWindow()
{
    m_ui_update_timer->cancel();
}

