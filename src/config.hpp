// Scratcher project
// Copyright (c) 2024-2025 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit


#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "cli11/CLI11.hpp"
#include "bybit.hpp"

class Config : public scratcher::bybit::Config{
    CLI::App mApp;

    size_t mVerbose;
    bool mTrace;

    std::string mDataDir;

    std::string m_http_host;
    std::string m_http_port;

    std::string m_stream_host;
    std::string m_stream_port;

public:
    Config() = delete;
    Config(int argc, const char *const argv[]);

    size_t Verbose() const { return mVerbose; }
    bool Trace() const {return mTrace; }
    const std::string& DataDir() const { return mDataDir; }

    const std::string& HttpHost() const override { return m_http_host; }
    const std::string& HttpPort() const override { return m_http_port; }

    const std::string& StreamHost() const override { return m_stream_host; }
    const std::string& StreamPort() const override { return m_stream_port; }
};


#endif //CONFIG_HPP
