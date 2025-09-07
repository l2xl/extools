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

#include "websocket.hpp"
#include <iostream>
#include <chrono>

namespace scratcher::connect {

using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;

namespace this_coro =  boost::asio::this_coro;


websock_connection::websock_connection(std::shared_ptr<context> context, const std::string& url, std::function<void(std::exception_ptr e, std::string message)> handler, EnsurePrivate)
    : m_context(std::move(context))
    , m_handler(std::move(handler))
    , m_strand(make_strand(context->scheduler()->io()))
    , m_heartbeat_timer(std::make_shared<boost::asio::steady_timer>(m_strand, std::chrono::steady_clock::time_point::max()))
    , m_last_heartbeat(std::chrono::steady_clock::time_point::min())
{
    try {
        auto parsed_url = boost::urls::parse_uri(url);

        std::string scheme = parsed_url.value().scheme();
        m_host = parsed_url.value().host();
        m_port = parsed_url.value().port();
        m_path_query = parsed_url.value().path();

        if (parsed_url.value().has_query())
            m_path_query += ("?" + parsed_url.value().query());

        if (m_port.empty()) {
            if (scheme == "wss")
                m_port = "443";
            else
                throw std::invalid_argument("Unsupported scheme: " + scheme);
        }
    }
    catch (...) {
        std::throw_with_nested(std::invalid_argument("Invalid URL: " + url));
    }
}

std::shared_ptr<websock_connection> websock_connection::create(std::shared_ptr<context> context, const std::string& url, std::function<void(std::exception_ptr e, std::string message)> handler)
{
    auto ws = std::make_shared<websock_connection>(std::move(context), url, std::move(handler), EnsurePrivate{});

    std::weak_ptr<websock_connection> ref = ws;
    ws->m_common_handler = [ref](std::exception_ptr e) {
        if (e) {
            if (auto self = ref.lock())
                self->m_handler(e, {});
        }
    };

    co_spawn(ws->m_strand, co_exec_loop(ws), ws->m_common_handler);
    co_spawn(ws->m_strand, co_heartbeat_loop(ws), ws->m_common_handler);

    return ws;
}

void websock_connection::set_heartbeat(std::chrono::seconds seconds, std::function<std::string(size_t number)> heartbeat_generator)
{
    m_make_heartbeat_mesage = std::move(heartbeat_generator);
    m_heartbeat_timer->expires_after(seconds);
}

void websock_connection::operator()(std::string message)
{
    std::weak_ptr<websock_connection> ref = weak_from_this();
    co_spawn(m_strand, co_message(shared_from_this(), std::move(message)), m_common_handler);
}

boost::asio::awaitable<void> websock_connection::co_heartbeat_loop(std::weak_ptr<websock_connection> ref)
{
    std::shared_ptr<boost::asio::steady_timer> heartbeat_timer;
    status cur_status = status::INIT;
    if (auto self = ref.lock()) {
        cur_status = self->m_status;
        heartbeat_timer = self->m_heartbeat_timer;
    }
    else co_return;

    while (cur_status != status::STALE) {
        try {
            if (auto self = ref.lock()) {
                // Check if we need to send heartbeat
                auto now = std::chrono::steady_clock::now();

                if (now > heartbeat_timer->expiry()) {
                    // Generate heartbeat message
                    std::string heartbeat_message = self->m_make_heartbeat_mesage(++self->m_request_counter);

                    if (heartbeat_message.empty()) {
                        std::cerr << "Empty heartbeat message!" << std::endl;
                        throw std::invalid_argument("WebSocket heartbeat failed due to empty heartbeat message");
                    }

                    co_await self->m_websocket->async_write(boost::asio::buffer(heartbeat_message), use_awaitable);

                    self->m_last_heartbeat = now;
                }

                self.reset();
            }
            else break;
        }
        catch (boost::system::error_code& ec) {
            if (auto self = ref.lock()) {
                self->m_status = status::STALE;
                std::cerr << "Heartbeat error: " << ec.message() << std::endl;
                heartbeat_timer->expires_after(std::chrono::steady_clock::duration::max());
            }
            else break;
        }
        catch (std::exception& e) {
            if (auto self = ref.lock()) {
                self->m_status = status::STALE;
                std::cerr << "Heartbeat unknown error: " << e.what() << std::endl;
                heartbeat_timer->expires_after(std::chrono::steady_clock::duration::max());
            }
            else break;
        }
        catch (...) {
            if (auto self = ref.lock()) {
                self->m_status = status::STALE;
                std::cerr << "Heartbeat unknown error" << std::endl;
                heartbeat_timer->expires_after(std::chrono::steady_clock::duration::max());
            }
            else break;
        }

        co_await heartbeat_timer->async_wait(use_awaitable);
    }
}

boost::asio::awaitable<void> websock_connection::co_exec_loop(std::weak_ptr<websock_connection> ref)
{
    namespace this_coro = boost::asio::this_coro;
    
    try {
        for (;;) {
            try {
                if (auto self = ref.lock()) {
                    co_await co_open(self);
                    break;
                }
                co_return;
            }
            catch (boost::system::error_code& ec) {
                std::cerr << "Connection error: " << ec.message() << std::endl;
            }
            catch (std::exception& e) {
                std::cerr << "Connection error: " << e.what() << std::endl;
            }
            co_await boost::asio::steady_timer(co_await this_coro::executor, std::chrono::milliseconds(250)).async_wait(use_awaitable);
        }

        if (auto self = ref.lock()) {
            for ( ;self->m_handler; ) {
                self->m_handler(nullptr, co_await co_read(self));
            }
        }
        co_return;
    }
    catch (boost::system::error_code& ec) {
        if (auto self = ref.lock()) {
            self->m_status = status::STALE;
            std::cerr << "WebSocket error: " << ec.message() << std::endl;
            std::rethrow_exception(std::current_exception());
        }
    }
    catch (std::exception& e) {
        if (auto self = ref.lock()) {
            self->m_status = status::STALE;
            std::cerr << "WebSocket unknown error: " << e.what() << std::endl;
            std::rethrow_exception(std::current_exception());
        }
    }
    catch (...) {
        if (auto self = ref.lock()) {
            self->m_status = status::STALE;
            std::cerr << "WebSocket unknown error" << std::endl;
            throw std::runtime_error("WebSocket unknown error");
        }
    }
}

boost::asio::awaitable<void> websock_connection::co_open(std::shared_ptr<websock_connection> self)
{
    if (auto context = self->m_context.lock())
    {
        // Resolve host
        auto resolved_endpoints = co_await context::co_resolve(context, self->m_host, self->m_port);

        if (self->m_websocket) {
            if (self->m_websocket->is_open()) {
                throw std::runtime_error("WebSocket already open");
            }
            self->m_websocket.reset();
        }

        auto websock = std::make_unique<websocket_stream>(co_await this_coro::executor, context->scheduler()->ssl());

        get_lowest_layer(*websock).expires_after(std::chrono::seconds(30));

        auto connect_result = co_await get_lowest_layer(*websock).async_connect(resolved_endpoints, use_awaitable);

        // Set SNI hostname for SSL
        if (!SSL_set_tlsext_host_name(websock->next_layer().native_handle(), self->m_host.c_str())) {
            throw std::runtime_error("Failed to set SNI Hostname");
        }

        // Set timeout for SSL handshake
        get_lowest_layer(*websock).expires_after(std::chrono::seconds(30));

        // Perform SSL handshake
        co_await websock->next_layer().async_handshake(ssl::stream_base::client, use_awaitable);

        // Turn off the timeout on the tcp_stream, because the websocket stream has its own timeout system
        get_lowest_layer(*websock).expires_never();

        // Set suggested timeout settings for the websocket
        websock->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

        // Set user agent
        websock->set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type& req)
            {
                req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
            }));

        std::string host_port = self->m_host + ":" + std::to_string(connect_result.port());

        std::clog << "WebSocket handshake: " << host_port << " " << self->m_path_query << std::endl;

        // Perform WebSocket handshake
        co_await websock->async_handshake(host_port, self->m_path_query, use_awaitable);

        self->m_websocket = std::move(websock);
        self->m_last_heartbeat = std::chrono::steady_clock::now();
        self->m_status = status::READY;

        std::clog << "WebSocket connection established" << std::endl;
    }
}

boost::asio::awaitable<std::string> websock_connection::co_read(std::shared_ptr<websock_connection> self)
{
    boost::beast::flat_buffer buffer;

    for (;;) {
        if (self->m_status != status::READY)
            break;

        co_await self->m_websocket->async_read(buffer, use_awaitable);

        if (buffer.size() != 0) {
            std::string data = boost::beast::buffers_to_string(buffer.data());
            buffer.clear();

            co_return data;
        }
    }

    if (self->m_status != status::STALE) {
        status s = self->m_status;
        throw std::runtime_error("Stream has wrong status: " + std::to_string((int)s));
    }

    co_return std::string{};
}

boost::asio::awaitable<void> websock_connection::co_message(std::shared_ptr<websock_connection> self, std::string message)
{
    namespace this_coro = boost::asio::this_coro;
    
    try {
        if (self->m_status == status::INIT) {
            boost::asio::steady_timer local_timer(co_await this_coro::executor, std::chrono::milliseconds(50));
            while (self->m_status == status::INIT) {
                co_await local_timer.async_wait(use_awaitable);
            }
        }

        if (self->m_status != status::READY) {
            co_return;
        }

        std::clog << "WebSocket write: " << message << " ... " << std::flush;

        co_await self->m_websocket->async_write(boost::asio::buffer(message), use_awaitable);

        std::clog << "ok" << std::endl;
        self->m_last_heartbeat = std::chrono::steady_clock::now();
    }
    catch (boost::system::error_code& ec) {
        self->m_status = status::STALE;
        std::cerr << "WebSocket write error: " << ec.message() << std::endl;
        std::rethrow_exception(std::current_exception());

    }
    catch (std::exception& e) {
        self->m_status = status::STALE;
        std::cerr << "WebSocket write unknown error: " << e.what() << std::endl;
        std::rethrow_exception(std::current_exception());
    }
    catch (...) {
        self->m_status = status::STALE;
        std::cerr << "WebSocket write unknown error" << std::endl;
        throw std::runtime_error("WebSocket write unknown error");
    }
}

} // namespace scratcher::connect
