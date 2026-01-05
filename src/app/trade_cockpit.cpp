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

#include <QVBoxLayout>

#include "trade_cockpit.h"
#include "ui_trade_cockpit.h"

#include "market_controller.hpp"
#include "content_frame.h"
#include "instrument_dropbox.h"

#include "config.hpp"
#include "bybit/data_manager.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

#include <chrono>

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::days;

typedef std::chrono::utc_clock::time_point time_point;

namespace this_coro =  boost::asio::this_coro;

TradeCockpitWindow::TradeCockpitWindow(std::shared_ptr<scratcher::scheduler> scheduler,
                                       std::shared_ptr<Config> config,
                                       std::shared_ptr<SQLite::Database> db,
                                       QWidget *parent, EnsurePrivate)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::TradeCockpitWindow>())
    , mScheduler(std::move(scheduler))
    , mMarketData(scratcher::bybit::ByBitDataManager::Create(mScheduler, std::move(config), std::move(db)))
{
    ui->setupUi(this);

    // Create the initial panel with market view
    size_t panel_id = m_next_panel_id++;
    auto contentFrame = createGridCell(*ui->gridLayout, 0, 0);
    contentFrame->setContent(createMarketView(panel_id));
    contentFrame.release();
}

std::shared_ptr<QWidget> TradeCockpitWindow::createMarketView(size_t panel_id)
{
    // Create container widget with vertical layout
    auto container = std::make_shared<QWidget>(nullptr);
    auto layout = new QVBoxLayout(container.get());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // Create instrument dropdown
    auto instrumentDropBox = std::make_shared<scratcher::InstrumentDropBox>(container.get());

    // Create market data widget
    auto marketWidget = std::make_shared<scratcher::DataScratchWidget>(container.get());
    marketWidget->setMinimumSize(640, 480);

    // Arrange in layout
    layout->addWidget(instrumentDropBox.get());
    layout->addWidget(marketWidget.get(), 1);  // stretch factor 1

    // Create and register controller
    mControllers[panel_id] = scratcher::MarketViewController::Create(panel_id, instrumentDropBox, marketWidget, mMarketData, mScheduler);

    // Subscribe to instrument updates and wire to dropbox
    std::weak_ptr<scratcher::InstrumentDropBox> dropboxRef = instrumentDropBox;
    mMarketData->SubscribeInstrumentList([dropboxRef](const std::deque<scratcher::bybit::InstrumentInfo>& instruments) {
        if (auto dropbox = dropboxRef.lock()) {
            auto list = std::make_shared<std::deque<scratcher::bybit::InstrumentInfo>>(instruments);
            QMetaObject::invokeMethod(dropbox.get(), [dropbox, list]() {
                dropbox->setInstruments(list);
            }, Qt::QueuedConnection);
        }
    });

    return container;
}


std::shared_ptr<TradeCockpitWindow> TradeCockpitWindow::Create(std::shared_ptr<scratcher::scheduler> scheduler,
                                                               std::shared_ptr<Config> config,
                                                               std::shared_ptr<SQLite::Database> db,
                                                               QWidget *parent)
{
    auto self = std::make_shared<TradeCockpitWindow>(scheduler, std::move(config), std::move(db), parent, EnsurePrivate{});
    self->show();

    co_spawn(self->mScheduler->io(), coUpdate(self), boost::asio::detached);

    return self;
}


boost::asio::awaitable<void> TradeCockpitWindow::coUpdate(std::weak_ptr<TradeCockpitWindow> ref)
{
    std::clog << "Enter update ==========================" << std::endl;
    while(true) {
        try {
            std::optional<boost::asio::steady_timer> timer;
            if (auto self = ref.lock()) {
                for (const auto& controller: self->mControllers){
                    controller.second->Update();
                }
                timer = std::make_optional(boost::asio::steady_timer(self->mScheduler->io(), milliseconds(1000)));
            }
            else {
                std::clog << "CockpitWindow destroyed?????????????????" << std::endl;
                break;
            }

            //TODO: Add check for cancellation happens before this point
            if (timer) {
                timer->expires_after(milliseconds(100));
                co_await timer->async_wait(boost::asio::use_awaitable);
            }
            else {
                std::clog << "Update timer is destroyed <<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
                break;
            }
        }
        catch (boost::system::error_code& e) {
            std::cerr << "System error: " << e.what() << std::endl;
            if (e.value() == boost::asio::error::operation_aborted) {
                std::clog << "Update timer is CANCELLED <<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
                break;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
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
    contentFrame->setTitle(QString::fromUtf8(mMarketData->Name()));
    contentFrame->setMinimumSize(QSize(640, 480));
    layout.addWidget(contentFrame.get());

    return contentFrame;
}

TradeCockpitWindow::~TradeCockpitWindow()
{
    //m_ui_update_timer->cancel();
}

