// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "bybit.hpp"

#include <chrono>

MainWindow::MainWindow(std::shared_ptr<bybit::ByBitApi> marketData, std::shared_ptr<scratcher::AsioScheduler> scheduler, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , mMarketData(std::move(marketData))
    , mScheduler(std::move(scheduler))
{
    ui->setupUi(this);

    mMarketView = std::make_unique<MarketWidget>(this);
    this->setCentralWidget(mMarketView.get());

    const static std::deque<std::array<double, 4>> sample_data {
        {75,100,50,90},
        {90,50,100,60},
        {60,55,95,65},
        {65,65,90,85},
        {85,85,55,55},
        {53,55,30,45}};

    mMarketView->SetMarketData(sample_data);

    auto start_time = std::chrono::utc_clock::now() - std::chrono::seconds(60) * 50;

    mMarketData->Subscribe({}, "BTCUSDC", start_time, std::chrono::seconds(60), 50);

}

MainWindow::~MainWindow()
{
}
