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

#ifndef SCRATCHER_HTTP_QUERY
#define SCRATCHER_HTTP_QUERY

#include <memory>
#include <string>
#include <functional>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url.hpp>

#include "connection_context.hpp"
#include "generic_handler.hpp"

namespace scratcher::connect {

/**
 * @brief http_query implements single-time HTTP requests
 *
 * This connection type is designed for one-time requests where:
 * - Each connection instance handles a single request
 * - Data provider creates connection per request
 * - DataSink calls operator() to execute the request and receive response
 * - generic generic_handler class helper is used to forward data/error to subscriber through handler_data/handler_error virtual methods
 */
class http_query : public std::enable_shared_from_this<http_query>
{
    std::weak_ptr<context> m_context;
    std::string m_host;
    std::string m_port;
    std::string m_path;
    std::string m_query;

public:
    explicit http_query(std::shared_ptr<context> context, const std::string& url);
    virtual ~http_query() = default;

    /**
     * @brief Create http_query instance with generic callable handlers
     * @tparam DataAcceptor Callable accepting std::string (e.g., data_dispatcher, data_adapter, lambda)
     * @tparam ErrorHandler Callable accepting std::exception_ptr
     * @param context Shared connection context with host resolution
     * @param url Full URL to request (e.g., "https://api.bybit.com/v5/market/time")
     * @param data_handler Handler that receives JSON response (data_dispatcher, data_adapter, etc.)
     * @param error_handler Handler that receives errors
     */
    template<typename DataAcceptor, typename ErrorHandler>
    static std::shared_ptr<http_query> create(std::shared_ptr<context> ctx, const std::string& url, DataAcceptor&& data_handler, ErrorHandler&& error_handler)
    {
        return std::static_pointer_cast<http_query>(std::make_shared<generic_handler<std::string&&, http_query, DataAcceptor, ErrorHandler, std::shared_ptr<context>, const std::string&>>(std::forward<DataAcceptor>(data_handler), std::forward<ErrorHandler>(error_handler), std::move(ctx), url));
    }

    /**
     * @brief Execute the HTTP request with additional query parameters
     * @param query_params Query string to append to the initial path/query (e.g., "&symbol=BTCUSDT")
     */
    void operator()(std::string query_params = {});

    virtual void handle_data(std::string&& data) = 0;
    virtual void handle_error(std::exception_ptr eptr) = 0;

private:
    static boost::asio::awaitable<std::string> co_request(std::weak_ptr<http_query> ref, std::string path_query);
};


} // namespace scratcher::connect

#endif // SCRATCHER_HTTP_QUERY
