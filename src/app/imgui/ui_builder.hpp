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

namespace scratcher::imgui {

using panel_id = size_t;
constexpr panel_id INVALID_PANEL = 0;

struct MarketPanelState {
    panel_id id;
    std::optional<std::string> symbol;
};

struct TabBarState {
    panel_id id;
    std::vector<MarketPanelState> tabs;
    size_t active_tab = 0;
};

class UiBuilder
{
public:
    using instrument_list = std::vector<std::string>; // symbol, display_name
    using on_instrument_selected_t = std::function<void(panel_id, const std::string&)>;
    using on_tab_closed_t = std::function<void(panel_id)>;

    UiBuilder() = default;

    bool IsReady() const { return mInstrumentsLoaded; }

    // Panel creation - returns panel_id
    panel_id RenderTabBar();
    panel_id AddMarketTab(panel_id tab_bar_id, const std::optional<std::string>& symbol = std::nullopt);

    // Panel state modification
    void SetTabSymbol(panel_id tab_id, const std::string& symbol);
    void CloseTab(panel_id tab_id);

    // Instrument list for dropdowns
    void SetInstruments(instrument_list instruments);

    // Event handlers
    void SetOnInstrumentSelected(on_instrument_selected_t handler);
    void SetOnTabClosed(on_tab_closed_t handler);

    // Called each frame by MainWindow
    void Render();

    // Render compact tab bar for title bar area
    void RenderTitleTabs();

private:
    void RenderTabBarImpl(TabBarState& tab_bar);
    void RenderMarketPanel(const MarketPanelState& panel);
    void RenderEmptyTab(MarketPanelState& panel);
    void RenderSpinner(const char* label);

    panel_id mNextPanelId = 1;
    bool mInstrumentsLoaded = false;

    std::unordered_map<panel_id, TabBarState> mTabBars;
    std::unordered_map<panel_id, panel_id> mTabToTabBar; // tab_id -> tab_bar_id

    instrument_list mInstruments;

    on_instrument_selected_t mOnInstrumentSelected;
    on_tab_closed_t mOnTabClosed;
};

} // namespace scratcher::imgui
