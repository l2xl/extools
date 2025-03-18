// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include "bybit/stream.hpp"
#include <iostream>

#include "bybit.hpp"

namespace scratcher::bybit {

const char* const MESSAGE_PING = R"({"op": "ping"})";

SubscriptionTopic SubscriptionTopic::Parse(std::string topic)
{
    std::istringstream buf(topic);
    std::string elem;
    std::vector<std::string> parts;
    while (std::getline(buf, elem, '.')) parts.push_back(elem);

    switch (parts.size()) {
    case 1:
        return SubscriptionTopic(move(topic));
    case 2:
        return SubscriptionTopic(move(parts[0]), move(parts[1]));
    case 3:
        try {
            return SubscriptionTopic(move(parts[0]), boost::lexical_cast<size_t>(parts[1]), move(parts[2]));
        }
        catch (...) {
            std::throw_with_nested(SubscriptionTopicFormatError(move(topic)));
        }
    default:
        throw SubscriptionTopicFormatError(move(topic));
    }
}


ByBitStream::ByBitStream(std::shared_ptr<ByBitApi> api, std::string spec, std::function<void(std::string&&)> callback, std::function<void(boost::system::error_code)> error_callback)
    : m_api(api), m_path_spec(move(spec)), m_status(status::INIT)
    , m_strand(make_strand(api->Scheduler()->io()))
    , m_heartbeat_timer(m_strand, seconds(15))
    , m_data_callback(move(callback)), m_error_callback(move(error_callback))
{
}

ByBitStream::~ByBitStream()
{
    if (m_websock && m_websock->is_open())
        m_websock->close(boost::beast::websocket::close_code::normal);
}

void ByBitStream::Spawn()
{
    spawn(m_strand, [ref = weak_from_this()](yield_context yield) {
        if (auto self = ref.lock()) {
            for (;;) {
                boost::system::error_code ec;
                self->DoOpenWebSocketStream(yield[ec]);
                if (ec) {
                    std::cerr << ec.message() << std::endl;
                    ec.clear();

                    boost::asio::steady_timer t(self->m_strand, milliseconds(500));
                    t.async_wait(yield[ec]);

                    if (!ec) continue;

                    // Timer error case
                    std::cerr << "Repeat timer error: " << ec.message() << std::endl;
                    return;
                }
                break;
            }
            self->DoReadWebSocketStream(yield);
        }
    });
    Heartbeat();
}

void ByBitStream::DoOpenWebSocketStream(yield_context yield)
{
    auto api = m_api.lock();
    if (!api) return;

    // if (!api->m_server_time_delta) {
    //     *yield.ec_ = xscratcher_error_code(error::no_time_sync);
    //     return;
    // }

    if (api->m_resolved_websock_host.empty()) {
        *yield.ec_ = xscratcher_error_code(error::no_host_name);
        return;
    }

    if (m_websock) {
        if (m_websock->is_open()) {
            *yield.ec_ = xscratcher_error_code(error::already_opened);
            return;
        }
        m_websock.reset();
    }

    auto websock = std::make_unique<websocket>(m_strand, api->Scheduler()->ssl());

    get_lowest_layer(*websock).expires_after(seconds(30));

    auto connect_result = get_lowest_layer(*websock).async_connect(api->m_resolved_websock_host, yield);
    if (*yield.ec_) {
        std::cerr << "stream connect error: ";
        return;
    }

    if (!SSL_set_tlsext_host_name(websock->next_layer().native_handle(), api->mConfig->StreamHost().c_str()))
        throw std::ios_base::failure("Failed to set SNI Hostname");

    get_lowest_layer(*websock).expires_after(seconds(30));

    websock->next_layer().async_handshake(ssl::stream_base::client, yield);
    if (*yield.ec_) {
        std::cerr << "ssl handshake error: ";
        return;
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    get_lowest_layer(*websock).expires_never();

    // Set suggested timeout settings for the websocket
    websock->set_option(websock::stream_base::timeout::suggested(boost::beast::role_type::client));

    websock->set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req)
        {
            req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
        }));

    std::string host_port = api->mConfig->StreamHost() + ":" + std::to_string(connect_result.port());

    std::clog << "WebSock handshake: " << host_port << "  " << m_path_spec << std::endl;

    websock->async_handshake(host_port, m_path_spec, yield);
    if (*yield.ec_) {
        std::cerr << "web-sock handshake error: " << yield.ec_->message() << std::endl;
        return;
    }

    m_websock = move(websock);
    m_last_heartbeat = std::chrono::system_clock::now();
    m_status = status::READY;
}

void ByBitStream::DoReadWebSocketStream(yield_context yield)
{
    boost::beast::flat_buffer buffer;

    while (true) {
        if (m_status != status::READY)
            break;

        boost::system::error_code ec;
        m_websock->async_read(buffer, yield[ec]);

        if (ec) {
            std::clog << "error" << std::endl;
            m_status = status::STALE;
            m_error_callback(ec);
            break;
        }

        if (buffer.size() != 0) {
            std::string data = boost::beast::buffers_to_string(buffer.data());
            m_data_callback(move(data));
            buffer.clear();
        }
        else {
                // std::clog << "web-sock wait..." << std::endl;
                // boost::asio::steady_timer local_timer(yield.get_executor(), milliseconds(50));
                // local_timer.async_wait(yield);
        }
    }

    if (m_status != status::STALE) {
        status s = m_status;
        throw std::runtime_error("Stream has wrong status: " + std::to_string((int)s));
    }
}

void ByBitStream::Heartbeat()
{
    spawn(m_strand, [ref = weak_from_this()](yield_context yield) {
        if (std::shared_ptr self = ref.lock()) {
            boost::system::error_code ec;
            while (self->m_status == status::INIT) {
                boost::asio::steady_timer local_timer(self->m_strand, milliseconds(50));
                local_timer.async_wait(yield);
            }
            while (self->m_status != status::STALE) {
                if (std::chrono::duration_cast<seconds>(std::chrono::system_clock::now() - self->m_last_heartbeat.load()) > seconds(20)) {
                    std::ostringstream buf;
                    buf << R"({"req_id":")" << ++(self->m_req_counter) << R"(","op":"ping"})" ;
                    std::string message = buf.str();

                    std::clog << "web-sock ping: " << message << " ... " << std::flush;
                    self->m_websock->async_write(boost::asio::buffer(message), yield[ec]);
                    if (ec) {
                        self->m_status = status::STALE;

                        std::clog << "error" << std::endl;
                        self->m_error_callback(ec);
                        return;
                    }
                    std::clog << "ok" << std::endl;
                    self->m_last_heartbeat = std::chrono::system_clock::now();
                }
                else {
                    self->m_heartbeat_timer.async_wait(yield);
                }
            }
        }
    });
}

void ByBitStream::Message(std::string message)
{
    spawn(m_strand, [message, ref = weak_from_this()](yield_context yield) {
        if (auto self = ref.lock()) {
            boost::system::error_code ec;

            while (self->m_status == status::INIT) {
                boost::asio::steady_timer local_timer(self->m_strand, milliseconds(50));
                local_timer.async_wait(yield);
            }

            std::clog << "web-sock write: " << message << " ... " << std::flush;
            self->m_websock->async_write(boost::asio::buffer(message), yield[ec]);
            if (ec) {
                self->m_status = status::STALE;

                std::clog << "error" << std::endl;
                self->m_error_callback(ec);
                return;
            }
            std::clog << "ok" << std::endl;
            self->m_last_heartbeat = std::chrono::system_clock::now();
        }
    });
}

}
