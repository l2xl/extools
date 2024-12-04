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

class ByBitApi;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    std::unique_ptr<MarketWidget> mMarketView;

    std::shared_ptr<ByBitApi> mMarketData;

public:
    MainWindow(std::shared_ptr<ByBitApi> marketData, QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
