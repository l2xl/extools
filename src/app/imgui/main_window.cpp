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

#include "main_window.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <iostream>
#include <sstream>

namespace scratcher::imgui {

namespace {

// Build glyph ranges containing only our button symbols
const ImWchar symbol_ranges[] = {
    0x00D7, 0x00D7,  // × (Multiplication sign - Close)
    0x2212, 0x2212,  // − (Minus sign - Minimize)
    0x25A1, 0x25A1,  // □ (White square - Maximize)
    0x2750, 0x2750,  // ❐ (Drop-shadowed white square - Restore)
    0x2630, 0x2630,  // ☰ (Trigram - Menu)
    0
};

SDL_Window* SdlCreateWindow(int w, int h, const char* title)
{
    SDL_Window* window = SDL_CreateWindow( title, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!window) {
        std::string error = (std::ostringstream()  << "Failed to create SDL window: " << SDL_GetError()).str();
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }
    return window;
}

}

void MainWindow::sdlWindowDestroy(SDL_Window* window)
{
    if (window) {
        SDL_DestroyWindow(window);
    }
}

SDL_HitTestResult MainWindow::HitTestCallback(SDL_Window* window, const SDL_Point* point, void* data)
{
    MainWindow* self = static_cast<MainWindow*>(data);

    int window_w, window_h;
    SDL_GetWindowSize(window, &window_w, &window_h);

    // Title bar area (scaled by ImGui's ScaleAllSizes)
    const float scaled_titlebar_height = self->mTitleBarHeight * self->mDpiScale;
    if (point->y >= 0 && point->y < static_cast<int>(scaled_titlebar_height)) {
        // When menu is in Hover or Active state, disable title bar dragging
        // so that menu clicks work properly
        if (self->mMenuState != TitleState::NORMAL) {
            return SDL_HITTEST_NORMAL;
        }

        // Use base sizes - ImGui scales them via ScaleAllSizes()
        const int button_size = static_cast<int>(28 * self->mDpiScale);
        const int button_spacing = static_cast<int>(4 * self->mDpiScale);
        const int padding = static_cast<int>(16 * self->mDpiScale);
        const auto& layout = self->mSystemTheme.GetButtonLayout();

        // Calculate button regions
        int left_buttons_width = 0;
        if (!layout.left_buttons.empty()) {
            left_buttons_width = button_size * layout.left_buttons.size() + button_spacing * (layout.left_buttons.size() - 1) + padding;
        }

        int right_buttons_width = 0;
        if (!layout.right_buttons.empty()) {
            right_buttons_width = button_size * layout.right_buttons.size() + button_spacing * (layout.right_buttons.size() - 1) + padding;
        }

        // Check if click is in button areas (non-draggable)
        if (point->x < left_buttons_width || point->x >= window_w - right_buttons_width) {
            return SDL_HITTEST_NORMAL;
        }

        // Rest of title bar is draggable
        return SDL_HITTEST_DRAGGABLE;
    }

    // Window resize areas (border thickness: 5 pixels, scaled for DPI)
    const int border_size = static_cast<int>(5 * self->mDpiScale);
    bool in_left = point->x < border_size;
    bool in_right = point->x >= window_w - border_size;
    bool in_top = point->y < border_size;
    bool in_bottom = point->y >= window_h - border_size;

    if (in_top && in_left) return SDL_HITTEST_RESIZE_TOPLEFT;
    if (in_top && in_right) return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (in_bottom && in_left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (in_bottom && in_right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (in_left) return SDL_HITTEST_RESIZE_LEFT;
    if (in_right) return SDL_HITTEST_RESIZE_RIGHT;
    if (in_top) return SDL_HITTEST_RESIZE_TOP;
    if (in_bottom) return SDL_HITTEST_RESIZE_BOTTOM;

    return SDL_HITTEST_NORMAL;
}

SdlSetup::SdlSetup()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to initialize SDL");
    }

    // Setup OpenGL context attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

SdlSetup::~SdlSetup()
{
    SDL_Quit();
}

SdlSetup MainWindow::sSdlSetup;

MainWindow::MainWindow() : mWindow(SdlCreateWindow(1280, 720, "ImScratcher"), &sdlWindowDestroy)
{
    const char* glsl_version = "#version 130";

    // Create OpenGL context
    SDL_GLContext gl_context = SDL_GL_CreateContext(mWindow.get());
    if (!gl_context) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to create OpenGL context");
    }
    SDL_GL_MakeCurrent(mWindow.get(), gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Detect system theme first to get font info
    mSystemTheme.DetectTheme();
    const auto& font_info = mSystemTheme.GetInterfaceFont();

    // Detect DPI scaling from multiple sources
    float dpi_scale = 1.0f;

    // Method 1: SDL3 window display scale (most reliable for actual display)
    float sdl_scale = SDL_GetWindowDisplayScale(mWindow.get());
    if (sdl_scale > 0.0f) {
        dpi_scale = sdl_scale;
        std::cout << "SDL3 detected display scale: " << dpi_scale << std::endl;
    }

    // Method 2: System settings detection (fallback/verification)
    auto display_info = mSystemTheme.DetectDisplayScale();
    if (display_info.is_hidpi && display_info.content_scale > dpi_scale) {
        dpi_scale = display_info.content_scale;
        std::cout << "Using system settings display scale: " << dpi_scale
                  << " (method: " << display_info.detection_method
                  << ", calculated PPI: " << display_info.calculated_ppi << ")" << std::endl;
    } else if (display_info.content_scale > dpi_scale) {
        // Even if not flagged as hidpi, use the calculated scale if higher
        dpi_scale = display_info.content_scale;
        std::cout << "Using calculated display scale: " << dpi_scale
                  << " (method: " << display_info.detection_method << ")" << std::endl;
    }

    // Load system font with proper glyph ranges
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;

    // Build glyph ranges for main UI font (text only)
    const ImWchar* text_ranges = io.Fonts->GetGlyphRangesDefault();

    // Calculate scaled font size
    float base_font_size = static_cast<float>(font_info.size);

    // For high-PPI displays (even at scale 1.0), increase base font size
    if (display_info.calculated_ppi >= 150.0f && dpi_scale == 1.0f) {
        base_font_size *= 1.5f;  // Increase font for readability on high-PPI
        std::cout << "High-PPI display detected, increasing font size for readability" << std::endl;
    }

    float scaled_font_size = base_font_size * dpi_scale;

    std::cout << "Loading font at size: " << scaled_font_size
              << " (base: " << base_font_size << " × DPI scale: " << dpi_scale << ")" << std::endl;

    // Load the system font if available, otherwise use default
    ImFont* main_font = nullptr;
    if (!font_info.file_path.empty()) {
        // Try to load the detected font
        main_font = io.Fonts->AddFontFromFileTTF(
            font_info.file_path.c_str(),
            scaled_font_size,
            &font_config,
            text_ranges
        );

        // If variable font failed, try non-variable Ubuntu font
        if (!main_font && font_info.family.find("Ubuntu") != std::string::npos) {
            std::cerr << "Variable font failed, trying non-variable Ubuntu-R.ttf" << std::endl;
            main_font = io.Fonts->AddFontFromFileTTF(
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
                scaled_font_size,
                &font_config,
                text_ranges
            );
        }

        if (!main_font) {
            std::cerr << "Failed to load system font, using default" << std::endl;
            font_config.SizePixels = scaled_font_size;
            main_font = io.Fonts->AddFontDefault(&font_config);
        }
    } else {
        std::cerr << "System font file not found, using default" << std::endl;
        font_config.SizePixels = scaled_font_size;
        main_font = io.Fonts->AddFontDefault(&font_config);
    }

    // Now merge symbol font for button icons

    ImFontConfig symbol_config;
    symbol_config.MergeMode = true;  // Merge into main font
    symbol_config.OversampleH = 2;
    symbol_config.OversampleV = 2;
    symbol_config.GlyphMinAdvanceX = scaled_font_size; // Make icons monospaced

    // Try DejaVu Sans which has excellent Unicode symbol coverage
    ImFont* symbol_font = io.Fonts->AddFontFromFileTTF(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        scaled_font_size,
        &symbol_config,
        symbol_ranges
    );

    if (symbol_font) {
        std::cout << "Merged DejaVu Sans for button symbols" << std::endl;
    } else {
        std::cerr << "Warning: Could not load DejaVu Sans for symbols, icons may not display correctly" << std::endl;
    }

    // Store DPI scale for later use
    mDpiScale = dpi_scale;

    // Note: Title bar height will be scaled by ScaleAllSizes() in ApplyToImGui()
    // Don't manually scale it here to avoid double scaling

    // Apply system theme styling with DPI scale
    mSystemTheme.ApplyToImGui(dpi_scale);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(mWindow.get(), gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup window hit test for custom title bar dragging
    SDL_SetWindowHitTest(mWindow.get(), HitTestCallback, this);
}

MainWindow::~MainWindow()
{
    // Future: Save layout to file

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

int MainWindow::Run()
{
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                running = false;
            }
        }

        RenderFrame();
    }
    return 0;
}

void MainWindow::RenderFrame()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    RenderTitleBar();
    RenderDockspace();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    SDL_GetWindowSizeInPixels(mWindow.get(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(mWindow.get());
}

void MainWindow::RenderTitleBar()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, mTitleBarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

    // Use system-inspired colors
    ImVec4 titlebar_bg = ImVec4(0.22f, 0.22f, 0.24f, 1.0f);
    ImVec4 button_normal = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 button_hovered = ImVec4(0.35f, 0.35f, 0.35f, 0.8f);
    ImVec4 button_active = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, titlebar_bg);
    ImGui::PushStyleColor(ImGuiCol_Button, button_normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active);

    ImGui::Begin("##TitleBar", nullptr, window_flags);

    // Note: Don't manually scale button sizes - ScaleAllSizes() handles this
    const float button_size = 28.0f;
    const float button_spacing = 4.0f;
    const auto& layout = mSystemTheme.GetButtonLayout();

    // Render left side buttons (skip Menu - it's combined with title)
    bool menu_on_left = false;
    for (size_t i = 0; i < layout.left_buttons.size(); ++i) {
        if (layout.left_buttons[i] == WindowButton::Menu) {
            menu_on_left = true;
            continue;  // Skip menu button, render combined with title
        }
        if (i > 0) ImGui::SameLine(0, button_spacing);
        RenderTitleButton(layout.left_buttons[i], button_size);
    }

    // Menu configuration
    const auto& icons = mSystemTheme.GetButtonIcons();
    const char* title_text = "ImScratcher";
    std::string menu_title = std::string(icons.menu) + "  " + title_text;
    float window_width = ImGui::GetWindowWidth();

    // Calculate space needed for right buttons (match window padding)
    const float padding = 8.0f;
    float right_buttons_width = 0;
    if (!layout.right_buttons.empty()) {
        right_buttons_width = button_size * layout.right_buttons.size() + button_spacing * (layout.right_buttons.size() - 1) + padding;
    }

    if (menu_on_left) {
        ImGui::SameLine(0, 0.0f);
    }

    // Track if mouse is over any menu element
    bool any_menu_hovered = false;
    bool any_popup_open = false;

    // Transparent button style for menu items, no rounding
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

    // STATE: Normal - show combined menu+title button
    if (mMenuState == TitleState::NORMAL) {
        if (ImGui::Button(menu_title.c_str())) {
            // Click -> Active state, open first menu
            mMenuState = TitleState::MENU_ACTIVE;
            mActiveMenuIndex = 0;
        }
        if (ImGui::IsItemHovered()) {
            // Hover -> Hover state
            mMenuState = TitleState::MENU_HOVERED;
        }

        // Render tabs in Normal state
        ImGui::SameLine(0, 16.0f);
        mUiBuilder.RenderTitleTabs();
    }
    // STATE: Hover or Active - show menu bar items
    else {
        // Store positions for child popups
        ImVec2 menu_positions[2];

        // Menu item 0: ImScratcher (with glyph)
        menu_positions[0] = ImGui::GetCursorScreenPos();
        menu_positions[0].y += mTitleBarHeight;

        ImGui::Button(menu_title.c_str());
        bool item0_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        bool item0_clicked = item0_hovered && ImGui::IsMouseClicked(0);
        if (item0_hovered) any_menu_hovered = true;

        // Menu item 1: Help
        ImGui::SameLine(0, 0.0f);
        menu_positions[1] = ImGui::GetCursorScreenPos();
        menu_positions[1].y += mTitleBarHeight;

        // Menu item labels with glyphs (same spacing as title button)
        std::string help_label = std::string("?") + "  Help";
        std::string exit_label = std::string(icons.close) + "  Exit";
        std::string about_label = std::string("?") + "  About...";

        ImGui::Button(help_label.c_str());
        bool item1_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        bool item1_clicked = item1_hovered && ImGui::IsMouseClicked(0);
        if (item1_hovered) any_menu_hovered = true;

        // Handle state transitions and menu logic
        if (mMenuState == TitleState::MENU_HOVERED) {
            // In Hover state: no child menus open
            if (item0_clicked) {
                mMenuState = TitleState::MENU_ACTIVE;
                mActiveMenuIndex = 0;
            } else if (item1_clicked) {
                mMenuState = TitleState::MENU_ACTIVE;
                mActiveMenuIndex = 1;
            } else if (!any_menu_hovered) {
                // Mouse left menu area -> back to Normal
                mMenuState = TitleState::NORMAL;
            }
        } else {
            // Active state: first check if popup was closed externally (click outside)
            bool expected_popup_open = (mActiveMenuIndex == 0 && ImGui::IsPopupOpen("##AppMenu")) ||
                                       (mActiveMenuIndex == 1 && ImGui::IsPopupOpen("##HelpMenu"));

            // If we expected a popup but it's closed, user clicked outside -> return to Normal
            if (mActiveMenuIndex >= 0 && !expected_popup_open && !any_menu_hovered) {
                mMenuState = TitleState::NORMAL;
                mActiveMenuIndex = -1;
            } else {
                // Handle menu switching
                int prev_menu_index = mActiveMenuIndex;

                if (item0_clicked) {
                    if (mActiveMenuIndex == 0) {
                        // Click on already active -> go to Hover (close child)
                        mMenuState = TitleState::MENU_HOVERED;
                        mActiveMenuIndex = -1;
                    } else {
                        mActiveMenuIndex = 0;
                    }
                } else if (item1_clicked) {
                    if (mActiveMenuIndex == 1) {
                        // Click on already active -> go to Hover (close child)
                        mMenuState = TitleState::MENU_HOVERED;
                        mActiveMenuIndex = -1;
                    } else {
                        mActiveMenuIndex = 1;
                    }
                }

                // Hover switches active menu (works even when popup is open)
                if (item0_hovered && mActiveMenuIndex != 0 && mActiveMenuIndex >= 0) {
                    mActiveMenuIndex = 0;
                } else if (item1_hovered && mActiveMenuIndex != 1 && mActiveMenuIndex >= 0) {
                    mActiveMenuIndex = 1;
                }

                // Close popup if menu changed
                if (prev_menu_index != mActiveMenuIndex && prev_menu_index >= 0) {
                    ImGui::ClosePopupToLevel(0, true);
                }

                // Popup style: no rounding, tight to titlebar
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));

                // Open and render active popup
                if (mActiveMenuIndex == 0) {
                    if (!ImGui::IsPopupOpen("##AppMenu")) {
                        ImGui::SetNextWindowPos(menu_positions[0]);
                        ImGui::OpenPopup("##AppMenu");
                    }
                    if (ImGui::BeginPopup("##AppMenu")) {
                        any_popup_open = true;
                        if (ImGui::MenuItem(exit_label.c_str())) {
                            SDL_Event quit_event;
                            quit_event.type = SDL_EVENT_QUIT;
                            SDL_PushEvent(&quit_event);
                            mMenuState = TitleState::NORMAL;
                            mActiveMenuIndex = -1;
                        }
                        ImGui::EndPopup();
                    }
                } else if (mActiveMenuIndex == 1) {
                    if (!ImGui::IsPopupOpen("##HelpMenu")) {
                        ImGui::SetNextWindowPos(menu_positions[1]);
                        ImGui::OpenPopup("##HelpMenu");
                    }
                    if (ImGui::BeginPopup("##HelpMenu")) {
                        any_popup_open = true;
                        if (ImGui::MenuItem(about_label.c_str())) {
                            // TODO: Show about dialog
                            mMenuState = TitleState::NORMAL;
                            mActiveMenuIndex = -1;
                        }
                        ImGui::EndPopup();
                    }
                }

                ImGui::PopStyleVar(6);
            }
        }

        // In Hover state, check for click outside menu area -> return to Normal
        if (mMenuState == TitleState::MENU_HOVERED && ImGui::IsMouseClicked(0) && !any_menu_hovered) {
            mMenuState = TitleState::NORMAL;
        }
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Render right side buttons (always visible)
    if (!layout.right_buttons.empty()) {
        ImGui::SameLine(window_width - right_buttons_width);
        for (size_t i = 0; i < layout.right_buttons.size(); ++i) {
            if (i > 0) ImGui::SameLine(0, button_spacing);
            RenderTitleButton(layout.right_buttons[i], button_size);
        }
    }

    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(5);
}

void MainWindow::RenderTitleButton(WindowButton button, float button_size)
{
    const auto& icons = mSystemTheme.GetButtonIcons();

    switch (button) {
        case WindowButton::Close: {
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.15f, 0.15f, 1.0f));
            if (ImGui::Button(icons.close, ImVec2(button_size, button_size))) {
                SDL_Event quit_event;
                quit_event.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit_event);
            }
            ImGui::PopStyleColor(2);
            break;
        }
        case WindowButton::Minimize:
            if (ImGui::Button(icons.minimize, ImVec2(button_size, button_size))) {
                SDL_MinimizeWindow(mWindow.get());
            }
            break;
        case WindowButton::Maximize: {
            bool is_maximized = SDL_GetWindowFlags(mWindow.get()) & SDL_WINDOW_MAXIMIZED;
            if (ImGui::Button(is_maximized ? icons.restore : icons.maximize, ImVec2(button_size, button_size))) {
                if (is_maximized) {
                    SDL_RestoreWindow(mWindow.get());
                } else {
                    SDL_MaximizeWindow(mWindow.get());
                }
            }
            break;
        }
        case WindowButton::Menu:
            // Menu is rendered combined with title, not as separate button
            break;
    }
}

void MainWindow::RenderDockspace()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Adjust position and size to account for custom title bar
    ImVec2 work_pos = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + mTitleBarHeight);
    ImVec2 work_size = ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - mTitleBarHeight);

    ImGui::SetNextWindowPos(work_pos);
    ImGui::SetNextWindowSize(work_size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                  | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                  | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
                                  | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));

    ImGui::Begin("##MainContent", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Render UI panels
    mUiBuilder.Render();

    ImGui::End();
}

} // namespace scratcher::imgui
