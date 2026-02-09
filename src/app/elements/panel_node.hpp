// Scratcher project
// Copyright (c) 2025-2026 l2xl (l2xl/at/proton.me)
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
#include <functional>

#include <elements.hpp>
#include "content_panel.hpp"
#include "split_direction.hpp"

namespace scratcher::elements {

class PanelNode
{
public:
    virtual ~PanelNode() = default;

    virtual bool IsLeaf() const = 0;

    cycfi::elements::element_ptr GetElement();

protected:
    explicit PanelNode(std::weak_ptr<cycfi::elements::view> view);

    std::shared_ptr<cycfi::elements::view> View() const;
    void RefreshDeck(cycfi::elements::element_ptr content);

    std::shared_ptr<cycfi::elements::deck_composite> mDeck;
    std::weak_ptr<cycfi::elements::view> mView;
};

class LeafPanelNode : public PanelNode
{
public:
    using PanelType = cockpit::PanelType;
    using panel_id = cockpit::panel_id;

    LeafPanelNode(std::weak_ptr<cycfi::elements::view> view, PanelType type);
    ~LeafPanelNode() override;

    void Initialize(cycfi::elements::element_ptr content, panel_id pid, std::function<void()> cleanup);

    bool IsLeaf() const override { return true; }

    PanelType Type() const { return mType; }
    panel_id PanelId() const { return mPanelId; }

private:
    PanelType mType;
    panel_id mPanelId = 0;
    std::function<void()> mCleanup;
};

class SplitPanelNode : public PanelNode
{
public:
    SplitPanelNode(std::weak_ptr<cycfi::elements::view> view, SplitDirection direction,
                   std::shared_ptr<PanelNode> first, std::shared_ptr<PanelNode> second );
    ~SplitPanelNode() override = default;

    bool IsLeaf() const override { return false; }

    SplitDirection Direction() const { return mDirection; }
    std::shared_ptr<PanelNode> First() const { return mFirst; }
    std::shared_ptr<PanelNode> Second() const { return mSecond; }

    void ReplaceChild(std::shared_ptr<PanelNode> oldChild, std::shared_ptr<PanelNode> newChild);

private:
    void BuildLayout();

    SplitDirection mDirection;
    std::shared_ptr<PanelNode> mFirst;
    std::shared_ptr<PanelNode> mSecond;
};

} // namespace scratcher::elements
