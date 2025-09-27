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

#ifndef SCRATCHER_SYNC_POLICY_HPP
#define SCRATCHER_SYNC_POLICY_HPP

#include <memory>
#include <functional>
#include <chrono>
#include <exception>

namespace scratcher::datahub::policy {

/**
 * @brief Base sync policy providing error handling functionality
 */
class sync_policy_base
{
public:
    using error_handler_type = std::function<void(std::exception_ptr)>;

    void set_error_handler(error_handler_type handler)
    {
        m_error_handler = std::move(handler);
    }

protected:
    error_handler_type m_error_handler;

    void handle_error(std::exception_ptr e)
    {
        if (m_error_handler) {
            m_error_handler(e);
        }
    }
};

/**
 * @brief Query data once on initialization
 */
template<typename Entity>
class query_on_init
{
/*    sync_policy_base m_base;

public:

    template<typename Connection>
    void start(Connection& connection)
    {
        // Trigger single query
        connection();
    }

    template<typename Connection>
    void reconnect(Connection& connection)
    {
        // Re-execute the query
        connection();
    }

    void set_error_handler(typename sync_policy_base<Entity>::error_handler_type handler)
    {
        m_base.set_error_handler(std::move(handler));
    }

protected:
    void handle_error(std::exception_ptr e)
    {
        m_base.handle_error(e);
    }
*/
};

/**
 * @brief WebSocket subscription with automatic reconnection
 */
template<typename Entity>
class websocket_subscription_sync
{
    /*
    sync_policy_base<Entity> m_base;

public:
    template<typename Connection>
    void start(Connection& connection)
    {
        connection();
    }

    template<typename Connection>
    void reconnect(Connection& connection)
    {
        if (m_running) {
            // TODO: Implement reconnection with delay
            connection();
        }
    }

    void set_error_handler(typename sync_policy_base<Entity>::error_handler_type handler)
    {
        m_base.set_error_handler(std::move(handler));
    }

protected:
    void handle_error(std::exception_ptr e)
    {
        m_base.handle_error(e);
    }
*/
};

/**
 * @brief Push local changes to remote after writes
 */
template<typename Entity>
class push_after_write_sync
{
/*
private:
    sync_policy_base<Entity> m_base;

public:
    template<typename Connection>
    void start(Connection& connection)
    {
        // Set up write monitoring
        // TODO: Monitor DAO writes and trigger pushes
    }

    void stop()
    {
        // Stop monitoring
    }

    template<typename Connection>
    void reconnect(Connection& connection)
    {
        // Re-establish push connection
        connection();
    }

    void set_error_handler(typename sync_policy_base<Entity>::error_handler_type handler)
    {
        m_base.set_error_handler(std::move(handler));
    }

protected:
    void handle_error(std::exception_ptr e)
    {
        m_base.handle_error(e);
    }
*/
};

/**
 * @brief Bidirectional sync combining push and pull
 */
template<typename Entity>
class bidirectional_sync
{
/*
private:
    sync_policy_base<Entity> m_base;
    std::unique_ptr<query_on_init_sync<Entity>> m_query_policy;
    std::unique_ptr<push_after_write_sync<Entity>> m_push_policy;

public:
    bidirectional_sync()
        : m_query_policy(std::make_unique<query_on_init_sync<Entity>>())
        , m_push_policy(std::make_unique<push_after_write_sync<Entity>>())
    {}

    template<typename Connection>
    void start(Connection& connection)
    {
        m_query_policy->start(connection);
        m_push_policy->start(connection);
    }

    void stop()
    {
        if (m_query_policy) m_query_policy->stop();
        if (m_push_policy) m_push_policy->stop();
    }

    template<typename Connection>
    void reconnect(Connection& connection)
    {
        if (m_query_policy) m_query_policy->reconnect(connection);
        if (m_push_policy) m_push_policy->reconnect(connection);
    }

    void set_error_handler(typename sync_policy_base<Entity>::error_handler_type handler)
    {
        m_base.set_error_handler(handler);
        if (m_query_policy) m_query_policy->set_error_handler(handler);
        if (m_push_policy) m_push_policy->set_error_handler(handler);
    }

protected:
    void handle_error(std::exception_ptr e)
    {
        m_base.handle_error(e);
    }
*/
};

} // namespace scratcher::datahub::policies

#endif // SCRATCHER_SYNC_POLICY_HPP
