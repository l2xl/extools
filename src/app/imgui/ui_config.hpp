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
#include <variant>
#include <memory>

namespace scratcher::imgui {

struct ui_panel;

struct instrument_panel
{
    std::string type = "instrument";
    std::string symbol;
};

struct grid_layout
{
    std::string type = "grid";
    int rows = 1;
    int cols = 1;
    std::vector<std::shared_ptr<ui_panel>> panels;
};

struct hsplit_layout
{
    std::string type = "hsplit";
    std::vector<std::shared_ptr<ui_panel>> panels;
};

struct vsplit_layout
{
    std::string type = "vsplit";
    std::vector<std::shared_ptr<ui_panel>> panels;
};

struct tabs_layout
{
    std::string type = "tabs";
    std::vector<std::shared_ptr<ui_panel>> panels;
};

using panel_variant = std::variant<
    instrument_panel,
    grid_layout,
    hsplit_layout,
    vsplit_layout,
    tabs_layout
>;

struct ui_panel
{
    panel_variant content;
};

struct ui_config
{
    ui_panel root;
};

} // namespace scratcher::imgui
