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

#include "http_query.hpp"
#include "connection_errors.hpp"
#include <iostream>

namespace scratcher::connect {

using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;

namespace this_coro = boost::asio::this_coro;
using ssl_stream = boost::beast::ssl_stream<boost::beast::tcp_stream>;


http_query::http_query(std::shared_ptr<context> context, const std::string& url, std::function<void(std::string)> data_handler, std::function<void(std::exception_ptr)> error_handler, ensure_private)
    : m_context(context)
    , m_data_handler(std::move(data_handler))
    , m_error_handler(std::move(error_handler))
{
    try {
        auto parsed_url = boost::urls::parse_uri(url);

        std::string scheme = parsed_url.value().scheme();
        m_host = parsed_url.value().host();
        m_port = parsed_url.value().port();
        m_path = parsed_url.value().path();

        if (parsed_url.value().has_query())
            m_query = parsed_url.value().query();

        if (m_port.empty()) {
            if (scheme == "https")
                m_port = "443";
            else
                throw std::invalid_argument("Unsupported scheme: " + scheme);
        }

        co_spawn(context->scheduler()->io(), context->co_resolve(context, m_host, m_port), detached);
    }
    catch (...) {
        std::throw_with_nested(std::invalid_argument("Invalid URL: " + url));
    }
}

std::shared_ptr<http_query> http_query::create(std::shared_ptr<context> context, const std::string& url, std::function<void(std::string)> data_handler, std::function<void(std::exception_ptr)> error_handler)
{
    return std::make_shared<http_query>(std::move(context), url, std::move(data_handler), std::move(error_handler), ensure_private{});
}

void http_query::operator()(std::string query_params)
{
    std::string path_query = m_path;

    if (!m_query.empty() || !query_params.empty()) {
        path_query += "?";

        if (!m_query.empty()) {
            path_query += m_query;
        }

        if (!query_params.empty()) {
            if (!m_query.empty()) {
                if (query_params[0] != '&') {
                    path_query += "&";
                }
            }

            if (query_params[0] == '&' && m_query.empty()) {
                query_params = query_params.substr(1);
            }

            path_query += std::move(query_params);
        }
    }

    std::weak_ptr<http_query> ref = weak_from_this();
    if (auto ctx = m_context.lock())
    {
        co_spawn(ctx->scheduler()->io(), co_request(ref, path_query), [ref](std::exception_ptr e, std::string result) {
            if (auto self = ref.lock()) {
                if (e) {
                    if (self->m_error_handler) {
                        self->m_error_handler(e);
                    }
                } else {
                    if (self->m_data_handler) {
                        self->m_data_handler(std::move(result));
                    }
                }
            }
        });
    }
}

boost::asio::awaitable<std::string> http_query::co_request(std::weak_ptr<http_query> ref, std::string path_query)
{
    std::shared_ptr<context> context;
    std::string host, port, full_url;

    if (auto self = ref.lock()) {
        context = self->m_context.lock();
        host = self->m_host;
        port = self->m_port;
        full_url = "https://" + host + ":" + port + path_query;
    }
    else {
        co_return "";
    }

    if (context)
    {
        try {
            // Resolve host and port
            auto resolved_endpoints = co_await context::co_resolve(context, host, port);

            // Create SSL stream with timeout
            ssl_stream stream(co_await this_coro::executor, context->scheduler()->ssl());

            // Set timeout on the lowest layer (TCP stream)
            boost::beast::get_lowest_layer(stream).expires_after(context->timeout());

            // Connect to resolved endpoint
            co_await boost::beast::get_lowest_layer(stream).async_connect(resolved_endpoints, use_awaitable);

            // Set SNI hostname
            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
                throw std::runtime_error("Failed to set SNI hostname");

            // Set timeout for SSL handshake
            boost::beast::get_lowest_layer(stream).expires_after(context->timeout());

            // SSL handshake
            co_await stream.async_handshake(ssl::stream_base::client, use_awaitable);

            // Create HTTP request
            boost::beast::http::request<boost::beast::http::string_body> req(boost::beast::http::verb::get, path_query, 11);
            req.set(boost::beast::http::field::host, host);
            req.prepare_payload();

            // Set timeout for HTTP operations
            boost::beast::get_lowest_layer(stream).expires_after(context->timeout());

            // Send request
            co_await boost::beast::http::async_write(stream, req, use_awaitable);

            // Read response
            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::string_body> response;
            co_await boost::beast::http::async_read(stream, buffer, response, use_awaitable);

            // Turn off the timeout on the tcp_stream, because we're done with the request
            boost::beast::get_lowest_layer(stream).expires_never();

            // Check response status
            if (response.result() != boost::beast::http::status::ok) {
                throw_http_error(response.result(), full_url);
            }

            // Return response body
            co_return response.body();
        }
        catch (const boost::system::system_error& e) {
            // Check if this is a domain resolution error
            const auto& error_code = e.code();
            if (error_code.category() == boost::asio::error::get_netdb_category() ||
                error_code.category() == boost::asio::error::get_addrinfo_category()) {
                throw_domain_error(error_code, host);
            }
            throw; // Re-throw if not a domain error
        }
    }
    co_return "";
}

} // namespace scratcher::connect
