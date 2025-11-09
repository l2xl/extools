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

#ifndef SCRATCHER_DAO_HPP
#define SCRATCHER_DAO_HPP

#include <SQLiteCpp/SQLiteCpp.h>
#include <glaze/glaze.hpp>
#include <memory>
#include <algorithm>
#include <concepts>

#include "query_builder.hpp"
#include "operations.hpp"
#include "metadata.hpp"

namespace scratcher::dao {

class QueryCondition;

/**
 * Generic DAO implementation providing CRUD operations for any entity type
 * Uses Glaze reflection for automatic schema generation and data mapping
 * Follows RAII principles - automatically creates metadata and table from Entity type
 * Uses operation classes for encapsulated database operations with precompiled statements
 */
template<typename Entity, auto PrimaryKey>
class data_model : public std::enable_shared_from_this<data_model<Entity, PrimaryKey> >
{
public:
    typedef Entity entity_type;
    typedef EntityMetadata<entity_type, PrimaryKey> metadata_type;
private:
    std::shared_ptr<SQLite::Database> m_db;
    metadata_type m_metadata;

    struct EnsurePrivate {};
public:
    // RAII constructor - automatically creates metadata from Entity type using Glaze reflection
    // Table is created synchronously during construction (RAII guarantee)
    explicit data_model(std::shared_ptr<SQLite::Database> db, EnsurePrivate)
        : m_db(std::move(db))
        , m_metadata(metadata_type::Create())
    {
        if (!m_db) {
            throw std::invalid_argument("Database connection cannot be null");
        }
        // Create table immediately during construction (RAII guarantee)
        if (!m_db->tableExists(name())) {
            std::string sql = QueryBuilder::create_table(m_metadata.table_name, m_metadata.column_definitions);
            m_db->exec(sql);
        }
    }

    static std::shared_ptr<data_model> create(std::shared_ptr<SQLite::Database> db)
    { return std::make_shared<data_model>(db, EnsurePrivate{}); }

    const std::string& name() const
    { return m_metadata.table_name; }

    const SQLite::Database& database() const
    { return *m_db; }

    const metadata_type& metadata() const
    { return m_metadata; }

    template<std::convertible_to<Entity> T>
    void insert(const T &entity) {
        Insert<data_model> insert_op(this->shared_from_this());
        insert_op(static_cast<const Entity&>(entity));
    }

    template<std::convertible_to<Entity> T>
    bool insert_or_replace(const T &entity) {
        InsertOrReplace<data_model> insert_or_replace_op(this->shared_from_this());
        return insert_or_replace_op(static_cast<const Entity&>(entity));
    }

    // Variadic template query method - primary interface for conditional queries
    template<typename... Args>
    std::deque<Entity> query(const QueryCondition &condition = {}, Args&&... args) {
        Query<data_model> query_op(this->shared_from_this(), condition);
        return query_op(std::forward<Args>(args)...);
    }

    // Update method - updates entity by primary key
    template<std::convertible_to<Entity> T>
    void update(const T &entity) {
        Update<data_model> update_op(this->shared_from_this());
        update_op(static_cast<const Entity&>(entity));
    }

    // Variadic template remove method - primary interface for delete operations
    template<typename... Args>
    void remove(const QueryCondition &condition = {}, Args&&... args) {
        if (condition.empty()) {
            throw std::invalid_argument("Delete condition cannot be empty");
        }
        Delete<data_model> delete_op(this->shared_from_this(), condition);
        delete_op(std::forward<Args>(args)...);
    }

    // Variadic template count method - primary interface for conditional counts
    template<typename... Args>
    size_t count(const QueryCondition &condition = {}, Args&&... args) {
        Count<data_model> count_op(this->shared_from_this(), condition);
        return count_op(std::forward<Args>(args)...);
    }

    void drop_table() {
        if (m_db->tableExists(name())) {
            std::string sql = QueryBuilder::drop_table(m_metadata.table_name);
            m_db->exec(sql);
        }
    }

    // Transaction factory method
    // Transaction createTransaction() {
    //     return Transaction(m_db);
    // }
};

} // namespace scratcher::dao

#endif // SCRATCHER_DAO_HPP
