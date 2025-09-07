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

#ifndef SCRATCHER_CONNECT_CONTEXT_HPP
#define SCRATCHER_CONNECT_CONTEXT_HPP

#include <memory>
#include <string>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>

#include "scheduler.hpp"

namespace scratcher::connect {

/**
 * @brief ConnectionContext manages shared connection infrastructure
 * 
 * This class provides common functionality needed by both JSON-RPC and WebSocket connections:
 * - AsioScheduler reference for async operations
 * - Host resolution and caching with strand-based thread safety
 * - Connection parameters management
 * 
 * The context is shared between multiple connection instances to avoid duplicate
 * DNS resolution and provide consistent connection parameters.
 */
class context : public std::enable_shared_from_this<context>
{
public:
    struct EnsurePrivate {};
    
    // Key for resolution cache (host:port)
    using HostPortKey = std::string;

private:
    std::shared_ptr<AsioScheduler> m_scheduler;
    
    // Resolution cache with strand for thread safety
    boost::asio::strand<io_context::executor_type> m_resolution_strand;
    boost::container::flat_map<HostPortKey, boost::asio::ip::tcp::resolver::results_type> m_resolution_cache;
    
    // Connection parameters
    std::chrono::milliseconds m_timeout;

public:
    /**
     * @brief Construct ConnectionContext with required parameters
     * @param scheduler AsioScheduler for async operations
     * @param timeout Request timeout duration
     */
    context(std::shared_ptr<AsioScheduler> scheduler, std::chrono::milliseconds timeout, EnsurePrivate)
        : m_scheduler(std::move(scheduler))
        , m_resolution_strand(boost::asio::make_strand(m_scheduler->io()))
        , m_timeout(timeout)
    {}

    /**
     * @brief Create ConnectionContext
     */
    static std::shared_ptr<context> create(
        std::shared_ptr<AsioScheduler> scheduler,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(10000));
    
    // Accessors
    std::shared_ptr<AsioScheduler> scheduler() const { return m_scheduler; }
    std::chrono::milliseconds timeout() const { return m_timeout; }
    
    /**
     * @brief Resolve host and port, using cache if available
     * @param host Remote host name or IP address
     * @param port Remote port number
     * @return awaitable that completes with resolved endpoints
     */
    static boost::asio::awaitable<boost::asio::ip::tcp::resolver::results_type>
    co_resolve(std::shared_ptr<context> self, std::string host, std::string port);
};

} // namespace scratcher::connect

#endif // SCRATCHER_CONNECT_CONTEXT_HPP
