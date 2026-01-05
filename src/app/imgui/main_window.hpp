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

#include <memory>
#include <SDL3/SDL_video.h>
#include "ui_builder.hpp"
#include "system_theme.hpp"

namespace scratcher::imgui {

struct SdlSetup
{
    SdlSetup();
    ~SdlSetup();
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    int Run();

    UiBuilder& GetUiBuilder()
    { return mUiBuilder; }

private:
    void RenderFrame();
    void RenderDockspace();
    void RenderTitleBar();
    void RenderTitleButton(WindowButton button, float button_size);

    static void sdlWindowDestroy(SDL_Window* window);
    static SDL_HitTestResult HitTestCallback(SDL_Window* window, const SDL_Point* point, void* data);

    static SdlSetup sSdlSetup;

    std::unique_ptr<SDL_Window, decltype(&sdlWindowDestroy)> mWindow;
    float mTitleBarHeight = 40.0f;
    float mDpiScale = 1.0f;

    enum class TitleState { NORMAL, MENU_HOVERED, MENU_ACTIVE };
    TitleState mMenuState = TitleState::NORMAL;
    int mActiveMenuIndex = -1;  // Which child menu is open in Active state

    SystemTheme mSystemTheme;
    UiBuilder mUiBuilder;
};

} // namespace scratcher::imgui
