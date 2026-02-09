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

namespace scratcher::elements {

namespace el = cycfi::elements;
using cockpit::PanelType;
using cockpit::PanelTypeName;

namespace {

el::color header_bg_color = el::rgba(40, 40, 45, 255);
el::color footer_bg_color = el::rgba(35, 35, 40, 255);
el::color content_bg_color = el::rgba(30, 30, 35, 255);
el::color divider_color = el::rgba(80, 80, 90, 255);
el::color dim_text_color = el::rgba(128, 128, 128, 255);
el::color label_text_color = el::rgba(200, 200, 200, 255);

const PanelType all_panel_types[] = {
    PanelType::MarketGraph,
    PanelType::OrderBook,
    PanelType::Orders,
    PanelType::TradeHistory,
};

class AutoPositionMenu : public el::basic_button_menu
{
public:
    bool click(el::context const& ctx, el::mouse_button btn) override
    {
        if (btn.down) {
            auto view_size = ctx.view.size();
            bool open_left = (ctx.bounds.left + ctx.bounds.right) / 2 > view_size.x / 2;
            bool open_up = (ctx.bounds.top + ctx.bounds.bottom) / 2 > view_size.y / 2;

            if (open_up)
                position(open_left ? el::menu_position::top_left : el::menu_position::top_right);
            else
                position(open_left ? el::menu_position::bottom_left : el::menu_position::bottom_right);
        }
        return el::basic_button_menu::click(ctx, btn);
    }
};

} // anonymous namespace

el::element_ptr UiBuilder::MakeAppBar(el::element_ptr tab_bar_area,
                                       el::element_ptr menu_items,
                                       std::function<void(bool)> onHamburger)
{
    auto& thm = el::get_theme();
    auto menu_bg_color = thm.panel_color;
    auto transparent = el::rgba(0, 0, 0, 0);

    auto hamburger = el::share(
        el::toggle_icon_button(el::icons::menu, 1.0f, transparent)
    );

    auto app_deck = el::share(
        el::deck(
            el::hold(tab_bar_area),
            el::hold(menu_items)
        )
    );

    auto deck_raw = app_deck.get();
    hamburger->on_click = [onHamburger, deck_raw](bool state) {
        if (auto* deck_ptr = dynamic_cast<el::deck_element*>(deck_raw)) {
            deck_ptr->select(state ? 1 : 0);
        }
        if (onHamburger) onHamburger(state);
    };

    return el::share(
        el::layer(
            el::margin({4, 4, 4, 4},
                el::htile(
                    el::hold(hamburger),
                    el::hspace(8),
                    el::hold(app_deck)
                )
            ),
            el::box(menu_bg_color)
        )
    );
}

el::element_ptr UiBuilder::MakeMenuItems(std::function<void()> onExit)
{
    auto exit_btn = el::share(
        el::momentary_button(
            el::margin({8, 4, 8, 4}, el::label("Exit"))
        )
    );
    exit_btn->on_click = [onExit](bool) {
        if (onExit) onExit();
    };

    auto about_btn = el::share(
        el::momentary_button(
            el::margin({8, 4, 8, 4}, el::label("About"))
        )
    );

    return el::share(
        el::htile(
            el::hold(exit_btn),
            el::hspace(4),
            el::hold(about_btn),
            el::hstretch(1.0, el::element{})
        )
    );
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

el::element_ptr UiBuilder::MakePanelTypeSelector(
    uint32_t icon_code,
    std::function<void(PanelType)> onTypeSelected)
{
    auto popup = el::share(
        el::momentary_button<AutoPositionMenu>(
            el::margin({4, 2, 4, 2}, el::icon(icon_code, 0.7f))
        )
    );

    el::vtile_composite menu_items;
    for (auto type : all_panel_types) {
        auto item = el::menu_item(PanelTypeName(type));
        item.on_click = [onTypeSelected, type]() {
            if (onTypeSelected) onTypeSelected(type);
        };
        menu_items.push_back(el::share(std::move(item)));
    }

    popup->menu(
        el::layer(
            el::vtile(std::move(menu_items)),
            el::panel{}
        )
    );

    return popup;
}

std::pair<el::element_ptr, std::shared_ptr<el::deck_composite>> UiBuilder::MakePanel(PanelType type, std::function<void(PanelType)> onChangeType, std::function<void(PanelType, SplitDirection)> onSplit)
{
    auto deck = std::make_shared<el::deck_composite>();
    deck->push_back(MakeWaitingIndicator());
    deck->push_back(MakePanelContent(type));
    deck->select(0);

    auto root = el::share(
        el::vtile(
            el::hold(MakePanelHeader(type, std::move(onChangeType), onSplit)),
            el::vstretch(1.0, el::hold(deck)),
            el::hold(MakePanelFooter(std::move(onSplit)))
        )
    );

    return std::make_pair(std::move(root), std::move(deck));
}

el::element_ptr UiBuilder::MakeVerticalSplit(el::element_ptr left, el::element_ptr right)
{
    return el::share(
        el::htile(
            el::hstretch(1.0, el::hold(left)),
            el::hold(MakeDivider(false)),
            el::hstretch(1.0, el::hold(right))
        )
    );
}

el::element_ptr UiBuilder::MakeHorizontalSplit(el::element_ptr top, el::element_ptr bottom)
{
    return el::share(
        el::vtile(
            el::vstretch(1.0, el::hold(top)),
            el::hold(MakeDivider(true)),
            el::vstretch(1.0, el::hold(bottom))
        )
    );
}

el::element_ptr UiBuilder::MakeDivider(bool horizontal)
{
    if (horizontal) {
        return el::share(el::hmin_size(1, el::vsize(4, el::box(divider_color))));
    }
    return el::share(el::vmin_size(1, el::hsize(4, el::box(divider_color))));
}

el::element_ptr UiBuilder::MakePanelHeader(PanelType type, std::function<void(PanelType)> onChangeType, std::function<void(PanelType, SplitDirection)> onSplit)
{
    return el::share(
        el::layer(
            el::margin({2, 1, 2, 1},
                el::htile(
                    el::hold(MakePanelTypeSelector(el::icons::down_dir, std::move(onChangeType))),
                    el::hstretch(1.0,
                        el::align_center(
                            el::label(PanelTypeName(type))
                                .font_size(12)
                                .font_color(label_text_color)
                        )
                    ),
                    el::hold(MakePanelTypeSelector(el::icons::plus, [onSplit](PanelType t) {
                        if (onSplit) onSplit(t, SplitDirection::Vertical);
                    }))
                )
            ),
            el::box(header_bg_color)
        )
    );
}

el::element_ptr UiBuilder::MakePanelFooter(std::function<void(PanelType, SplitDirection)> onSplit)
{
    return el::share(
        el::layer(
            el::margin({2, 1, 2, 1},
                el::htile(
                    el::hold(MakePanelTypeSelector(el::icons::plus, [onSplit](PanelType t) {
                        if (onSplit) onSplit(t, SplitDirection::Horizontal);
                    })),
                    el::hstretch(1.0, el::element{})
                )
            ),
            el::box(footer_bg_color)
        )
    );
}

el::element_ptr UiBuilder::MakeWaitingIndicator()
{
    return el::share(
        el::layer(
            el::align_center_middle(
                el::label("Loading...")
                    .font_size(16)
                    .font_color(dim_text_color)
            ),
            el::box(content_bg_color)
        )
    );
}

el::element_ptr UiBuilder::MakePanelContent(PanelType type)
{
    return el::share(
        el::layer(
            el::align_center_middle(
                el::label(PanelTypeName(type))
                    .font_size(24)
                    .font_color(dim_text_color)
            ),
            el::box(content_bg_color)
        )
    );
}

} // namespace scratcher::elements
