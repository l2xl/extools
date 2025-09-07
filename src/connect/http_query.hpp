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

namespace scratcher::connect {

/**
 * @brief JsonRpcConnection implements single-time HTTP JSON-RPC requests
 * 
 * This connection type is designed for one-time requests where:
 * - Each connection instance handles a single request
 * - Data provider creates connection per request
 * - DataSink calls operator() to execute the request and receive response
 * 
 * Handler callback type:
 * - std::function<void(std::exception_ptr, std::string)> - co_spawn completion handler
 */
class http_query
{
    std::weak_ptr<context> m_context;
    std::function<void(std::exception_ptr, std::string)> m_handler;
    std::string m_host;
    std::string m_port;
    std::string m_path_query;

    // SSL stream type for HTTPS requests
    using ssl_stream = boost::beast::ssl_stream<boost::beast::tcp_stream>;
    
public:
    /**
     * @brief Construct JsonRpcConnection with connection context and handler
     * @param context Shared connection context with host resolution
     * @param url Full URL to request (e.g., "https://api.bybit.com/v5/market/time")
     * @param handler Data handler that receives exception_ptr and JSON response string
     */
    explicit http_query(std::shared_ptr<context> context, const std::string& url, std::function<void(std::exception_ptr, std::string)> handler)
        : m_context(std::move(context)), m_handler(std::move(handler))
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
                if (scheme == "https")
                    m_port = "443";
                else
                    throw std::invalid_argument("Unsupported scheme: " + scheme);
            }
        }
        catch (...) {
            std::throw_with_nested(std::invalid_argument("Invalid URL: " + url));
        }
    }
    
    /**
     * @brief Execute the HTTP request
     */
    void operator()()
    {
        if (auto ctx = m_context.lock())
        {
            boost::asio::co_spawn(ctx->scheduler()->io(), co_request(m_context, m_host, m_port, m_path_query), m_handler);
        }
    }
    
private:
    static boost::asio::awaitable<std::string> co_request(std::weak_ptr<context> ctx_ref, std::string host, std::string port, std::string path);
};


} // namespace scratcher::connect

#endif // SCRATCHER_HTTP_QUERY
