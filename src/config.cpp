// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#include "exception"
#include "config.hpp"

#include <boost/container/flat_map.hpp>

#include <cstdlib>
#include <filesystem>

namespace {
const char * const CONF = "--config";
const char * const DATADIR = "--data-dir,-d";
const char * const VERBOSE = "--verbose,-v";
const char * const TRACE = "--debug-trace,-t";

const char* const BYBIT = "bybit";

const char* const HOST = "--host";
const char* const PORT = "--port";
}
Config::Config(int argc, const char *const argv[])
{
    std::filesystem::path home;
    if (const char* psz_home = std::getenv("HOME")) {
        home = psz_home;
    }

    std::filesystem::path data_path;
    std::filesystem::path conf_path;

    CLI::App preConf;
    preConf.set_version_flag("--version", []{ return std::string("BScratcher Wallet v.0.1"); }); // Version flag is processed here
    preConf.add_option(DATADIR, data_path, "Directory path to store wallet data")->default_val(home/".scratcher");
    preConf.add_option(CONF, conf_path, "Config file path")->default_val("config");
    try {
       preConf.parse(argc, argv);
    }
    catch (const CLI::CallForVersion& e) {
        std::cout << preConf.version() << std::endl;
        std::rethrow_exception(std::current_exception());
    }
    catch (const CLI::CallForHelp& ) {} // Skip here and next in favor for main config processing
    catch (const CLI::CallForAllHelp& ) {}
    catch (const CLI::ParseError &e) {
        preConf.exit(e);
        std::rethrow_exception(std::current_exception());
    }

    if (!conf_path.is_absolute() && conf_path.string().front() != '.')
        conf_path = data_path / conf_path;

    mApp.set_help_flag("--help,-h", "Display this help information and exit");
    mApp.set_version_flag("--version", []{ return std::string("BScratcher Wallet v.0.1"); }); // Version flag is here for help message
    mApp.set_config(CONF, conf_path, "Configuration file");
    mApp.add_option(DATADIR, mDataDir, "Directory path to store wallet data")->default_val(home/".scratcher")->configurable(false);
    mApp.add_flag(TRACE, mTrace, "Print debug traces to log");

    auto bybit = mApp.add_subcommand(BYBIT, "ByBit exchange options")->configurable()->group("Configb File Sections");
    bybit->add_option(HOST, m_host, "ByBit exchange API host")->configurable(true);
    bybit->add_option(PORT, m_port, "ByBit exchange API port")->configurable(true);

    try {
        mApp.parse(argc, argv);
    }
    catch (const CLI::ParseError &e) {
        mApp.exit(e);
        std::rethrow_exception(std::current_exception());
    }
}

