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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <deque>

#include <boost/container/flat_map.hpp>

#include "scheduler.hpp"
#include "bybit/data_manager.hpp"
#include "content_panel.hpp"

class Config;

namespace SQLite {
class Database;
}

namespace scratcher::cockpit {

class TradeCockpit : public std::enable_shared_from_this<TradeCockpit>
{
private:
    std::shared_ptr<scheduler> mScheduler;
    std::shared_ptr<IDataController> mDataManager;

    std::vector<std::string> mInstruments;
    boost::container::flat_map<panel_id, std::shared_ptr<ContentPanel>> mPanels;
    bool mDataReady = false;
    panel_id mNextPanelId = 1;

    struct EnsurePrivate {};

    void OnInstrumentsLoaded(const std::deque<bybit::InstrumentInfo>& instruments);

public:
    TradeCockpit(std::shared_ptr<scheduler> sched, std::shared_ptr<Config> config,
                 std::shared_ptr<SQLite::Database> db, EnsurePrivate);

    static std::shared_ptr<TradeCockpit> Create(std::shared_ptr<scheduler> sched,
                                                 std::shared_ptr<Config> config,
                                                 std::shared_ptr<SQLite::Database> db);

    panel_id RegisterPanel(std::shared_ptr<ContentPanel> panel);
    void UnregisterPanel(panel_id pid);
};

} // namespace scratcher::cockpit
