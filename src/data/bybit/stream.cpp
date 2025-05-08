// Scratcher project
// Copyright (c) 2024 l2xl (l2xl/at/proton.me)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or https://opensource.org/license/mit
//

#include "bybit/stream.hpp"
#include <iostream>

#include "bybit.hpp"

namespace scratcher::bybit {

namespace this_coro =  boost::asio::this_coro;

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


ByBitStream::ByBitStream(std::shared_ptr<ByBitApi> api, std::string spec, std::function<void(std::string&&)> callback, std::function<void(boost::system::error_code)> error_callback, EnsurePrivate)
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

std::shared_ptr<ByBitStream> ByBitStream::Create(std::shared_ptr<ByBitApi> api, std::string path_spec,
    std::function<void(std::string &&)> callback, std::function<void(boost::system::error_code)> error_callback)
{
    auto stream = std::make_shared<ByBitStream>(api, move(path_spec), callback, error_callback, EnsurePrivate());

    co_spawn(stream->m_strand, coExecute(stream), detached);
    co_spawn(stream->m_strand, coHeartbeat(stream), detached);

    return stream;
}

awaitable<void> ByBitStream::coExecute(std::weak_ptr<ByBitStream> ref)
{
    try {
        for (;;) {
            try {
                if (auto self = ref.lock()) {
                    co_await coOpenWebSocketStream(self);
                    break;
                }
                co_return;
            }
            catch (boost::system::error_code& ec) {
                std::cerr << ec.message() << std::endl;
            }
            co_await boost::asio::steady_timer(co_await this_coro::executor, milliseconds(250)).async_wait(use_awaitable);
        }

        if (auto self = ref.lock()) {
            for (;;)
                self->m_data_callback(co_await coReadWebSocketStream(self));
        }
        co_return;
    }
    catch (boost::system::error_code& ec) {
        if (auto self = ref.lock()) {
            self->m_status = status::STALE;
            std::cerr << "error" << std::endl;
            self->m_error_callback(ec);
        }
    }
    catch (std::exception& e) {
        if (auto self = ref.lock()) {
            self->m_status = status::STALE;
            std::cerr << "unknown error: " << e.what() << std::endl;
        }
    }

}

awaitable<void> ByBitStream::coOpenWebSocketStream(std::shared_ptr<ByBitStream> self)
{
    auto api = self->m_api.lock();
    if (!api) throw std::runtime_error("destroyed");

    while (!api->m_is_websock_resolved) {
        std::clog << "Waiting for resolve..." << std::endl;
        co_await boost::asio::steady_timer(co_await this_coro::executor, milliseconds(250)).async_wait(use_awaitable);
    }

    if (api->m_resolved_websock_host.empty()) throw xscratcher_error_code(error::no_host_name);

    if (self->m_websock) {
        if (self->m_websock->is_open()) throw xscratcher_error_code(error::already_opened);
        self->m_websock.reset();
    }

    auto websock = std::make_unique<websocket>(co_await this_coro::executor, api->Scheduler()->ssl());

    get_lowest_layer(*websock).expires_after(seconds(30));

    auto connect_result = co_await get_lowest_layer(*websock).async_connect(api->m_resolved_websock_host, use_awaitable);

    if (!SSL_set_tlsext_host_name(websock->next_layer().native_handle(), api->mConfig->StreamHost().c_str()))
        throw std::ios_base::failure("Failed to set SNI Hostname");

    get_lowest_layer(*websock).expires_after(seconds(30));

    co_await websock->next_layer().async_handshake(ssl::stream_base::client, use_awaitable);

    // Turn off the timeout on the tcp_stream, because the websocket stream has its own timeout system.
    get_lowest_layer(*websock).expires_never();

    // Set suggested timeout settings for the websocket
    websock->set_option(ws::stream_base::timeout::suggested(boost::beast::role_type::client));

    websock->set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req)
        {
            req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
        }));

    std::string host_port = api->mConfig->StreamHost() + ":" + std::to_string(connect_result.port());

    std::clog << "WebSock handshake: " << host_port << "  " << self->m_path_spec << std::endl;

    co_await websock->async_handshake(host_port, self->m_path_spec, use_awaitable);

    self->m_websock = move(websock);
    self->m_last_heartbeat = std::chrono::system_clock::now();
    self->m_status = status::READY;
}

awaitable<std::string> ByBitStream::coReadWebSocketStream(std::shared_ptr<ByBitStream> self)
{
    boost::beast::flat_buffer buffer;

    while (true) {
        if (self->m_status != status::READY)
            break;

        co_await self->m_websock->async_read(buffer, use_awaitable);

        if (buffer.size() != 0) {
            std::string data = boost::beast::buffers_to_string(buffer.data());
            buffer.clear();
            co_return data;;
        }
    }

    if (self->m_status != status::STALE) {
        status s = self->m_status;
        throw std::runtime_error("Stream has wrong status: " + std::to_string((int)s));
    }
}


awaitable<void> ByBitStream::coHeartbeat(std::weak_ptr<ByBitStream> ref)
{
    try {
        status cur_status = status::INIT;
        if (std::shared_ptr self = ref.lock()) {
            while (self->m_status == status::INIT) {
                co_await boost::asio::steady_timer(co_await this_coro::executor, milliseconds(50)).async_wait(use_awaitable);
            }
            cur_status = self->m_status;
        }
        else co_return;

        while (cur_status != status::STALE) {
            if (std::shared_ptr self = ref.lock()) {

                if (std::chrono::duration_cast<seconds>(std::chrono::system_clock::now() - self->m_last_heartbeat.load()) > seconds(20)) {
                    std::ostringstream buf;
                    buf << R"({"req_id":")" << ++(self->m_req_counter) << R"(","op":"ping"})" ;
                    std::string message = buf.str();

                    std::clog << "web-sock ping: " << message << " ... " << std::flush;
                    co_await self->m_websock->async_write(boost::asio::buffer(message), use_awaitable);
                    std::clog << "ok" << std::endl;

                    self->m_last_heartbeat = std::chrono::system_clock::now();
                }
                else {
                    co_await self->m_heartbeat_timer.async_wait(use_awaitable);
                }
            }
            else co_return;;
        }
    }
    catch (boost::system::error_code& ec) {
        if (std::shared_ptr self = ref.lock()) {
            self->m_status = status::STALE;

            std::cerr << "error" << std::endl;
            self->m_error_callback(ec);
        }
    }
    catch (std::exception& e) {
        if (std::shared_ptr self = ref.lock()) {
            self->m_status = status::STALE;
            std::cerr << "unknown error: " << e.what() << std::endl;
        }
    }

}

awaitable<void> ByBitStream::coMessage(std::shared_ptr<ByBitStream> self, std::string message)
{
    try {
        if (self->m_status == status::INIT) {
            boost::asio::steady_timer local_timer(co_await this_coro::executor, milliseconds(50));
            while (self->m_status == status::INIT) {
                co_await local_timer.async_wait(use_awaitable);
            }
        }

        std::clog << "web-sock write: " << message << " ... " << std::flush;

        co_await self->m_websock->async_write(boost::asio::buffer(message), use_awaitable);

        std::clog << "ok" << std::endl;
        self->m_last_heartbeat = std::chrono::system_clock::now();
    }
    catch (boost::system::error_code &ec) {
        self->m_status = status::STALE;

        std::cerr << "error" << std::endl;
        self->m_error_callback(ec);
    }
    catch (std::exception &e) {
        self->m_status = status::STALE;
        std::cerr << "unknown error: " << e.what() << std::endl;
    }
}

}
