// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "bybit.hpp"
#include "bybit/data_manager.hpp"

#include <chrono>

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::days;

typedef std::chrono::utc_clock::time_point time_point;


MainWindow::MainWindow(std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, std::shared_ptr<scratcher::AsioScheduler> scheduler, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , mMarketApi(std::move(marketApi))
    , mScheduler(std::move(scheduler))
{
    ui->setupUi(this);

    mMarketView = std::make_shared<scratcher::DataScratchWidget>(this);
    mMarketView->setMinimumSize(640, 480);

    mMarketData = scratcher::bybit::ByBitDataManager::Create("BTCUSDC", mMarketApi);
    mMarketViewController = scratcher::MarketController::Create(mMarketView, mMarketData);

    this->setCentralWidget(mMarketView.get());
}

MainWindow::~MainWindow()
{
}
