// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit


#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "cli11/CLI11.hpp"


class Config {
    CLI::App mApp;

    size_t mVerbose;
    bool mTrace;

    std::string mDataDir;

    std::string m_host;
    std::string m_port;

public:
    Config() = delete;
    Config(int argc, const char *const argv[]);

    size_t Verbose() const { return mVerbose; }
    bool Trace() const {return mTrace; }
    const std::string& DataDir() const { return mDataDir; }

    const std::string& Host() const { return m_host; }
    const std::string& Port() const { return m_port; }
};


#endif //CONFIG_HPP
