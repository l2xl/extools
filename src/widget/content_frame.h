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

#ifndef CONTENT_FRAME_WIDGET_H
#define CONTENT_FRAME_WIDGET_H

#include <memory>

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>


class ContentFrameWidget : public QFrame
{
    Q_OBJECT

private:
    std::unique_ptr<QLabel> m_titleLabel;
    std::unique_ptr<QVBoxLayout> m_contentLayout;
    std::unique_ptr<QFrame> m_contentWidget;

public:
    explicit ContentFrameWidget(QWidget* parent = nullptr);

    void setTitle(const QString& title)
    { if (m_titleLabel) m_titleLabel->setText(title); }
    
    void setContent(std::unique_ptr<QFrame> widget);

signals:
    void settingsRequested();

};


#endif // CONTENT_FRAME_WIDGET_H
