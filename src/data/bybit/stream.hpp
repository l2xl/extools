// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

//
// Created by l2xl on 06.03.25.
//

#ifndef BYBIT_STREAM_HPP
#define BYBIT_STREAM_HPP

#include <chrono>
#include <deque>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

namespace scratcher::bybit {

namespace ip = boost::asio::ip;
namespace websock = boost::beast::websocket;

using boost::asio::io_context;
using boost::asio::yield_context;

class ByBitApi;

class ByBitStream: public std::enable_shared_from_this<ByBitStream>
{
    enum class status {READY, MESSAGE, CLOSING};

    const std::shared_ptr<ByBitApi> m_api;
    const std::string m_path_spec;
    std::atomic<status> m_status = status::READY;

    websock::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> m_websock;
    boost::asio::steady_timer m_heartbeat_timer;
    std::atomic<std::chrono::system_clock::time_point> m_last_heartbeat = std::chrono::system_clock::time_point::min();

    std::deque<std::string> m_message_queue;
    std::mutex m_queue_mutex;

    void Heartbeat();
    void DoOpenWebSocketStream(yield_context &yield);
    void DoWriteReadWebSocketStream(yield_context &yield);

public:
    ByBitStream(std::shared_ptr<ByBitApi> api, std::string spec, std::string symbol);
    static std::shared_ptr<ByBitStream> Create(std::shared_ptr<ByBitApi> api, std::string path_spec, std::string symbol);

    void Message(std::string message);

};

}

#endif //BYBIT_STREAM_HPP
