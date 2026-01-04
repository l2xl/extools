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

#include "data_model.hpp"

namespace datahub {

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
                m_handler(std::move(*result));
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
    explicit data_dispatcher(boost::asio::any_io_executor executor, Acceptor&&... acceptors)
        : m_data_queue(std::make_shared<queue_type>(16))
        , m_acceptors(std::make_shared<acceptor_tuple_type>(std::forward<Acceptor>(acceptors)...))
        , m_dispatch_strand(boost::asio::make_strand(std::move(executor)))
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
data_dispatcher<Acceptor...> make_data_dispatcher(boost::asio::any_io_executor executor, Acceptor&& ... acceptors)
{ return data_dispatcher<Acceptor...>(std::move(executor), std::forward<Acceptor>(acceptors)...); }

/**
 * @brief DataSink template providing data flow management with optional filtering
 *
 * Template parameters:
 * - Entity: The data entity type
 * - DataCallable: Callback for processed data
 * - ErrorCallable: Callback for errors
 * - DataFilter: Optional filter callable (e.g., from data_model::data_acceptor())
 */
template<typename Entity, typename DataCallable, typename ErrorCallable, typename DataFilter = std::nullptr_t>
class data_sink : public std::enable_shared_from_this<data_sink<Entity, DataCallable, ErrorCallable, DataFilter>>
{
public:
    using entity_type = Entity;
    using data_handler_type = DataCallable;
    using error_handler_type = ErrorCallable;
    using data_filter_type = DataFilter;

private:
    data_handler_type m_data_handler;
    error_handler_type m_error_handler;
    [[no_unique_address]] data_filter_type m_data_filter;

    struct ensure_private {};

public:
    data_sink(data_handler_type&& data_callback,
              error_handler_type&& error_handler,
              data_filter_type filter,
              ensure_private)
        : m_data_handler(std::forward<data_handler_type>(data_callback))
        , m_error_handler(std::forward<error_handler_type>(error_handler))
        , m_data_filter(std::move(filter))
    { }

    /**
     * @brief Factory function to create DataSink
     * @param data_callback Handler for processed entity data
     * @param error_callback Handler for errors
     * @param filter Optional filter (e.g., data_model::data_acceptor() for persistence)
     */
    static std::shared_ptr<data_sink> create(
        data_handler_type&& data_callback,
        error_handler_type&& error_callback,
        data_filter_type filter = {})
    {
        auto sink = std::make_shared<data_sink>(
            std::forward<data_handler_type>(data_callback),
            std::forward<error_handler_type>(error_callback),
            std::move(filter),
            ensure_private{});

        return sink;
    }

    template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, entity_type>
    auto data_acceptor() {
        std::weak_ptr<data_sink> ref = this->shared_from_this();
        return [=](Range&& entities) {
            if (auto self = ref.lock()) {
                self->template process_entities<Range>(std::forward<Range>(entities));
            }
        };
    }

private:
    template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, entity_type>
    void process_entities(Range&& entities)
    {
        try {
            std::deque<entity_type> new_entities;

            if constexpr (std::is_same_v<data_filter_type, std::nullptr_t>) {
                // No filter - pass all entities through
                for (const auto& entity : entities) {
                    new_entities.emplace_back(static_cast<entity_type>(entity));
                }
            } else {
                // Convert input range to deque and apply filter
                std::deque<entity_type> input;
                for (const auto& entity : entities) {
                    input.emplace_back(static_cast<entity_type>(entity));
                }
                new_entities = m_data_filter(std::move(input));
            }

            if (!new_entities.empty()) {
                m_data_handler(std::move(new_entities));
            }
        }
        catch (const std::exception&) {
            m_error_handler(std::current_exception());
        }
    }
};

// Factory with filter (e.g., data_model::data_acceptor() for persistence)
template<typename Entity, typename DataCallable, typename ErrorCallable, typename DataFilter>
auto make_data_sink(DataFilter&& data_filter, DataCallable&& data_callback, ErrorCallable&& error_callback)
{
    return data_sink<Entity, std::decay_t<DataCallable>, std::decay_t<ErrorCallable>, std::decay_t<DataFilter>>::create(
        std::forward<DataCallable>(data_callback),
        std::forward<ErrorCallable>(error_callback),
        std::forward<DataFilter>(data_filter));
}

// Factory without filter (pass-through mode)
template<typename Entity, typename DataCallable, typename ErrorCallable>
auto make_data_sink(DataCallable&& data_callback, ErrorCallable&& error_callback)
{
    return data_sink<Entity, std::decay_t<DataCallable>, std::decay_t<ErrorCallable>, std::nullptr_t>::create(
        std::forward<DataCallable>(data_callback),
        std::forward<ErrorCallable>(error_callback),
        nullptr);
}

template<typename Container>
requires std::output_iterator<std::back_insert_iterator<Container>, std::ranges::range_value_t<Container>>
auto make_data_acceptor(Container& data_cache)
{
    return [&data_cache]<std::ranges::input_range Range>(Range&& entities)
        requires std::convertible_to<std::ranges::range_value_t<Range>, std::ranges::range_value_t<Container>>
    {
        std::ranges::move(entities, std::back_inserter(data_cache));
    };
}

} // namespace datahub

#endif // DATAHUB_DATA_SINK_HPP
