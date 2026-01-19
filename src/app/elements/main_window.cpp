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

#include "main_window.hpp"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <iostream>

namespace scratcher::elements {

namespace el = cycfi::elements;

namespace {

std::string ExecuteCommand(const char* cmd)
{
    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

bool IsDarkModePreferred()
{
    // Method 1: GNOME color-scheme setting (modern GNOME 42+)
    std::string scheme = ExecuteCommand("gsettings get org.gnome.desktop.interface color-scheme 2>/dev/null");
    if (scheme.find("dark") != std::string::npos) {
        return true;
    }
    if (scheme.find("light") != std::string::npos) {
        return false;
    }

    // Method 2: GTK theme name contains "dark"
    std::string theme = ExecuteCommand("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null");
    if (theme.find("dark") != std::string::npos || theme.find("Dark") != std::string::npos) {
        return true;
    }

    // Method 3: Environment variable
    const char* gtk_theme = std::getenv("GTK_THEME");
    if (gtk_theme) {
        std::string theme_str(gtk_theme);
        if (theme_str.find("dark") != std::string::npos || theme_str.find("Dark") != std::string::npos) {
            return true;
        }
    }

    // Method 4: Check freedesktop portal color scheme
    std::string portal = ExecuteCommand(
        "gdbus call --session --dest org.freedesktop.portal.Desktop "
        "--object-path /org/freedesktop/portal/desktop "
        "--method org.freedesktop.portal.Settings.Read org.freedesktop.appearance color-scheme 2>/dev/null"
    );
    // Portal returns: (<uint32 1>,) for dark, (<uint32 2>,) for light, (<uint32 0>,) for no preference
    if (portal.find("uint32 1") != std::string::npos) {
        return true;
    }

    return false;
}

// std::string GetCurrentGtkTheme()
// {
//     std::string theme = ExecuteCommand("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null");
//     // Remove quotes from gsettings output: 'Adwaita-dark' -> Adwaita-dark
//     if (theme.size() >= 2 && theme.front() == '\'' && theme.back() == '\'') {
//         theme = theme.substr(1, theme.size() - 2);
//     }
//     return theme.empty() ? "Adwaita" : theme;
// }

// void ApplyElementsDarkTheme()
// {
//     el::theme thm = el::get_theme();
//
//     thm.panel_color = el::rgba(45, 45, 48, 255);
//     thm.frame_color = el::rgba(70, 70, 75, 255);
//     thm.frame_hilite_color = el::rgba(100, 100, 105, 255);
//     thm.scrollbar_color = el::rgba(80, 80, 85, 200);
//     thm.default_button_color = el::rgba(60, 60, 65, 255);
//     thm.controls_color = el::rgba(55, 55, 60, 255);
//     thm.indicator_color = el::rgba(80, 140, 200, 255);
//     thm.indicator_bright_color = el::rgba(100, 160, 220, 255);
//     thm.indicator_hilite_color = el::rgba(120, 180, 240, 255);
//     thm.basic_font_color = el::rgba(220, 220, 220, 255);
//     thm.heading_font_color = el::rgba(240, 240, 240, 255);
//     thm.label_font_color = el::rgba(200, 200, 200, 255);
//     thm.icon_color = el::rgba(180, 180, 180, 255);
//     thm.icon_button_color = el::rgba(50, 50, 55, 255);
//     thm.text_box_font_color = el::rgba(220, 220, 220, 255);
//     thm.text_box_hilite_color = el::rgba(80, 120, 180, 180);
//     thm.text_box_caret_color = el::rgba(220, 220, 220, 255);
//     thm.inactive_font_color = el::rgba(128, 128, 128, 255);
//     thm.ticks_color = el::rgba(100, 100, 105, 255);
//     thm.major_grid_color = el::rgba(80, 80, 85, 255);
//     thm.minor_grid_color = el::rgba(60, 60, 65, 255);
//
//     el::set_theme(thm);
// }
//
// void ApplyElementsLightTheme()
// {
//     el::theme thm;
//     el::set_theme(thm);
// }

} // anonymous namespace

MainWindow::MainWindow()
    : mApp("Scratcher")
    , mWindow(mApp.name())
{
    mWindow.on_close = [this]() { mApp.stop(); };

    mView = std::make_unique<el::view>(mWindow);
    mUiBuilder.SetView(mView.get());

    SetupContent();
}

MainWindow::~MainWindow() = default;

int MainWindow::Run()
{
    mApp.run();
    return 0;
}

el::element_ptr MainWindow::MakeMenuItems()
{
    auto exit_btn = el::share(
        el::momentary_button(
            el::margin({8, 4, 8, 4},
                el::label("Exit")
            )
        )
    );
    exit_btn->on_click = [this](bool) {
        mApp.stop();
    };

    auto about_btn = el::share(
        el::momentary_button(
            el::margin({8, 4, 8, 4},
                el::label("About")
            )
        )
    );
    about_btn->on_click = [](bool) {
        // TODO: Show about dialog
    };

    return el::share(
        el::htile(
            el::hold(exit_btn),
            el::hspace(4),
            el::hold(about_btn),
            el::hstretch(1.0, el::element{})
        )
    );
}

el::element_ptr MainWindow::MakeAppBar()
{
    auto& thm = el::get_theme();
    auto menu_bg_color = thm.panel_color;
    auto transparent = el::rgba(0, 0, 0, 0);

    auto hamburger = el::share(
        el::toggle_icon_button(el::icons::menu, 1.0f, transparent)
    );

    auto menu_items = MakeMenuItems();
    auto tab_bar = mUiBuilder.MakeTabBarOnly();

    mAppDeck = el::share(
        el::deck(
            el::hold(tab_bar),
            el::hold(menu_items)
        )
    );

    hamburger->on_click = [this](bool state) {
        mMenuVisible = state;
        if (auto* deck_ptr = dynamic_cast<el::deck_element*>(mAppDeck.get())) {
            deck_ptr->select(mMenuVisible ? 1 : 0);
            mView->refresh();
        }
    };

    return el::share(
        el::layer(
            el::margin({4, 4, 4, 4},
                el::htile(
                    el::hold(hamburger),
                    el::hspace(8),
                    el::hold(mAppDeck)
                )
            ),
            el::box(menu_bg_color)
        )
    );
}

void MainWindow::SetupContent()
{
    auto app_bar = MakeAppBar();
    auto content = mUiBuilder.MakeContentPanel();

    mView->content(
        el::layer(
            el::vtile(
                el::hold(app_bar),
                el::vstretch(1.0, el::hold(content))
             )
        )
    );
}

} // namespace scratcher::elements
