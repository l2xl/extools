// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the Intellectual Property Reserve License (IPRL)
// -----BEGIN PGP PUBLIC KEY BLOCK-----
//
// mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
// lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b21tYWlsLmNvbT6IkAQTFggA
// OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
// AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
// 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
// =KKu7
// -----END PGP PUBLIC KEY BLOCK-----

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <deque>
#include <unordered_map>

#include "scheduler.hpp"
#include "bybit/data_manager.hpp"
#include "app/ui_builder.hpp"

class Config;

namespace SQLite {
class Database;
}

namespace scratcher::cockpit {

class TradeCockpit : public std::enable_shared_from_this<TradeCockpit>
{
public:
    using panel_id = ui::panel_id;

private:
    ui::IUiBuilder& mUiBuilder;
    std::shared_ptr<scheduler> mScheduler;
    std::shared_ptr<IDataController> mDataManager;

    panel_id mTabBarId = ui::INVALID_PANEL;
    panel_id mEmptyTabId = ui::INVALID_PANEL;

    std::unordered_map<panel_id, std::string> mTabSymbols;

    struct EnsurePrivate {};

    void OnInstrumentsLoaded(const std::deque<bybit::InstrumentInfo>& instruments);

public:
    TradeCockpit(ui::IUiBuilder& ui_builder, std::shared_ptr<scheduler> sched, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db, EnsurePrivate);

    static std::shared_ptr<TradeCockpit> Create(ui::IUiBuilder& ui_builder, std::shared_ptr<scheduler> sched, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db);

    std::shared_ptr<IDataController> DataController()
    { return mDataManager; }
};

} // namespace scratcher::cockpit
