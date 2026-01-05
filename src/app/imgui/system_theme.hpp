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

namespace scratcher::imgui {

enum class ColorScheme
{
    NoPreference = 0,
    Dark = 1,
    Light = 2
};

enum class WindowButton
{
    Close,
    Minimize,
    Maximize,
    Menu
};

struct button_layout
{
    std::vector<WindowButton> left_buttons;
    std::vector<WindowButton> right_buttons;
};

struct font_info
{
    std::string family;
    int size = 11;
    std::string file_path;
};

struct display_scale_info
{
    float content_scale = 1.0f;  // Display content scale (1.0, 2.0 for Retina/4K)
    bool is_hidpi = false;        // Whether display is high-DPI
    float calculated_ppi = 96.0f; // Calculated physical PPI
    std::string detection_method; // How DPI was detected (for debugging)
};

struct button_icons
{
    const char* close = "×";
    const char* minimize = "−";
    const char* maximize = "□";
    const char* restore = "❐";
    const char* menu = "☰";
};

class SystemTheme
{
public:
    SystemTheme();

    void DetectTheme();
    void ApplyToImGui(float dpi_scale);

    ColorScheme GetColorScheme() const { return mColorScheme; }
    const button_layout& GetButtonLayout() const { return mButtonLayout; }
    const font_info& GetInterfaceFont() const { return mInterfaceFont; }
    const button_icons& GetButtonIcons() const { return mButtonIcons; }
    display_scale_info DetectDisplayScale() const;
    bool IsDarkMode() const { return mColorScheme == ColorScheme::Dark; }

private:
    ColorScheme DetectColorScheme();
    button_layout DetectButtonLayout();
    font_info DetectInterfaceFont();
    std::string FindFontFile(const std::string& font_family);

    float CalculatePPIFromXrandr() const;
    float CalculateScaleFromPPI(float ppi) const;

    std::string ExecuteCommand(const std::string& command) const;
    WindowButton ParseButton(const std::string& button_name) const;

    ColorScheme mColorScheme = ColorScheme::NoPreference;
    button_layout mButtonLayout;
    font_info mInterfaceFont;
    button_icons mButtonIcons;
};

} // namespace scratcher::imgui
