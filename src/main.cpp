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

#include "trade_cockpit.h"
#include "config.hpp"

#include <QApplication>

#include "scheduler.hpp"
#include "bybit.hpp"

int main(int argc, char *argv[])
{
    try {
        QApplication a(argc, argv);
        
        auto config = std::make_shared<Config>(argc, argv);

        auto scheduler = scratcher::AsioScheduler::Create(2);

        auto bybit = scratcher::bybit::ByBitApi::Create(config, scheduler);

        auto w = TradeCockpitWindow::Create(scheduler, bybit);

        return a.exec();
    }
    catch(std::system_error& e) {
        std::cerr << "System error: " << e.what() << " ("  << e.code() << ')' << std::endl;
        return -1;
    }
    catch(boost::system::system_error& e) {
        std::cerr << "System error: " << e.what() << " ("  << e.code() << ')' << std::endl;
        return -1;
    }
    catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    catch(...) {
        std::cerr << "Unknown error" << std::endl;
        return -1;
    }
}
