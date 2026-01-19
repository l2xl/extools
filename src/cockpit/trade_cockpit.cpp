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

TradeCockpit::TradeCockpit(ui::IUiBuilder& ui_builder, std::shared_ptr<scheduler> sched, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db, EnsurePrivate)
    : mUiBuilder(ui_builder)
    , mScheduler(std::move(sched))
{
    mDataManager = bybit::ByBitDataManager::Create(mScheduler, config, db);

    mTabBarId = mUiBuilder.CreateTabBar();
    mEmptyTabId = mUiBuilder.AddMarketTab(mTabBarId, std::nullopt);
}

std::shared_ptr<TradeCockpit> TradeCockpit::Create(ui::IUiBuilder& ui_builder, std::shared_ptr<scheduler> sched, std::shared_ptr<Config> config, std::shared_ptr<SQLite::Database> db)
{
    auto self = std::make_shared<TradeCockpit>(
        ui_builder, std::move(sched), std::move(config), std::move(db), EnsurePrivate{});

    self->mDataManager->SubscribeInstrumentList(
        [weak = std::weak_ptr(self)](const std::deque<bybit::InstrumentInfo>& instruments) {
            if (auto s = weak.lock()) {
                s->OnInstrumentsLoaded(instruments);
            }
        });

    self->mUiBuilder.SetOnInstrumentSelected(
        [weak = std::weak_ptr(self)](panel_id pid, const std::string& symbol) {
            if (auto s = weak.lock()) {
                s->mUiBuilder.SetTabSymbol(pid, symbol);
                s->mTabSymbols[pid] = symbol;

                s->mEmptyTabId = s->mUiBuilder.AddMarketTab(s->mTabBarId, std::nullopt);
            }
        });

    self->mUiBuilder.SetOnTabClosed(
        [weak = std::weak_ptr(self)](panel_id pid) {
            if (auto s = weak.lock()) {
                s->mUiBuilder.CloseTab(pid);
                s->mTabSymbols.erase(pid);
            }
        });

    self->mUiBuilder.SetOnAddTab(
        [weak = std::weak_ptr(self)](panel_id tab_bar_id) {
            if (auto s = weak.lock()) {
                s->mEmptyTabId = s->mUiBuilder.AddMarketTab(tab_bar_id, std::nullopt);
            }
        });

    return self;
}

void TradeCockpit::OnInstrumentsLoaded(const std::deque<bybit::InstrumentInfo>& instruments)
{
    ui::IUiBuilder::instrument_list list;
    list.reserve(instruments.size());

    for (const auto& inst : instruments) {
        list.emplace_back(inst.symbol);
    }

    mUiBuilder.SetInstruments(std::move(list));
}

} // namespace scratcher::cockpit
