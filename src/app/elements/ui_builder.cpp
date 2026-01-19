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

#include "ui_builder.hpp"
#include <algorithm>

namespace scratcher::elements {

namespace el = cycfi::elements;

namespace {

el::color tab_normal_color = el::rgba(45, 45, 48, 255);
el::color tab_active_color = el::rgba(60, 60, 65, 255);
el::color tab_hover_color = el::rgba(70, 70, 75, 255);
el::color separator_color = el::rgba(80, 80, 85, 255);
el::color text_color = el::rgba(200, 200, 200, 255);
el::color accent_color = el::rgba(180, 180, 230, 255);
el::color dim_text_color = el::rgba(128, 128, 128, 255);

} // anonymous namespace

UiBuilder::UiBuilder()
{
}

panel_id UiBuilder::CreateTabBar()
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
    RefreshView();
}

void UiBuilder::CloseTab(panel_id tab_id)
{
    auto bar_it = mTabToTabBar.find(tab_id);
    if (bar_it == mTabToTabBar.end()) return;

    auto& tab_bar = mTabBars[bar_it->second];
    auto it = std::find_if(tab_bar.tabs.begin(), tab_bar.tabs.end(),
        [tab_id](const MarketPanelState& t) { return t.id == tab_id; });

    if (it != tab_bar.tabs.end()) {
        tab_bar.tabs.erase(it);
        mTabToTabBar.erase(tab_id);

        if (tab_bar.active_tab >= tab_bar.tabs.size() && tab_bar.active_tab > 0) {
            tab_bar.active_tab = tab_bar.tabs.size() - 1;
        }
    }
    RefreshView();
}

void UiBuilder::SetInstruments(instrument_list instruments)
{
    mInstruments = std::move(instruments);
    mInstrumentsLoaded = true;
    RefreshView();
}

void UiBuilder::SetOnInstrumentSelected(on_instrument_selected_t handler)
{
    mOnInstrumentSelected = std::move(handler);
}

void UiBuilder::SetOnTabClosed(on_tab_closed_t handler)
{
    mOnTabClosed = std::move(handler);
}

void UiBuilder::SetOnAddTab(on_add_tab_t handler)
{
    mOnAddTab = std::move(handler);
}

void UiBuilder::RefreshView()
{
    if (mView) {
        mView->refresh();
    }
}

el::element_ptr UiBuilder::MakeContent()
{
    if (!mInstrumentsLoaded) {
        return MakeLoadingSpinner();
    }

    if (mTabBars.empty()) {
        return el::share(
            el::align_center_middle(
                el::label("No tabs available").font_size(16)
            )
        );
    }

    el::vtile_composite content_vtile;

    for (auto& [id, tab_bar] : mTabBars) {
        content_vtile.push_back(MakeTabBar(tab_bar));

        if (tab_bar.active_tab < tab_bar.tabs.size()) {
            const auto& tab = tab_bar.tabs[tab_bar.active_tab];
            if (tab.symbol.has_value()) {
                content_vtile.push_back(MakeMarketPanel(tab));
            } else {
                content_vtile.push_back(MakeEmptyTab(const_cast<MarketPanelState&>(tab)));
            }
        }
    }

    return el::share(el::vtile(std::move(content_vtile)));
}

el::element_ptr UiBuilder::MakeTabBarOnly()
{
    el::htile_composite tabs_htile;
    panel_id first_tab_bar_id = INVALID_PANEL;

    for (auto& [id, tab_bar] : mTabBars) {
        if (first_tab_bar_id == INVALID_PANEL) {
            first_tab_bar_id = id;
        }
        for (size_t i = 0; i < tab_bar.tabs.size(); ++i) {
            tabs_htile.push_back(MakeTabButton(tab_bar.tabs[i], i, tab_bar));
        }
    }

    // Add "plus" button for adding new tabs (Material Design flat style)
    auto add_btn = el::share(
        el::momentary_button(
            el::margin({6, 4, 6, 4},
                el::icon(el::icons::plus, 0.7f)
            )
        )
    );
    add_btn->on_click = [this, first_tab_bar_id](bool) {
        if (mOnAddTab && first_tab_bar_id != INVALID_PANEL) {
            mOnAddTab(first_tab_bar_id);
        }
    };
    tabs_htile.push_back(add_btn);

    tabs_htile.push_back(el::share(el::hstretch(1.0, el::element{})));

    return el::share(
        el::layer(
            el::htile(std::move(tabs_htile)),
            el::box(tab_normal_color)
        )
    );
}

el::element_ptr UiBuilder::MakeContentPanel()
{
    if (!mInstrumentsLoaded) {
        return MakeLoadingSpinner();
    }

    if (mTabBars.empty()) {
        return el::share(
            el::align_center_middle(
                el::label("No tabs available").font_size(16)
            )
        );
    }

    el::vtile_composite content_vtile;

    for (auto& [id, tab_bar] : mTabBars) {
        if (tab_bar.active_tab < tab_bar.tabs.size()) {
            const auto& tab = tab_bar.tabs[tab_bar.active_tab];
            if (tab.symbol.has_value()) {
                content_vtile.push_back(MakeMarketPanel(tab));
            } else {
                content_vtile.push_back(MakeEmptyTab(const_cast<MarketPanelState&>(tab)));
            }
        }
    }

    return el::share(el::vtile(std::move(content_vtile)));
}

el::element_ptr UiBuilder::MakeTabBar(TabBarState& tab_bar)
{
    el::htile_composite tabs_htile;

    for (size_t i = 0; i < tab_bar.tabs.size(); ++i) {
        tabs_htile.push_back(MakeTabButton(tab_bar.tabs[i], i, tab_bar));
    }

    tabs_htile.push_back(el::share(el::hstretch(1.0, el::element{})));

    return el::share(
        el::layer(
            el::htile(std::move(tabs_htile)),
            el::box(tab_normal_color)
        )
    );
}

el::element_ptr UiBuilder::MakeTabButton(const MarketPanelState& tab, size_t index, TabBarState& tab_bar)
{
    bool is_empty = !tab.symbol.has_value();
    std::string label_text = is_empty ? "+" : *tab.symbol;
    bool is_active = (index == tab_bar.active_tab);

    auto body_color = is_active ? tab_active_color : tab_normal_color;

    panel_id tab_id = tab.id;
    panel_id bar_id = tab_bar.id;

    auto tab_btn = el::share(
        el::momentary_button(
            el::button_styler{label_text}
                .body_color(body_color)
                .text_color(text_color)
                .corner_radius(0)
        )
    );

    tab_btn->on_click = [this, tab_id, bar_id, index](bool) {
        auto it = mTabBars.find(bar_id);
        if (it != mTabBars.end()) {
            it->second.active_tab = index;
            RefreshView();
        }
    };

    if (!is_empty && tab_bar.tabs.size() > 1) {
        auto close_btn = el::share(
            el::momentary_button(
                el::button_styler{"x"}
                    .size(0.7f)
                    .body_color(body_color)
                    .text_color(el::colors::red)
                    .corner_radius(0)
            )
        );

        close_btn->on_click = [this, tab_id](bool) {
            if (mOnTabClosed) {
                mOnTabClosed(tab_id);
            }
        };

        return el::share(
            el::htile(
                el::hold(tab_btn),
                el::hold(close_btn)
            )
        );
    }

    return tab_btn;
}

el::element_ptr UiBuilder::MakeMarketPanel(const MarketPanelState& panel)
{
    auto header = el::htile(
        el::margin({10, 8, 10, 8},
            el::label(*panel.symbol)
                .font_size(16)
                .font_color(accent_color)
        ),
        el::hstretch(1.0, el::element{})
    );

    auto separator = el::vsize(1, el::box(separator_color));

    auto placeholder = el::align_center_middle(
        el::label("Market data will appear here")
            .font_size(14)
            .font_color(dim_text_color)
    );

    return el::share(
        el::layer(
            el::vtile(
                header,
                separator,
                el::vstretch(1.0, placeholder)
            ),
            el::margin({1, 1, 1, 1}, el::box(el::rgba(55, 55, 60, 255)))
        )
    );
}

el::element_ptr UiBuilder::MakeEmptyTab(MarketPanelState& panel)
{
    auto header = el::margin({10, 10, 10, 5},
        el::label("Select instrument to create panel")
            .font_size(14)
            .font_color(dim_text_color)
    );

    auto separator = el::vsize(1, el::box(separator_color));

    auto list = MakeInstrumentList(panel);

    return el::share(
        el::vtile(
            header,
            separator,
            el::vstretch(1.0, el::scroller(el::hold(list)))
        )
    );
}

el::element_ptr UiBuilder::MakeInstrumentList(MarketPanelState& panel)
{
    el::vtile_composite list_vtile;
    panel_id pid = panel.id;

    for (const auto& symbol : mInstruments) {
        std::string sym_copy = symbol;

        auto item_btn = el::share(
            el::momentary_button(
                el::button_styler{symbol}
                    .align_left()
                    .body_color(tab_normal_color)
                    .active_body_color(tab_hover_color)
                    .text_color(text_color)
                    .corner_radius(0)
            )
        );

        item_btn->on_click = [this, pid, sym_copy](bool) {
            if (mOnInstrumentSelected) {
                mOnInstrumentSelected(pid, sym_copy);
            }
        };

        list_vtile.push_back(item_btn);
    }

    return el::share(el::vtile(std::move(list_vtile)));
}

el::element_ptr UiBuilder::MakeLoadingSpinner()
{
    return el::share(
        el::align_center_middle(
            el::vtile(
                el::label("Loading...").font_size(40).font_color(el::rgba(100, 150, 230, 255)),
                el::vspace(10),
                el::label("Loading instruments...").font_size(14).font_color(dim_text_color)
            )
        )
    );
}

} // namespace scratcher::elements
