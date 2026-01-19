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
#include <optional>
#include <functional>
#include <unordered_map>
#include <memory>

#include <elements.hpp>
#include "app/ui_builder.hpp"

namespace scratcher::elements {

using panel_id = ui::panel_id;
constexpr panel_id INVALID_PANEL = ui::INVALID_PANEL;

struct MarketPanelState {
    panel_id id;
    std::optional<std::string> symbol;
};

struct TabBarState {
    panel_id id;
    std::vector<MarketPanelState> tabs;
    size_t active_tab = 0;
};

class UiBuilder : public ui::IUiBuilder
{
public:
    UiBuilder();

    bool IsReady() const override { return mInstrumentsLoaded; }

    panel_id CreateTabBar() override;
    panel_id AddMarketTab(panel_id tab_bar_id, const std::optional<std::string>& symbol = std::nullopt) override;

    void SetTabSymbol(panel_id tab_id, const std::string& symbol) override;
    void CloseTab(panel_id tab_id) override;

    void SetInstruments(instrument_list instruments) override;

    void SetOnInstrumentSelected(on_instrument_selected_t handler) override;
    void SetOnTabClosed(on_tab_closed_t handler) override;
    void SetOnAddTab(on_add_tab_t handler) override;

    cycfi::elements::element_ptr MakeContent();
    cycfi::elements::element_ptr MakeTabBarOnly();
    cycfi::elements::element_ptr MakeContentPanel();

    void SetView(cycfi::elements::view* view) { mView = view; }
    void RefreshView();

private:
    cycfi::elements::element_ptr MakeTabBar(TabBarState& tab_bar);
    cycfi::elements::element_ptr MakeTabButton(const MarketPanelState& tab, size_t index, TabBarState& tab_bar);
    cycfi::elements::element_ptr MakeMarketPanel(const MarketPanelState& panel);
    cycfi::elements::element_ptr MakeEmptyTab(MarketPanelState& panel);
    cycfi::elements::element_ptr MakeInstrumentList(MarketPanelState& panel);
    cycfi::elements::element_ptr MakeLoadingSpinner();

    panel_id mNextPanelId = 1;
    bool mInstrumentsLoaded = false;

    std::unordered_map<panel_id, TabBarState> mTabBars;
    std::unordered_map<panel_id, panel_id> mTabToTabBar;

    instrument_list mInstruments;

    on_instrument_selected_t mOnInstrumentSelected;
    on_tab_closed_t mOnTabClosed;
    on_add_tab_t mOnAddTab;

    cycfi::elements::view* mView = nullptr;
};

} // namespace scratcher::elements
