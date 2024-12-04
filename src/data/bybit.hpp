// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit

#ifndef MARKET_DATA_SINK_HPP
#define MARKET_DATA_SINK_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

#include <memory>

class Config;

namespace ssl = boost::asio::ssl;
namespace ip = boost::asio::ip;
namespace websock = boost::beast::websocket;

class ByBitApi
{
    std::string m_host;
    std::string m_port;

    boost::asio::io_context io_ctx;
    ssl::context ssl_ctx;

    ip::tcp::resolver::results_type m_resolved_host;

    // websock::stream<boost::beast::ssl_stream<ip::tcp::socket>> m_sock{ io_ctx, ssl_ctx };
    //
    // boost::beast::flat_buffer m_sock_buf;

    long last_server_time = 0;

public:
    explicit ByBitApi(std::shared_ptr<Config> config);

    void requestServerTime(){}

};



#endif //MARKET_DATA_SINK_HPP
