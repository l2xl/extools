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

#include "panel_node.hpp"

#include <stdexcept>

namespace scratcher::elements {

namespace el = cycfi::elements;

namespace {
el::color divider_color = el::rgba(80, 80, 90, 255);
} // anonymous namespace

// --- PanelNode ---

PanelNode::PanelNode(std::weak_ptr<el::view> view)
    : mView(std::move(view))
    , mDeck(std::make_shared<el::deck_composite>())
{
    mDeck->push_back(el::share(el::element{}));
    mDeck->select(0);
}

std::shared_ptr<el::view> PanelNode::View() const
{
    auto v = mView.lock();
    if (!v) throw std::logic_error("PanelNode: view expired");
    return v;
}

el::element_ptr PanelNode::GetElement()
{
    return el::share(el::hold(mDeck));
}

void PanelNode::RefreshDeck(el::element_ptr content)
{
    mDeck->clear();
    mDeck->push_back(std::move(content));
    mDeck->select(0);

    auto v = View();
    v->layout();
    v->refresh();
}

// --- LeafPanelNode ---

LeafPanelNode::LeafPanelNode(std::weak_ptr<el::view> view, PanelType type)
    : PanelNode(std::move(view))
    , mType(type)
{}

LeafPanelNode::~LeafPanelNode()
{
    if (mCleanup) mCleanup();
}

void LeafPanelNode::Initialize(el::element_ptr content, panel_id pid, std::function<void()> cleanup)
{
    mPanelId = pid;
    mCleanup = std::move(cleanup);
    RefreshDeck(std::move(content));
}

// --- SplitPanelNode ---

SplitPanelNode::SplitPanelNode(std::weak_ptr<el::view> view, SplitDirection direction,
                               std::shared_ptr<PanelNode> first, std::shared_ptr<PanelNode> second)
    : PanelNode(std::move(view)) , mDirection(direction) , mFirst(std::move(first)) , mSecond(std::move(second))
{
    BuildLayout();
}

void SplitPanelNode::ReplaceChild(std::shared_ptr<PanelNode> oldChild, std::shared_ptr<PanelNode> newChild)
{
    if (mFirst == oldChild)
        mFirst = std::move(newChild);
    else if (mSecond == oldChild)
        mSecond = std::move(newChild);

    BuildLayout();
}

void SplitPanelNode::BuildLayout()
{
    el::element_ptr layout;

    if (mDirection == SplitDirection::Vertical) {
        layout = el::share(
            el::htile(
                el::hstretch(1.0, el::hold(mFirst->GetElement())),
                el::vmin_size(1, el::hsize(4, el::box(divider_color))),
                el::hstretch(1.0, el::hold(mSecond->GetElement()))
            )
        );
    } else {
        layout = el::share(
            el::vtile(
                el::vstretch(1.0, el::hold(mFirst->GetElement())),
                el::hmin_size(1, el::vsize(4, el::box(divider_color))),
                el::vstretch(1.0, el::hold(mSecond->GetElement()))
            )
        );
    }

    mDeck->clear();
    mDeck->push_back(std::move(layout));
    mDeck->select(0);
}

} // namespace scratcher::elements
