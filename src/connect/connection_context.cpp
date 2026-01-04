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

#include "connection_context.hpp"
#include <iostream>

namespace scratcher::connect {

using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;

std::shared_ptr<context> context::create(io_context& io_ctx, std::chrono::milliseconds timeout)
{
    return std::make_shared<context>(io_ctx, timeout, ensure_private{});
}

boost::asio::awaitable<boost::asio::ip::tcp::resolver::results_type>
context::co_resolve(std::shared_ptr<context> self, std::string host, std::string port)
{
    co_await boost::asio::post(self->m_resolution_strand, use_awaitable);

    // All operations run on the strand, so no synchronization needed
    const auto key = host + ":" + port;

    // Check cache first
    auto it = self->m_resolution_cache.find(key);
    if (it != self->m_resolution_cache.end())
    {
        std::clog << "Using cached resolution for " << key << std::endl;
        co_return it->second;
    }

    // Perform resolution
    std::clog << "Starting name resolution for " << key << std::endl;
    boost::asio::ip::tcp::resolver resolver(self->io());
    auto results = co_await resolver.async_resolve(host, port, use_awaitable);

    self->m_resolution_cache.emplace(move(key), results);
    std::clog << "name resolution completed for " << key << std::endl;

    co_return results;
}

} // namespace scratcher::connect
