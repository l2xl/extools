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
#include <iostream>

namespace scratcher::connect {

boost::asio::awaitable<std::string> http_query::co_request(std::weak_ptr<context> ctx_ref, std::string host, std::string port, std::string path)
{
    if (auto context = ctx_ref.lock())
    {
        // Resolve host and port
        auto resolved_endpoints = co_await context::co_resolve(context, host, port);

        // Create SSL stream
        ssl_stream stream(co_await boost::asio::this_coro::executor, context->scheduler()->ssl());

        // Connect to resolved endpoint
        co_await boost::beast::get_lowest_layer(stream).async_connect(resolved_endpoints, use_awaitable);

        // Set SNI hostname
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            throw std::runtime_error("Failed to set SNI hostname");

        // SSL handshake
        co_await stream.async_handshake(ssl::stream_base::client, use_awaitable);

        // Create HTTP request
        boost::beast::http::request<boost::beast::http::string_body> req( boost::beast::http::verb::get, path, 11);
        req.set(boost::beast::http::field::host, host);
        req.prepare_payload();

        // Send request
        co_await boost::beast::http::async_write(stream, req, use_awaitable);

        // Read response
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> response;
        co_await boost::beast::http::async_read(stream, buffer, response, use_awaitable);

        // Check response status
        if (response.result() != boost::beast::http::status::ok)
            throw std::runtime_error("HTTP request failed: " + std::string(response.reason()));

        // Call handler with response body
        co_return response.body();
    }
    co_return "";
}

} // namespace scratcher::connect
