// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <memory>

#include "marketwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


namespace scratcher {
namespace bybit {
class ByBitApi;
class ByBitDataProvider;
}
class AsioScheduler;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    std::unique_ptr<Ui::MainWindow> ui;

    std::shared_ptr<scratcher::AsioScheduler> mScheduler;
    std::shared_ptr<scratcher::bybit::ByBitApi> mMarketApi;
    std::shared_ptr<scratcher::bybit::ByBitDataProvider> mMarketData;
    std::unique_ptr<MarketWidget> mMarketView;

public:
    MainWindow(std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, std::shared_ptr<scratcher::AsioScheduler> scheduler, QWidget *parent = nullptr);
    ~MainWindow() override;


};
#endif // MAINWINDOW_H
