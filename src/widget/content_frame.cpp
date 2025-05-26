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

#include <QPushButton>
#include <QWidget>

#include "content_frame.h"


ContentFrameWidget::ContentFrameWidget(QWidget *parent): QFrame(parent)
{
    setMinimumSize(300, 200);

    // Create a layout for this widget
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create a header widget
    auto headerWidget = new QWidget(this);
    headerWidget->setMinimumHeight(32);
    headerWidget->setMaximumHeight(32);
    headerWidget->setAutoFillBackground(true);

    // Create header layout
    auto headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(10, 0, 5, 0);
    headerLayout->setSpacing(5);

    // Add title label
    m_titleLabel = std::make_unique<QLabel>("Panel", headerWidget);
    headerLayout->addWidget(m_titleLabel.get(), 1);

    // Add settings button
    auto settingsButton = new QPushButton("⚙", headerWidget);
    settingsButton->setMaximumSize(24, 24);
    headerLayout->addWidget(settingsButton);

    // Connect settings button
    connect(settingsButton, &QPushButton::clicked, this, &ContentFrameWidget::settingsRequested);

    // Create content widget
    auto contentWidget = new QWidget(this);
    contentWidget->setAutoFillBackground(true);
    m_contentLayout = std::make_unique<QVBoxLayout>(contentWidget);
    m_contentLayout->setContentsMargins(5, 5, 5, 5);

    // Add widgets to main layout
    layout->addWidget(headerWidget);
    layout->addWidget(contentWidget, 1);
}

void ContentFrameWidget::setContent(std::unique_ptr<QFrame> widget)
{
    // Remove any existing widgets
    if (m_contentWidget) {
        m_contentLayout->removeWidget(m_contentWidget.get());
        m_contentWidget.reset(); // Release the old widget
    }

    // Take ownership of the new widget if it's not null
    if (widget) {
        m_contentWidget = std::move(widget);

        m_contentWidget->setAutoFillBackground(true);

        // Add the widget to the layout
        m_contentLayout->addWidget(m_contentWidget.get());
    }
}

