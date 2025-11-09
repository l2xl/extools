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

#ifndef DATAHUB_DATA_SINK_HPP
#define DATAHUB_DATA_SINK_HPP

#include <memory>
#include <exception>
#include <concepts>
#include <ranges>
#include <boost/lockfree/spsc_queue.hpp>

#include "dao/data_model.hpp"
#include "data/db_storage.hpp"

namespace scratcher::datahub {

enum class DataSource {
    CACHE,   // Data loaded from database/cache
    SERVER   // Data received from network/server
};

template<typename D, typename H>
class data_adapter
{
public:
    using data_type = D;
    using handler_type = H;

private:
    handler_type m_handler;

public:
    data_adapter(handler_type&& h) : m_handler(std::forward<H>(h))
    {}

    bool operator()(const std::string& json_data) {
        //std::clog << "Try JSON: " << json_data << std::endl;
        try {
            auto result = glz::read_json<data_type>(json_data);
            if (result) {
                m_handler(move(*result));
                return true;
            }
            // else {
            //     const auto& err = result.error();
            //     std::clog << "Failed to read json data for type '" << typeid(data_type).name() << "':\n"
            //               << "  Error: " << glz::format_error(err, json_data) << std::endl;
            // }
        }
        catch (...) {
            std::cerr << "Unknown exception while parsing json for type '" << typeid(data_type).name() << "'" << std::endl;
            // Parsing failed, this acceptor cannot handle this data
        }
        return false;
    }
};

template <typename D, typename H>
data_adapter<D, H> make_data_adapter(H&& h)
{ return data_adapter<D, H>(std::forward<H>(h)); }

template<typename... Acceptor>
class data_dispatcher
{
public:
    using acceptor_tuple_type = std::tuple<Acceptor...>;
    using queue_type = boost::lockfree::spsc_queue<std::string>;
    using executor_type = boost::asio::any_io_executor;

private:
    std::shared_ptr<boost::lockfree::spsc_queue<std::string>> m_data_queue;
    std::shared_ptr<acceptor_tuple_type> m_acceptors;
    boost::asio::strand<executor_type> m_dispatch_strand;

public:
    explicit data_dispatcher(const std::shared_ptr<AsioScheduler>& scheduler, Acceptor&&... acceptors)
        : m_data_queue(std::make_shared<queue_type>(16))
        , m_acceptors(std::make_shared<acceptor_tuple_type>(std::forward<Acceptor>(acceptors)...))
        , m_dispatch_strand(boost::asio::make_strand(scheduler->io().get_executor()))
    { }

    data_dispatcher(const data_dispatcher&) = default;
    data_dispatcher(data_dispatcher&&) = default;
    data_dispatcher& operator=(const data_dispatcher&) = default;
    data_dispatcher& operator=(data_dispatcher&&) = default;

    void operator()(std::string data) const
    {
        if (!m_data_queue->push(std::move(data))) {
            // Queue is full, data is discarded
            return;
        }

        auto acceptors_ref = std::weak_ptr<acceptor_tuple_type>(m_acceptors);
        auto queue_ref = std::weak_ptr<queue_type>(m_data_queue);

        // Post for async dispatching
        boost::asio::post(m_dispatch_strand, [=]() {
            process_queue(queue_ref, acceptors_ref);
        });
    }

private:
    static void process_queue(std::weak_ptr<queue_type> queue_ref, std::weak_ptr<acceptor_tuple_type> acceptors_ref)
    {
        auto queue = queue_ref.lock();
        auto acceptors = acceptors_ref.lock();

        if (queue && acceptors) {
            std::string data;
            while (queue->pop(data)) {
                // Try every acceptor in sequence
                try_acceptors(*acceptors, data, std::index_sequence_for<Acceptor...>{});
            }
        }
    }

    template<std::size_t... Indices>
    static bool try_acceptors(acceptor_tuple_type& acceptors, const std::string& data, std::index_sequence<Indices...>)
    {
        // Try each acceptor in sequence using fold expression
        return (try_accept<Indices>(acceptors, data) || ...);
    }

    template<std::size_t Index>
    static bool try_accept(acceptor_tuple_type& acceptors, const std::string& data)
    {
        auto& acceptor = std::get<Index>(acceptors);
        return acceptor(data);
    }
};

template<typename... Acceptor>
data_dispatcher<Acceptor...> make_data_dispatcher(const std::shared_ptr<AsioScheduler>& scheduler, Acceptor&& ... acceptors)
{ return data_dispatcher<Acceptor...>(scheduler, std::forward<Acceptor>(acceptors)...); }

/**
 * @brief DataSink template providing RAII-compliant data management
 *
 * Template parameters:
 * - Entity: The data entity type with primary key
 * - PrimaryKey: Pointer to member for primary key field
 * - Connection: Adapter that converts data and provides handler interface
 */
template<typename Entity, auto PrimaryKey, typename DataCallable, typename ErrorCallable>
class data_sink : public std::enable_shared_from_this<data_sink<Entity, PrimaryKey, DataCallable, ErrorCallable>>
{
public:
    using entity_type = Entity;
    using data_handler_type = DataCallable;
    using error_handler_type = ErrorCallable;

private:
    std::shared_ptr<dao::data_model<Entity, PrimaryKey>> m_dao;
    data_handler_type m_data_handler;
    error_handler_type m_error_handler;

    struct ensure_private {};

public:
    /**
     * @brief Private constructor - use create() factory function instead
     */
    data_sink(const std::shared_ptr<SQLite::Database>& db,
             data_handler_type&& data_callback,
             error_handler_type&& error_handler,
             ensure_private)
        : m_dao(dao::data_model<Entity, PrimaryKey>::create(db)) // Create DAO implicitly
        , m_data_handler(std::forward<data_handler_type>(data_callback))
        , m_error_handler(std::forward<error_handler_type>(error_handler))
    { }

    /**
     * @brief Factory function to create DataSink with RAII guarantees
     * Automatically loads existing data from database and calls callback with CACHE source
     */
    static std::shared_ptr<data_sink> create(const std::shared_ptr<SQLite::Database>& db,
                                           data_handler_type&& data_callback, error_handler_type&& error_callback)
    {
        auto sink = std::make_shared<data_sink>(db,
            std::forward<data_handler_type>(data_callback),
            std::forward<error_handler_type>(error_callback),
            ensure_private{});

        // Automatically load existing data from database
        sink->load_cached_data();

        return sink;
    }

    template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, Entity>
    auto data_acceptor() {
        std::weak_ptr<data_sink> ref = this->shared_from_this();
        return [=](Range&& entities) {
            if (auto self = ref.lock()) {
                self->process_new_entities(std::forward<Range>(entities), DataSource::SERVER);
            }
        };
    }

private:
    template <std::ranges::input_range Range>
    void process_new_entities(Range&& entities, DataSource source = DataSource::SERVER)
    requires std::convertible_to<std::ranges::range_value_t<decltype(entities)>, Entity>
    {
        try {
            // Store in DAO using insert_or_replace
            std::deque<Entity> new_entities;
            for (const auto& entity: entities) {
                if (m_dao->insert_or_replace(entity))
                    new_entities.emplace_back(static_cast<Entity>(entity));
            }

            // Only notify callback if we have entities to report
            if (!new_entities.empty()) {
                m_data_handler(std::move(new_entities), source);
            } else {
                std::clog << "data_sink::on_data_received: No new entities to report, skipping callback" << std::endl;
            }
        }
        catch (const std::exception& e) {
            m_error_handler(std::current_exception());
        }
    }

    /**
     * @brief Load existing data from database and trigger callback with CACHE source
     */
    void load_cached_data() {
        try {
            auto cached_entities = m_dao->query();
            if (!cached_entities.empty()) {
                m_data_handler(std::move(cached_entities), DataSource::CACHE);
            }
        }
        catch (const std::exception& e) {
            m_error_handler(std::current_exception());
        }
    }
};

template<typename Entity, auto PrimaryKey, typename DataCallable, typename ErrorCallable>
std::shared_ptr<data_sink<Entity, PrimaryKey, DataCallable, ErrorCallable>> make_data_sink(
    const std::shared_ptr<SQLite::Database>& db, DataCallable&& data_callback, ErrorCallable&& error_callback)
{
    return data_sink<Entity, PrimaryKey, DataCallable, ErrorCallable>::create(
                         db, std::forward<DataCallable>(data_callback), std::forward<ErrorCallable>(error_callback));
}


} // namespace scratcher::datahub

#endif // DATAHUB_DATA_SINK_HPP
