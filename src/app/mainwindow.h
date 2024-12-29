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
}
class AsioScheduler;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<MarketWidget> mMarketView;
    std::shared_ptr<scratcher::bybit::ByBitApi> mMarketData;
    std::shared_ptr<scratcher::AsioScheduler> mScheduler;

public:
    MainWindow(std::shared_ptr<scratcher::bybit::ByBitApi> marketData, std::shared_ptr<scratcher::AsioScheduler> scheduler, QWidget *parent = nullptr);
    ~MainWindow() override;


};
#endif // MAINWINDOW_H
