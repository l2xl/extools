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

#ifndef INSTRUMENT_DROPBOX_H
#define INSTRUMENT_DROPBOX_H

#include <deque>
#include <memory>

#include <QComboBox>
#include <QMetaType>

#include "bybit/entities/instrument.hpp"

namespace scratcher {

using instrument_ptr = std::shared_ptr<bybit::InstrumentInfo>;
using instrument_list_ptr = std::shared_ptr<std::deque<bybit::InstrumentInfo>>;

class InstrumentDropBox : public QComboBox
{
    Q_OBJECT

    instrument_list_ptr m_instruments;

public:
    explicit InstrumentDropBox(QWidget* parent = nullptr);

public slots:
    void setInstruments(instrument_list_ptr instruments);

signals:
    void instrumentSelected(instrument_ptr instrument);

private slots:
    void onCurrentIndexChanged(int index);
};

} // namespace scratcher

Q_DECLARE_METATYPE(scratcher::instrument_ptr)
Q_DECLARE_METATYPE(scratcher::instrument_list_ptr)

#endif // INSTRUMENT_DROPBOX_H
