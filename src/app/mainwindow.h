// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <memory>

#include "widget/scratch_widget.h"
#include "market_controller.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


namespace scratcher {
namespace bybit {
class ByBitApi;
class ByBitDataManager;
}
class AsioScheduler;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    std::unique_ptr<Ui::MainWindow> ui;

    std::shared_ptr<scratcher::AsioScheduler> mScheduler;
    std::shared_ptr<scratcher::bybit::ByBitApi> mMarketApi;
    std::shared_ptr<scratcher::MarketController> mMarketViewController;
    std::shared_ptr<scratcher::bybit::ByBitDataManager> mMarketData;
    std::shared_ptr<scratcher::DataScratchWidget> mMarketView;

public:
    MainWindow(std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, std::shared_ptr<scratcher::AsioScheduler> scheduler, QWidget *parent = nullptr);
    ~MainWindow() override;


};
#endif // MAINWINDOW_H
