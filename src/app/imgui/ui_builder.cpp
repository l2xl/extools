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

#include "ui_builder.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace scratcher::imgui {

panel_id UiBuilder::RenderTabBar()
{
    panel_id id = mNextPanelId++;
    mTabBars[id] = TabBarState{id, {}, 0};
    return id;
}

panel_id UiBuilder::AddMarketTab(panel_id tab_bar_id, const std::optional<std::string>& symbol)
{
    auto it = mTabBars.find(tab_bar_id);
    if (it == mTabBars.end())
        return INVALID_PANEL;

    panel_id tab_id = mNextPanelId++;
    it->second.tabs.push_back(MarketPanelState{tab_id, symbol});
    mTabToTabBar[tab_id] = tab_bar_id;
    return tab_id;
}

void UiBuilder::SetTabSymbol(panel_id tab_id, const std::string& symbol)
{
    auto bar_it = mTabToTabBar.find(tab_id);
    if (bar_it == mTabToTabBar.end()) return;

    auto& tab_bar = mTabBars[bar_it->second];
    for (auto& tab : tab_bar.tabs) {
        if (tab.id == tab_id) {
            tab.symbol = symbol;
            break;
        }
    }
}

void UiBuilder::CloseTab(panel_id tab_id)
{
    auto bar_it = mTabToTabBar.find(tab_id);
    if (bar_it == mTabToTabBar.end()) return;

    auto& tab_bar = mTabBars[bar_it->second];
    auto it = std::find_if(tab_bar.tabs.begin(), tab_bar.tabs.end(), [tab_id](const MarketPanelState& t) { return t.id == tab_id; });

    if (it != tab_bar.tabs.end()) {
        tab_bar.tabs.erase(it);
        mTabToTabBar.erase(tab_id);
    }
}

void UiBuilder::SetInstruments(instrument_list instruments)
{
    mInstruments = std::move(instruments);
    mInstrumentsLoaded = true;
    Render();
}

void UiBuilder::SetOnInstrumentSelected(on_instrument_selected_t handler)
{
    mOnInstrumentSelected = std::move(handler);
}

void UiBuilder::SetOnTabClosed(on_tab_closed_t handler)
{
    mOnTabClosed = std::move(handler);
}

void UiBuilder::Render()
{
    if (!mInstrumentsLoaded) {
        RenderSpinner("Loading instruments...");
        return;
    }

    // Render content area for active tab
    for (auto& [id, tab_bar] : mTabBars) {
        if (tab_bar.active_tab < tab_bar.tabs.size()) {
            const auto& tab = tab_bar.tabs[tab_bar.active_tab];
            if (tab.symbol.has_value()) {
                RenderMarketPanel(tab);
            } else {
                RenderEmptyTab(const_cast<MarketPanelState&>(tab));
            }
        }
    }
}

void UiBuilder::RenderTitleTabs()
{
    if (!mInstrumentsLoaded || mTabBars.empty()) {
        return;
    }

    // Render compact tabs in title bar
    for (auto& [id, tab_bar] : mTabBars) {
        ImGuiTabBarFlags tab_flags = ImGuiTabBarFlags_NoTabListScrollingButtons | ImGuiTabBarFlags_FittingPolicyScroll;

        std::string bar_id = "##TitleTabs_" + std::to_string(id);

        if (ImGui::BeginTabBar(bar_id.c_str(), tab_flags)) {
            for (size_t i = 0; i < tab_bar.tabs.size(); ++i) {
                auto& tab = tab_bar.tabs[i];
                bool is_empty = !tab.symbol.has_value();

                std::string tab_label = is_empty ? "+" : *tab.symbol;
                std::string tab_id = "##ttab_" + std::to_string(tab.id);
                std::string full_label = tab_label + tab_id;

                bool tab_open = true;
                bool can_close = !is_empty && tab_bar.tabs.size() > 1;
                bool* p_open = can_close ? &tab_open : nullptr;

                ImGuiTabItemFlags item_flags = ImGuiTabItemFlags_None;
                if (i == tab_bar.active_tab) {
                    item_flags |= ImGuiTabItemFlags_SetSelected;
                }

                if (ImGui::BeginTabItem(full_label.c_str(), p_open, item_flags)) {
                    tab_bar.active_tab = i;
                    ImGui::EndTabItem();
                }

                if (p_open && !tab_open) {
                    if (mOnTabClosed) {
                        mOnTabClosed(tab.id);
                    }
                }
            }
            ImGui::EndTabBar();
        }
    }
}

void UiBuilder::RenderTabBarImpl(TabBarState& tab_bar)
{
    ImGuiTabBarFlags tab_flags = ImGuiTabBarFlags_AutoSelectNewTabs |
                                  ImGuiTabBarFlags_Reorderable |
                                  ImGuiTabBarFlags_FittingPolicyResizeDown;

    std::string bar_id = "##TabBar_" + std::to_string(tab_bar.id);

    if (ImGui::BeginTabBar(bar_id.c_str(), tab_flags)) {
        std::vector<panel_id> tabs_to_close;

        for (size_t i = 0; i < tab_bar.tabs.size(); ++i) {
            auto& tab = tab_bar.tabs[i];
            bool is_empty = !tab.symbol.has_value();

            std::string tab_label = is_empty ? "+" : *tab.symbol;
            std::string tab_id = "##tab_" + std::to_string(tab.id);
            std::string full_label = tab_label + tab_id;

            bool tab_open = true;
            bool can_close = !is_empty && tab_bar.tabs.size() > 1;
            bool* p_open = can_close ? &tab_open : nullptr;

            if (ImGui::BeginTabItem(full_label.c_str(), p_open)) {
                ImVec2 content_size = ImGui::GetContentRegionAvail();
                std::string child_id = "##content_" + std::to_string(tab.id);
                ImGui::BeginChild(child_id.c_str(), content_size, false);

                if (is_empty) {
                    RenderEmptyTab(tab);
                } else {
                    RenderMarketPanel(tab);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (p_open && !tab_open) {
                tabs_to_close.push_back(tab.id);
            }
        }

        ImGui::EndTabBar();

        for (panel_id id : tabs_to_close) {
            if (mOnTabClosed) {
                mOnTabClosed(id);
            }
        }
    }
}

void UiBuilder::RenderEmptyTab(MarketPanelState& panel)
{
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select instrument to create panel");

    if (!mInstruments.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (const auto& symbol : mInstruments) {
            if (ImGui::Selectable(symbol.c_str())) {
                if (mOnInstrumentSelected) {
                    mOnInstrumentSelected(panel.id, symbol);
                }
            }
        }
    }
}

void UiBuilder::RenderMarketPanel(const MarketPanelState& panel)
{
    ImGui::BeginChild(("##market_" + std::to_string(panel.id)).c_str(),
                     ImVec2(0, 0), ImGuiChildFlags_Borders);

    // Header with symbol and dropdown
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "  %s", panel.symbol->c_str());

    ImGui::SameLine();
    std::string combo_id = "##change_" + std::to_string(panel.id);
    ImGui::SetNextItemWidth(20);

    if (ImGui::BeginCombo(combo_id.c_str(), "", ImGuiComboFlags_NoPreview)) {
        for (const auto& symbol : mInstruments) {
            bool selected = (symbol == *panel.symbol);
            if (ImGui::Selectable(symbol.c_str(), selected)) {
                if (mOnInstrumentSelected) {
                    mOnInstrumentSelected(panel.id, symbol);
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Placeholder for market data
    ImVec2 content_size = ImGui::GetContentRegionAvail();
    ImVec2 text_size = ImGui::CalcTextSize("Market data will appear here");
    ImVec2 text_pos = ImVec2( (content_size.x - text_size.x) * 0.5f, (content_size.y - text_size.y) * 0.5f );

    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + text_pos.x, ImGui::GetCursorPosY() + text_pos.y));
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Market data will appear here");

    ImGui::EndChild();
}

void UiBuilder::RenderSpinner(const char* label)
{
    ImVec2 content_size = ImGui::GetContentRegionAvail();
    ImVec2 center = ImVec2(content_size.x * 0.5f, content_size.y * 0.5f);

    // Spinner parameters
    float radius = 20.0f;
    float thickness = 4.0f;
    int num_segments = 30;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 spinner_center = ImVec2(window_pos.x + center.x, window_pos.y + center.y);

    // Animate rotation
    float time = static_cast<float>(ImGui::GetTime());
    float start_angle = time * 3.0f;

    // Draw arc (partial circle)
    ImU32 color = ImGui::GetColorU32(ImVec4(0.4f, 0.6f, 0.9f, 1.0f));

    float arc_length = 0.7f * 2.0f * 3.14159f; // ~70% of circle
    for (int i = 0; i < num_segments; ++i) {
        float a0 = start_angle + (static_cast<float>(i) / num_segments) * arc_length;
        float a1 = start_angle + (static_cast<float>(i + 1) / num_segments) * arc_length;

        ImVec2 p0 = ImVec2(spinner_center.x + cosf(a0) * radius, spinner_center.y + sinf(a0) * radius);
        ImVec2 p1 = ImVec2(spinner_center.x + cosf(a1) * radius, spinner_center.y + sinf(a1) * radius);

        draw_list->AddLine(p0, p1, color, thickness);
    }

    // Draw label below spinner
    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImGui::SetCursorPos(ImVec2(center.x - text_size.x * 0.5f, center.y + radius + 20.0f));
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", label);
}

} // namespace scratcher::imgui
