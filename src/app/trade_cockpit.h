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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <QApplication>
#include <QPalette>
#include <QStyleHints>

#include <memory>
#include <boost/container/flat_map.hpp>
#include <boost/asio.hpp>

#include "widget/scratch_widget.h"
#include "market_controller.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class TradeCockpitWindow;
}
QT_END_NAMESPACE


namespace scratcher {
namespace bybit {
class ByBitApi;
class ByBitDataManager;
}
class AsioScheduler;
class ViewController;
}

class ContentFrameWidget;

class TradeCockpitWindow : public QMainWindow, public std::enable_shared_from_this<TradeCockpitWindow>
{
    Q_OBJECT
    std::unique_ptr<Ui::TradeCockpitWindow> ui;

    std::shared_ptr<scratcher::AsioScheduler> mScheduler;
    std::shared_ptr<boost::asio::steady_timer> m_ui_update_timer;


    std::shared_ptr<scratcher::bybit::ByBitApi> mMarketApi;
    std::shared_ptr<scratcher::bybit::ByBitDataManager> mMarketData;

    // Panels management
    size_t m_next_panel_id = 1;

    boost::container::flat_map<size_t, std::shared_ptr<scratcher::ViewController>> mControllers;
    std::mutex mControllersMutex;
private slots:

private:
    std::unique_ptr<ContentFrameWidget> createGridCell(QGridLayout& gridLayout, int row, int col);
    std::unique_ptr<ContentFrameWidget> createTab(QTabWidget& tabWidget);
    std::unique_ptr<ContentFrameWidget> createPanel(QLayout& layout);

    static boost::asio::awaitable<void> coUpdate(std::weak_ptr<TradeCockpitWindow>);

    struct EnsurePrivate{};

public:
    TradeCockpitWindow(std::shared_ptr<scratcher::AsioScheduler>, std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, QWidget *parent, EnsurePrivate);
    ~TradeCockpitWindow() override;

    static std::shared_ptr<TradeCockpitWindow> Create(std::shared_ptr<scratcher::AsioScheduler>, std::shared_ptr<scratcher::bybit::ByBitApi> marketApi, QWidget *parent = nullptr);


    inline bool isDarkMode() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        const auto scheme = QApplication::styleHints()->colorScheme();
        return scheme == Qt::ColorScheme::Dark;
#else
        const QPalette defaultPalette;
        const auto text = defaultPalette.color(QPalette::WindowText);
        const auto window = defaultPalette.color(QPalette::Window);
        return text.lightness() > window.lightness();
#endif // QT_VERSION
    }

};
#endif // MAINWINDOW_H
