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
#include <SQLiteCpp/SQLiteCpp.h>

// SQLiteCpp requires this assertion handler when SQLITECPP_ENABLE_ASSERT_HANDLER is defined
namespace SQLite {
    void assertion_failed(const char* apFile, int apLine, const char* apFunc, const char* apExpr, const char* apMsg) {
        std::cerr << "SQLite assertion failed: " << apFile << ":" << apLine << " in " << apFunc << "() - " << apExpr;
        if (apMsg) std::cerr << " (" << apMsg << ")";
        std::cerr << std::endl;
        std::abort();
    }
}

int main(int argc, char *argv[])
{
    try {
        QApplication a(argc, argv);

        auto config = std::make_shared<Config>(argc, argv);

        auto scheduler = scratcher::scheduler::create(2);

        // Create SQLite database for datahub persistence
        auto database = std::make_shared<SQLite::Database>(
            config->DataDir() + "/market_data.sqlite",
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        auto w = TradeCockpitWindow::Create(scheduler, config, database);

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
