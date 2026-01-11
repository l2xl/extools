// Scratcher project
// Copyright (c) 2025 l2xl (l2xl/at/proton.me)
// Distributed under the Intellectual Property Reserve License (IPRL)
// -----BEGIN PGP PUBLIC KEY BLOCK-----
//
// mDMEYdxcVRYJKwYBBAHaRw8BAQdAfacBVThCP5QDPEgSbSIudtpJS4Y4Imm5dzaN
// lM1HTem0IkwyIFhsIChsMnhsKSA8bDJ4bEBwcm90b21tYWlsLmNvbT6IkAQTFggA
// OBYhBKRCfUyWnduCkisNl+WRcOaCK79JBQJh3FxVAhsDBQsJCAcCBhUKCQgLAgQW
// AgMBAh4BAheAAAoJEOWRcOaCK79JDl8A/0/AjYVbAURZJXP3tHRgZyYyN9txT6mW
// 0bYCcOf0rZ4NAQDoFX4dytPDvcjV7ovSQJ6dzvIoaRbKWGbHRCufrm5QBA==
// =KKu7
// -----END PGP PUBLIC KEY BLOCK-----

#ifndef DATAHUB_DATA_PROVIDER_HPP
#define DATAHUB_DATA_PROVIDER_HPP

#include <memory>
#include <deque>
#include <list>
#include <functional>

#include "data_model.hpp"
#include "data_sink.hpp"
#include "generic_handler.hpp"

namespace datahub {

/**
 * Generic data provider managing entity lifecycle: DB persistence, memory cache, and subscriptions.
 * Uses RAII - DB table and data_sink are created during construction.
 *
 * Template parameters:
 * - Entity: The data entity type
 * - PrimaryKey: Pointer to member for primary key field
 * - ErrorHandler: Error callback type (inferred by make_data_provider)
 */
template<typename Entity, auto PrimaryKey>
class data_provider : public std::enable_shared_from_this<data_provider<Entity, PrimaryKey>>
{
public:
    using entity_type = Entity;
    using model_type = data_model<Entity, PrimaryKey>;
    using cache_type = std::deque<entity_type>;
    using subscription_handler = std::function<void(const cache_type&)>;

private:
    using data_handler_type = std::function<void(cache_type&&)>;
    using sink_filter_type = decltype(std::declval<model_type>().template data_acceptor<cache_type>());
    using sink_type = data_sink<entity_type, sink_filter_type>;

    std::shared_ptr<model_type> m_model;
    cache_type m_cache;
    std::shared_ptr<sink_type> m_sink;

    std::list<subscription_handler> m_subscribers;

    static void handle_new_entities(std::weak_ptr<data_provider> ref, cache_type&& new_entities) {
        if (auto self = ref.lock()) {
            self->handle_new_entities(std::move(new_entities));
        }
    }

    void handle_new_entities(cache_type&& new_entities) {
        std::ranges::move(new_entities, std::back_inserter(m_cache));
        for (const auto& handler : m_subscribers) {
            handler(m_cache);
        }
    }

public:
    data_provider(std::shared_ptr<SQLite::Database> db)
        : m_model(model_type::create(std::move(db)))
        , m_sink()
    {}
    virtual ~data_provider() = default;

    template<typename ErrorCallable>
    static std::shared_ptr<data_provider> create(std::shared_ptr<SQLite::Database> db, ErrorCallable&& error_handler)
    {
        auto self = std::make_shared<generic_handler<cache_type&&, data_provider, void(*)(cache_type&&), ErrorCallable, std::shared_ptr<SQLite::Database>>>(
            [](cache_type&&){}, std::forward<ErrorCallable>(error_handler), std::move(db));

        // Load cached data from DB
        self->m_cache = self->m_model->query();

        // Create data sink linking DB persistence and memory cache
        std::weak_ptr<data_provider> ref = self;

        self->m_sink = make_data_sink<entity_type>(
            self->m_model->template data_acceptor<cache_type>(),
            [ref](cache_type&& entities) { handle_new_entities(ref, std::move(entities)); },
            [ref](auto eptr) { if (auto self = ref.lock()) self->handle_error(eptr); }
        );

        return std::static_pointer_cast<data_provider>(self);
    }

    const cache_type& cache() const
    { return m_cache; }

    std::shared_ptr<model_type> model() const
    { return m_model; }

    std::shared_ptr<sink_type> sink() const
    { return m_sink; }

    template<std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, entity_type>
    auto data_acceptor()
    { return m_sink->template data_acceptor<Range>(); }

    void subscribe(subscription_handler handler)
    {
        m_subscribers.emplace_back(std::move(handler));

        // Immediately notify with current cache if not empty
        if (!m_cache.empty()) {
            m_subscribers.back()(m_cache);
        }
    }


    virtual void handle_data(cache_type&& data) = 0;
    virtual void handle_error(std::exception_ptr eptr) = 0;
};

template<typename Entity, auto PrimaryKey, typename ErrorCallable>
auto make_data_provider(std::shared_ptr<SQLite::Database> db, ErrorCallable&& error_handler)
{
    return data_provider<Entity, PrimaryKey>::create(std::move(db), std::forward<ErrorCallable>(error_handler));
}

} // namespace datahub

#endif // DATAHUB_DATA_PROVIDER_HPP
