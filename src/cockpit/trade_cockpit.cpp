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

#include "trade_cockpit.hpp"
#include "config.hpp"

namespace scratcher::cockpit {

TradeCockpit::TradeCockpit(imgui::UiBuilder& ui_builder, std::shared_ptr<scheduler> sched, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db, EnsurePrivate)
    : mUiBuilder(ui_builder) , mScheduler(std::move(sched))
{
    mDataManager = bybit::ByBitDataManager::Create(mScheduler, config, db);

    // Create initial UI structure
    mTabBarId = mUiBuilder.RenderTabBar();
    mEmptyTabId = mUiBuilder.AddMarketTab(mTabBarId, std::nullopt);
}

std::shared_ptr<TradeCockpit> TradeCockpit::Create( imgui::UiBuilder& ui_builder, std::shared_ptr<scheduler> sched, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db)
{
    auto self = std::make_shared<TradeCockpit>(
        ui_builder, std::move(sched), std::move(config), std::move(db), EnsurePrivate{});

    self->mDataManager->SubscribeInstrumentList(
        [weak = std::weak_ptr(self)](const std::deque<bybit::InstrumentInfo>& instruments) {
            if (auto s = weak.lock()) {
                s->OnInstrumentsLoaded(instruments);
            }
        });

    return self;
}

void TradeCockpit::OnInstrumentsLoaded(const std::deque<bybit::InstrumentInfo>& instruments)
{
    imgui::UiBuilder::instrument_list list;
    list.reserve(instruments.size());

    for (const auto& inst : instruments) {
        list.emplace_back(inst.symbol);
    }

    mUiBuilder.SetInstruments(std::move(list));
}

} // namespace scratcher::cockpit
