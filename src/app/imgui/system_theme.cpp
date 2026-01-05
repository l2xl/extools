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

#include "system_theme.hpp"
#include "imgui.h"
#include <array>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <regex>
#include <fontconfig/fontconfig.h>

namespace scratcher::imgui {

SystemTheme::SystemTheme()
{
    DetectTheme();
}

void SystemTheme::DetectTheme()
{
    mColorScheme = DetectColorScheme();
    mButtonLayout = DetectButtonLayout();
    mInterfaceFont = DetectInterfaceFont();
}

std::string SystemTheme::ExecuteCommand(const std::string& command) const
{
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

ColorScheme SystemTheme::DetectColorScheme()
{
    // Try freedesktop.org standard via D-Bus
    std::string cmd = "dbus-send --print-reply --dest=org.freedesktop.portal.Desktop "
                      "/org/freedesktop/portal/desktop "
                      "org.freedesktop.portal.Settings.Read "
                      "string:'org.freedesktop.appearance' string:'color-scheme' 2>/dev/null";

    std::string output = ExecuteCommand(cmd);

    // Parse D-Bus output: "variant       uint32 1" means dark mode
    if (output.find("uint32 1") != std::string::npos) {
        std::cout << "Detected dark mode via freedesktop portal" << std::endl;
        return ColorScheme::Dark;
    } else if (output.find("uint32 2") != std::string::npos) {
        std::cout << "Detected light mode via freedesktop portal" << std::endl;
        return ColorScheme::Light;
    }

    // Fallback: Try GNOME settings
    std::string gtk_theme = ExecuteCommand("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null");

    // Remove quotes from output
    gtk_theme.erase(std::remove(gtk_theme.begin(), gtk_theme.end(), '\''), gtk_theme.end());
    gtk_theme.erase(std::remove(gtk_theme.begin(), gtk_theme.end(), '\"'), gtk_theme.end());

    if (gtk_theme.find("dark") != std::string::npos || gtk_theme.find("Dark") != std::string::npos) {
        std::cout << "Detected dark mode from GTK theme: " << gtk_theme << std::endl;
        return ColorScheme::Dark;
    }

    std::cout << "Using default color scheme (dark)" << std::endl;
    return ColorScheme::Dark; // Default to dark
}

WindowButton SystemTheme::ParseButton(const std::string& button_name) const
{
    if (button_name == "close") return WindowButton::Close;
    if (button_name == "minimize") return WindowButton::Minimize;
    if (button_name == "maximize") return WindowButton::Maximize;
    if (button_name == "menu") return WindowButton::Menu;
    return WindowButton::Close; // Default
}

button_layout SystemTheme::DetectButtonLayout()
{
    button_layout layout;

    std::string cmd = "gsettings get org.gnome.desktop.wm.preferences button-layout 2>/dev/null";
    std::string output = ExecuteCommand(cmd);

    if (output.empty()) {
        // Default layout: menu on left, control buttons on right
        layout.left_buttons = {WindowButton::Menu};
        layout.right_buttons = {WindowButton::Minimize, WindowButton::Maximize, WindowButton::Close};
        std::cout << "Using default button layout (menu left, controls right)" << std::endl;
        return layout;
    }

    // Remove quotes
    output.erase(std::remove(output.begin(), output.end(), '\''), output.end());
    output.erase(std::remove(output.begin(), output.end(), '\"'), output.end());

    // Find colon separator
    size_t colon_pos = output.find(':');
    if (colon_pos == std::string::npos) {
        layout.left_buttons = {WindowButton::Menu};
        layout.right_buttons = {WindowButton::Minimize, WindowButton::Maximize, WindowButton::Close};
        return layout;
    }

    // Parse left side
    std::string left_side = output.substr(0, colon_pos);
    std::stringstream left_stream(left_side);
    std::string button;
    while (std::getline(left_stream, button, ',')) {
        if (!button.empty()) {
            layout.left_buttons.push_back(ParseButton(button));
        }
    }

    // Parse right side
    std::string right_side = output.substr(colon_pos + 1);
    std::stringstream right_stream(right_side);
    while (std::getline(right_stream, button, ',')) {
        if (!button.empty()) {
            layout.right_buttons.push_back(ParseButton(button));
        }
    }

    // If no menu button detected, add it to the left side
    bool has_menu = false;
    for (const auto& btn : layout.left_buttons) {
        if (btn == WindowButton::Menu) has_menu = true;
    }
    for (const auto& btn : layout.right_buttons) {
        if (btn == WindowButton::Menu) has_menu = true;
    }
    if (!has_menu) {
        layout.left_buttons.push_back(WindowButton::Menu);
    }

    std::cout << "Button layout: " << left_side << " : " << right_side << std::endl;
    return layout;
}

std::string SystemTheme::FindFontFile(const std::string& font_family)
{
    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config) {
        std::cerr << "Failed to initialize fontconfig" << std::endl;
        return "";
    }

    // Create pattern for font family
    FcPattern* pat = FcNameParse((const FcChar8*)font_family.c_str());
    if (!pat) {
        std::cerr << "Failed to parse font pattern: " << font_family << std::endl;
        FcConfigDestroy(config);
        return "";
    }

    // Apply configuration substitutions
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    // Find best matching font
    FcResult result;
    FcPattern* font = FcFontMatch(config, pat, &result);

    std::string font_path;
    if (font) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            font_path = (const char*)file;
            std::cout << "Found font file for '" << font_family << "': " << font_path << std::endl;
        } else {
            std::cerr << "Could not get font file path for: " << font_family << std::endl;
        }
        FcPatternDestroy(font);
    }

    FcPatternDestroy(pat);
    FcConfigDestroy(config);

    return font_path;
}

font_info SystemTheme::DetectInterfaceFont()
{
    font_info info;

    std::string cmd = "gsettings get org.gnome.desktop.interface font-name 2>/dev/null";
    std::string output = ExecuteCommand(cmd);

    if (output.empty()) {
        info.family = "Sans";
        info.size = 11;
        std::cout << "Using default font: Sans 11" << std::endl;
    } else {
        // Remove quotes
        output.erase(std::remove(output.begin(), output.end(), '\''), output.end());
        output.erase(std::remove(output.begin(), output.end(), '\"'), output.end());

        // Parse font string like "Ubuntu 11" or "Sans 10"
        size_t last_space = output.find_last_of(' ');
        if (last_space != std::string::npos) {
            info.family = output.substr(0, last_space);
            try {
                info.size = std::stoi(output.substr(last_space + 1));
            } catch (...) {
                info.size = 11;
            }
        } else {
            info.family = output;
            info.size = 11;
        }

        std::cout << "Detected system font: " << info.family << " " << info.size << std::endl;
    }

    // Find font file path using fontconfig
    info.file_path = FindFontFile(info.family);

    return info;
}

float SystemTheme::CalculatePPIFromXrandr() const
{
    // Get xrandr output for primary/connected displays
    std::string output = ExecuteCommand("xrandr | grep -A 1 \" connected\" | head -10");

    if (output.empty()) {
        return 96.0f; // Default fallback
    }

    // Parse xrandr output: "DP-3 connected 3840x2160+0+0 ... 600mm x 340mm"
    // Looking for pattern: WIDTHxHEIGHT ... WIDTHmm x HEIGHTmm
    std::regex pattern(R"((\d+)x(\d+)\+\d+\+\d+[^\n]*?(\d+)mm x (\d+)mm)");
    std::smatch matches;

    if (std::regex_search(output, matches, pattern)) {
        int width_px = std::stoi(matches[1].str());
        int height_px = std::stoi(matches[2].str());
        int width_mm = std::stoi(matches[3].str());
        int height_mm = std::stoi(matches[4].str());

        if (width_mm > 0 && height_mm > 0) {
            // Calculate PPI using horizontal dimension (more commonly used)
            float width_inches = width_mm / 25.4f;
            float ppi = width_px / width_inches;

            std::cout << "Calculated PPI from xrandr: " << ppi
                      << " (" << width_px << "x" << height_px << " @ "
                      << width_mm << "x" << height_mm << "mm)" << std::endl;

            return ppi;
        }
    }

    // Fallback: try to detect common high-res patterns from resolution alone
    std::regex res_pattern(R"((\d+)x(\d+))");
    if (std::regex_search(output, matches, res_pattern)) {
        int width = std::stoi(matches[1].str());
        int height = std::stoi(matches[2].str());

        // Heuristic: 4K resolution is likely high-DPI
        if ((width >= 3840 && height >= 2160) || (width >= 2560 && height >= 1440)) {
            std::cout << "Detected high-resolution pattern: " << width << "x" << height
                      << " (likely high-DPI)" << std::endl;
            return 180.0f; // Assume ~180 PPI for 4K on typical monitor
        }
    }

    return 96.0f; // Standard DPI fallback
}

float SystemTheme::CalculateScaleFromPPI(float ppi) const
{
    // PPI thresholds based on research:
    // - 96 PPI: standard DPI (scale 1.0)
    // - 144 PPI: 1.5× scaling threshold
    // - 192 PPI: 2.0× scaling (Retina-like)
    // - 288 PPI: 3.0× scaling (high-end Retina)

    if (ppi < 120.0f) return 1.0f;  /* Standard DPI */
    if (ppi < 168.0f) return 1.5f;  /* High-DPI, 1.5× scaling */
    if (ppi < 240.0f) return 2.0f;  // Retina-class, 2× scaling
    return 3.0f;  // Very high-DPI, 3× scaling
}

display_scale_info SystemTheme::DetectDisplayScale() const
{
    display_scale_info info;

    // Method 1: Try to detect via GNOME gsettings
    std::string cmd = "gsettings get org.gnome.desktop.interface scaling-factor 2>/dev/null";
    std::string output = ExecuteCommand(cmd);

    if (!output.empty()) {
        // Output format: "uint32 2" or just "2"
        try {
            // Extract number from string
            size_t pos = output.find_last_of(' ');
            if (pos != std::string::npos) {
                output = output.substr(pos + 1);
            }
            int scale_int = std::stoi(output);
            if (scale_int > 1) {
                info.content_scale = static_cast<float>(scale_int);
                info.is_hidpi = true;
                info.detection_method = "gsettings";
                std::cout << "Detected display scale from gsettings: " << info.content_scale << std::endl;
                return info;
            }
        } catch (...) {
            // Failed to parse, continue to fallback
        }
    }

    // Method 2: Check GDK_SCALE environment variable
    const char* gdk_scale = std::getenv("GDK_SCALE");
    if (gdk_scale) {
        try {
            int scale_int = std::stoi(gdk_scale);
            if (scale_int > 1) {
                info.content_scale = static_cast<float>(scale_int);
                info.is_hidpi = true;
                info.detection_method = "GDK_SCALE";
                std::cout << "Detected display scale from GDK_SCALE: " << info.content_scale << std::endl;
                return info;
            }
        } catch (...) {
            // Failed to parse
        }
    }

    // Method 3: Physical PPI calculation (for information only)
    // Calculate PPI but don't override scale 1.0 from system
    // Scale 1.0 is likely intentional user configuration
    float ppi = CalculatePPIFromXrandr();
    info.calculated_ppi = ppi;
    info.content_scale = 1.0f;
    info.is_hidpi = (ppi >= 150.0f);
    info.detection_method = "system_default";

    std::cout << "Using system default scale: 1.0 (calculated PPI: " << ppi << ")" << std::endl;

    return info;
}

void SystemTheme::ApplyToImGui(float dpi_scale)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Apply color scheme
    if (mColorScheme == ColorScheme::Dark) {
        ImGui::StyleColorsDark();

        // Apply GNOME-inspired dark colors
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.95f);
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.42f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.42f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    } else {
        ImGui::StyleColorsLight();
    }

    // Apply rounded corners like modern GNOME apps
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    // Apply spacing similar to GNOME
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

    // Apply DPI scaling to all size-related style values
    // Only apply if scale > 1.0 to respect user's display settings
    if (dpi_scale > 1.01f) {  // Use 1.01 to account for float precision
        style.ScaleAllSizes(dpi_scale);
        std::cout << "Applied DPI scale: " << dpi_scale << " to ImGui style" << std::endl;
    } else {
        std::cout << "DPI scale is 1.0, not applying ScaleAllSizes()" << std::endl;
    }

    std::cout << "Applied system theme styling to ImGui" << std::endl;
}

} // namespace scratcher::imgui
