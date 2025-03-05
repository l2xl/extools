// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

//
// Created by l2xl on 06.03.25.
//


#include "bybit/stream.hpp"
#include <iostream>

#include "bybit.hpp"

namespace scratcher::bybit {

const char* const MESSAGE_PING = R"({"op": "ping"})";


ByBitStream::ByBitStream(std::shared_ptr<ByBitApi> api, std::string spec, std::string symbol)
    : m_api(move(api)), m_path_spec(move(spec))
    , m_websock(m_api->mScheduler->io(), m_api->mScheduler->ssl())
    , m_heartbeat_timer(m_api->mScheduler->io(), seconds(10))
{
    std::string ordbook = "orderbook.50." + symbol;
    std::string pubtrade = "publicTrade." + symbol;

    nlohmann::json request = {{"req_id","init_subscruption"},{"op", "subscribe"}, {"args", {ordbook, pubtrade}}};
    std::clog << "Subscription message to " << m_api->mConfig->StreamHost() << ":\n" << request.dump(2) << std::endl;
    m_message_queue.emplace_back(request.dump());
    m_status = status::MESSAGE;
}

std::shared_ptr<ByBitStream> ByBitStream::Create(std::shared_ptr<ByBitApi> api, std::string spec, std::string symbol)
{
    auto self = std::make_shared<ByBitStream>(api, move(spec), move(symbol));
    std::weak_ptr ref = self;
    api->Spawn([ref](yield_context yield) {
        if (auto self = ref.lock()) {
            self->DoOpenWebSocketStream(yield);
            if (!*yield.ec_) {
                self->DoWriteReadWebSocketStream(yield);
            }
        }
    });

    return self;
}

void ByBitStream::DoOpenWebSocketStream(yield_context &yield)
{
    if (!m_api->m_server_time_delta) {
        *yield.ec_ = xscratcher_error_code(error::no_time_sync);
        return;
    }

    if (m_api->m_resolved_websock_host.empty()) {
        *yield.ec_ = xscratcher_error_code(error::no_host_name);
        return;
    }

    if (m_websock.is_open()) {
        *yield.ec_ = xscratcher_error_code(error::already_opened);
        return;
    }

    get_lowest_layer(m_websock).expires_after(seconds(30));

    auto connect_result = get_lowest_layer(m_websock).async_connect(m_api->m_resolved_websock_host, yield);
    if (*yield.ec_) {
        std::cerr << "stream connect error: ";
        return;
    }

    if (!SSL_set_tlsext_host_name(m_websock.next_layer().native_handle(), m_api->mConfig->StreamHost().c_str()))
        throw std::ios_base::failure("Failed to set SNI Hostname");

    get_lowest_layer(m_websock).expires_after(seconds(30));

    m_websock.next_layer().async_handshake(ssl::stream_base::client, yield);
    if (*yield.ec_) {
        std::cerr << "ssl handshake error: ";
        return;
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    get_lowest_layer(m_websock).expires_never();

    // Set suggested timeout settings for the websocket
    m_websock.set_option(websock::stream_base::timeout::suggested(boost::beast::role_type::client));

    m_websock.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req)
        {
            req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
        }));

    std::string host_port = m_api->mConfig->StreamHost() + ":" + std::to_string(connect_result.port());

    std::clog << "WebSock handshake: " << host_port << "  " << m_path_spec << std::endl;

    m_websock.async_handshake(host_port, m_path_spec, yield);
    if (*yield.ec_) {
        std::cerr << "web-sock handshake error: " << yield.ec_->message() << std::endl;
        return;
    }

    m_last_heartbeat = std::chrono::system_clock::now();
    Heartbeat();
}

void ByBitStream::DoWriteReadWebSocketStream(yield_context &yield)
{
    while (true) {
        while (true) {
            std::string message;
            {
                auto lock = std::unique_lock(m_queue_mutex);
                if (m_message_queue.empty()) {

                    status cur_status = status::MESSAGE;
                    m_status.compare_exchange_strong(cur_status, status::READY);

                   break;
                }

                message = std::move(m_message_queue.front());
                m_message_queue.pop_front();
            }

            std::clog << "web-sock message:\n" << message << std::endl;

            m_websock.async_write(boost::asio::buffer(message), yield);
            if (*yield.ec_) {
                m_status = status::CLOSING;

                std::cerr << "web-sock write error: " << yield.ec_->message() << std::endl;

                // Place the message back to the queue
                auto lock = std::unique_lock(m_queue_mutex);
                m_message_queue.emplace_front(move(message));

                break;
            }
            m_last_heartbeat = std::chrono::system_clock::now();
        }

        boost::beast::flat_buffer buffer;

        while (true) {
            if (m_status != status::READY)
                break;

            std::clog << "web-sock read..." << std::endl;

            m_websock.async_read(buffer, yield);

            if (*yield.ec_) {
                m_status = status::CLOSING;
                break;
            }

            std::string data = boost::beast::buffers_to_string(buffer.data());
            std::cout << data << std::endl;
            buffer.clear();

            m_last_heartbeat = std::chrono::system_clock::now();
        }

        if (m_status == status::CLOSING) break;
    }

    if (m_websock.is_open()) m_websock.async_close(boost::beast::websocket::close_code::internal_error, yield);
}

void ByBitStream::Heartbeat()
{
    if (std::chrono::duration_cast<seconds>(std::chrono::system_clock::now() - m_last_heartbeat.load()) > seconds(20)) {
        Message(MESSAGE_PING);
    }
    else {
        auto ref = weak_from_this();
        m_heartbeat_timer.async_wait([=](boost::system::error_code status) {
            if (auto self = ref.lock()) self->Heartbeat();
        });
    }
}

void ByBitStream::Message(std::string message)
{
    auto lock = std::unique_lock(m_queue_mutex);
    m_message_queue.emplace_back(move(message));
    status cur_status = status::READY;
    if (!m_status.compare_exchange_strong(cur_status, status::MESSAGE)) {
        if (cur_status != status::CLOSING && cur_status != status::MESSAGE)
            throw std::runtime_error("Bad web-sock stream status: " + std::to_string((int)cur_status));
    }
}


}
