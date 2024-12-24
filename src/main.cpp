// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "mainwindow.h"
#include "config.hpp"

#include "scheduler.hpp"
#include "bybit.hpp"

#include <QApplication>

#include <boost/system.hpp>

int main(int argc, char *argv[])
{
    try {
        QApplication a(argc, argv);

        auto config = std::make_shared<Config>(argc, argv);

        auto scheduler = std::make_shared<scratcher::AsioScheduler>();

        auto mdSink = std::make_shared<bybit::ByBitApi>(config, scheduler);
        MainWindow w(mdSink, scheduler);
        w.show();
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
