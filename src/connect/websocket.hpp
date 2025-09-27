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

#ifndef SCRATCHER_WEBSOCK_CONNECTION
#define SCRATCHER_WEBSOCK_CONNECTION

#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url.hpp>


#include "connection_context.hpp"

namespace scratcher::connect {

/**
 * @brief WebSocketConnection implements persistent WebSocket subscriptions
 *
 * This connection type is designed for persistent subscriptions where:
 * - Single connection instance is shared between multiple DataSinks
 * - Connection opens after first create() call
 * - Each operator() call sends a subscription message
 * - Connection remains open and distributes data to the handler
 *
 * Handler callback types:
 * - std::function<void(std::string)> - receives JSON messages
 * - std::function<void(std::exception_ptr)> - receives errors
 */
class websock_connection : public std::enable_shared_from_this<websock_connection>
{
    using websocket_stream = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;

    enum class status {INIT, READY, STALE};

    std::atomic<status> m_status = status::INIT;
    std::weak_ptr<context> m_context;
    std::function<void(std::string)> m_data_handler;
    std::function<void(std::exception_ptr)> m_error_handler;
    boost::asio::strand<websocket_stream::executor_type> m_strand;

    std::function<void(std::exception_ptr)> m_common_handler;

    std::string m_host;
    std::string m_port;
    std::string m_path_query;

    std::shared_ptr<websocket_stream> m_websocket;

    // Heartbeat management
    std::function<std::string(size_t)> m_make_heartbeat_mesage = [](size_t){ return std::string{}; };
    std::shared_ptr<boost::asio::steady_timer> m_heartbeat_timer;
    std::chrono::steady_clock::time_point m_last_heartbeat;
    std::atomic<size_t> m_request_counter = 0;
    
    // Async operations
    static boost::asio::awaitable<void> co_heartbeat_loop(std::weak_ptr<websock_connection>);
    static boost::asio::awaitable<void> co_exec_loop(std::weak_ptr<websock_connection>);
    static boost::asio::awaitable<void> co_open(std::shared_ptr<websock_connection>);
    static boost::asio::awaitable<std::string> co_read(std::weak_ptr<websock_connection>);
    static boost::asio::awaitable<void> co_message(std::shared_ptr<websock_connection>, std::string message);
    
    struct ensure_private {};
public:
    explicit websock_connection(std::shared_ptr<context> context, const std::string& url, std::function<void(std::string)> data_handler, std::function<void(std::exception_ptr)> error_handler, ensure_private);

    /**
     * @brief Create WebSocketConnection instance
     * @param context Shared connection context with host resolution
     * @param url Full URL to request (e.g., "wss://api.bybit.com/v5/public/spot")
     * @param data_handler Handler that receives JSON messages
     * @param error_handler Handler that receives errors
     */
    static std::shared_ptr<websock_connection> create(std::shared_ptr<context> context, const std::string& url, std::function<void(std::string)> data_handler, std::function<void(std::exception_ptr)> error_handler);

    /**
     * @brief Create WebSocketConnection instance with generic callable handlers
     * @tparam DataAcceptor Callable accepting std::string (e.g., data_dispatcher, data_adapter, lambda)
     * @tparam ErrorHandler Callable accepting std::exception_ptr
     * @param context Shared connection context with host resolution
     * @param url Full URL to request (e.g., "wss://api.bybit.com/v5/public/spot")
     * @param data_handler Handler that receives JSON messages (data_dispatcher, data_adapter, etc.)
     * @param error_handler Handler that receives errors
     */
    template<typename DataAcceptor, typename ErrorHandler>
    static std::shared_ptr<websock_connection> create(std::shared_ptr<context> context, const std::string& url, DataAcceptor&& data_handler, ErrorHandler&& error_handler)
    {
        // Wrap callables in std::function for type erasure
        std::function<void(std::string)> wrapped_data_handler =
            [handler = std::forward<DataAcceptor>(data_handler)](std::string data) mutable {
                handler(std::move(data));
            };

        std::function<void(std::exception_ptr)> wrapped_error_handler =
            [handler = std::forward<ErrorHandler>(error_handler)](std::exception_ptr e) mutable {
                handler(e);
            };

        return create(std::move(context), url, std::move(wrapped_data_handler), std::move(wrapped_error_handler));
    }

    void set_heartbeat(std::chrono::seconds seconds, std::function<std::string(size_t number)>);
    /**
     * @brief Send subscription message
     * @param message Subscription message to send
     */
    void operator()(std::string message);
};

} // namespace scratcher::connect

#endif // SCRATCHER_WEBSOCK_CONNECTION
