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

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "bybit.hpp"
#include "bybit/data_manager.hpp"

#include <chrono>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QDebug>

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::days;

typedef std::chrono::utc_clock::time_point time_point;


MainWindow::MainWindow(std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , mMarketApi(std::move(marketApi))
{
    ui->setupUi(this);
    
    // Create the market view
    mMarketView = std::make_shared<scratcher::DataScratchWidget>(nullptr);
    mMarketView->setMinimumSize(640, 480);
    
    // Initialize market data and controller
    mMarketData = scratcher::bybit::ByBitDataManager::Create("BTCUSDC", mMarketApi);
    mMarketViewController = scratcher::MarketController::Create(mMarketView, mMarketData);
    
    // Create the initial panel with the market view
    createInitialPanel();
}


void MainWindow::createInitialPanel()
{
    // Set the title for the market data frame
    ui->marketDataFrame->setTitle(QString::fromUtf8(mMarketData->Symbol()));
    
    // Create a container widget for the market view
    auto container = std::make_unique<QFrame>();
    auto containerLayout = new QVBoxLayout(container.get());
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(mMarketView.get());

    // Set the market view as the content of the frame
    ui->marketDataFrame->setContent(std::move(container));
}

MainWindow::~MainWindow()
{
    // The unique_ptr will automatically clean up the UI and panel widgets
}
