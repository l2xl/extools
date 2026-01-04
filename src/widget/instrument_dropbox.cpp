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

#include "instrument_dropbox.h"

namespace scratcher {

InstrumentDropBox::InstrumentDropBox(QWidget* parent)
    : QComboBox(parent)
{
    connect(this, &QComboBox::currentIndexChanged,
            this, &InstrumentDropBox::onCurrentIndexChanged);
}

void InstrumentDropBox::setInstruments(instrument_list_ptr instruments)
{
    blockSignals(true);
    clear();
    m_instruments = instruments;

    if (m_instruments) {
        for (size_t i = 0; i < m_instruments->size(); ++i) {
            addItem(QString::fromStdString((*m_instruments)[i].symbol),
                    static_cast<int>(i));
        }
    }

    blockSignals(false);

    if (count() > 0) {
        setCurrentIndex(0);
    }
}

void InstrumentDropBox::onCurrentIndexChanged(int index)
{
    if (m_instruments && index >= 0 && static_cast<size_t>(index) < m_instruments->size()) {
        auto selected = std::make_shared<bybit::InstrumentInfo>((*m_instruments)[index]);
        emit instrumentSelected(std::move(selected));
    }
}

} // namespace scratcher
