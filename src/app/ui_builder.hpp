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

namespace scratcher::ui {

using panel_id = size_t;
constexpr panel_id INVALID_PANEL = 0;

class IUiBuilder
{
public:
    using instrument_list = std::vector<std::string>;
    using on_instrument_selected_t = std::function<void(panel_id, const std::string&)>;
    using on_tab_closed_t = std::function<void(panel_id)>;
    using on_add_tab_t = std::function<void(panel_id)>;

    virtual ~IUiBuilder() = default;

    virtual bool IsReady() const = 0;

    virtual panel_id CreateTabBar() = 0;
    virtual panel_id AddMarketTab(panel_id tab_bar_id, const std::optional<std::string>& symbol = std::nullopt) = 0;

    virtual void SetTabSymbol(panel_id tab_id, const std::string& symbol) = 0;
    virtual void CloseTab(panel_id tab_id) = 0;

    virtual void SetInstruments(instrument_list instruments) = 0;

    virtual void SetOnInstrumentSelected(on_instrument_selected_t handler) = 0;
    virtual void SetOnTabClosed(on_tab_closed_t handler) = 0;
    virtual void SetOnAddTab(on_add_tab_t handler) = 0;
};

} // namespace scratcher::ui
