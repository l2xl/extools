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

namespace datahub {

/**
 * Generic data provider managing entity lifecycle: DB persistence, memory cache, and subscriptions.
 * Uses RAII - DB table and data_sink are created during construction.
 *
 * Template parameters:
 * - Entity: The data entity type
 * - PrimaryKey: Pointer to member for primary key field
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
    std::shared_ptr<model_type> m_model;
    cache_type m_cache;
    std::list<subscription_handler> m_subscribers;

    using sink_data_handler = std::function<void(cache_type&&)>;
    using sink_error_handler = std::function<void(std::exception_ptr)>;
    using sink_filter_type = decltype(std::declval<model_type>().template data_acceptor<cache_type>());
    using sink_type = data_sink<entity_type, sink_data_handler, sink_error_handler, sink_filter_type>;

    std::shared_ptr<sink_type> m_sink;

    struct ensure_private {};

    void notify_subscribers() {
        for (const auto& handler : m_subscribers) {
            handler(m_cache);
        }
    }

public:
    data_provider(std::shared_ptr<SQLite::Database> db, ensure_private)
        : m_model(model_type::create(std::move(db)))
    {}

    static std::shared_ptr<data_provider> create(std::shared_ptr<SQLite::Database> db)
    {
        auto self = std::make_shared<data_provider>(std::move(db), ensure_private{});

        // Load cached data from DB
        self->m_cache = self->m_model->query();

        // Create data sink linking DB persistence and memory cache
        std::weak_ptr<data_provider> ref = self;

        sink_data_handler data_handler = [ref](cache_type&& entities) {
            if (auto self = ref.lock()) {
                std::ranges::move(entities, std::back_inserter(self->m_cache));
                self->notify_subscribers();
            }
        };

        sink_error_handler error_handler = [](std::exception_ptr) {};

        self->m_sink = sink_type::create(
            std::move(data_handler),
            std::move(error_handler),
            self->m_model->template data_acceptor<cache_type>()
        );

        // Notify subscribers with initial cache if not empty
        if (!self->m_cache.empty()) {
            self->notify_subscribers();
        }

        return self;
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

    template<typename... Args>
    void subscribe(const QueryCondition& condition, subscription_handler handler, Args&&... args)
    {
        m_subscribers.emplace_back(std::move(handler));

        // Query with condition and notify immediately
        auto filtered = m_model->query(condition, std::forward<Args>(args)...);
        if (!filtered.empty()) {
            m_subscribers.back()(filtered);
        }
    }
};

} // namespace datahub

#endif // DATAHUB_DATA_PROVIDER_HPP