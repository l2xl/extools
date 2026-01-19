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
#include <elements.hpp>
#include "ui_builder.hpp"

namespace scratcher::elements {

class MainWindow
{
public:
    explicit MainWindow();
    ~MainWindow();

    int Run();

    UiBuilder& GetUiBuilder() { return mUiBuilder; }

private:
    void SetupContent();
    cycfi::elements::element_ptr MakeAppBar();
    cycfi::elements::element_ptr MakeMenuItems();

    cycfi::elements::app mApp;
    cycfi::elements::window mWindow;
    std::unique_ptr<cycfi::elements::view> mView;

    UiBuilder mUiBuilder;

    bool mMenuVisible = false;
    cycfi::elements::element_ptr mAppDeck;
};

} // namespace scratcher::elements
