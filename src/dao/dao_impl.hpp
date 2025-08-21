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

// Template method implementations for Dao<Entity>
// This file is included by dao.hpp to provide template instantiations

#include <stdexcept>
#include <sqlite3.h>
#include <algorithm>

template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::insert(const Entity& entity) {
    co_await ensure_table_created();
    
    std::string sql = QueryBuilder::insert(m_metadata.table_name, m_metadata.column_names);
    std::vector<QueryValue> values = m_metadata.extract_values(entity);
    
    co_await execute_update_query(sql, values);
}

template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::insert(const std::vector<Entity>& entities) {
    if (entities.empty()) {
        co_return;
    }
    
    co_await ensure_table_created();
    
    std::string sql = QueryBuilder::insert_batch(m_metadata.table_name, m_metadata.column_names, entities.size());
    
    std::vector<QueryValue> all_values;
    for (const auto& entity : entities) {
        std::vector<QueryValue> entity_values = m_metadata.extract_values(entity);
        all_values.insert(all_values.end(), entity_values.begin(), entity_values.end());
    }
    
    co_await execute_update_query(sql, all_values);
}



template<typename Entity>
boost::asio::awaitable<std::vector<Entity>> Dao<Entity>::queryAll() {
    co_await ensure_table_created();
    
    std::string sql = QueryBuilder::select_all(m_metadata.table_name);
    co_return co_await execute_select_query(sql);
}

template<typename Entity>
boost::asio::awaitable<std::vector<Entity>> Dao<Entity>::query(const QueryCondition<Entity>& condition) {
    co_await ensure_table_created();
    
    if (condition.empty()) {
        co_return co_await queryAll();
    }
    
    std::string sql = QueryBuilder::select_where(m_metadata.table_name, condition.sql());
    co_return co_await execute_select_query(sql, condition.parameters());
}

template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::update(const Entity& entity) {
    co_await ensure_table_created();
    
    // Find primary key column index
    auto pk_it = std::find(m_metadata.column_names.begin(), m_metadata.column_names.end(), m_metadata.primary_key_field);
    if (pk_it == m_metadata.column_names.end()) {
        throw std::invalid_argument("Primary key field '" + m_metadata.primary_key_field + "' not found in entity columns");
    }
    size_t pk_index = std::distance(m_metadata.column_names.begin(), pk_it);
    
    // Get primary key value
    std::vector<QueryValue> all_values = m_metadata.extract_values(entity);
    QueryValue pk_value = all_values[pk_index];
    
    // Build non-primary-key columns list
    std::vector<std::string> non_pk_columns;
    for (size_t i = 0; i < m_metadata.column_names.size(); ++i) {
        if (i != pk_index) {
            non_pk_columns.push_back(m_metadata.column_names[i]);
        }
    }
    
    // Build WHERE clause for primary key
    std::string where_clause = m_metadata.primary_key_field + " = ?";
    std::string sql = QueryBuilder::update_where(m_metadata.table_name, non_pk_columns, where_clause);
    
    // Build parameter values (non-PK values + PK value for WHERE clause)
    std::vector<QueryValue> update_values;
    for (size_t i = 0; i < all_values.size(); ++i) {
        if (i != pk_index) {
            update_values.push_back(all_values[i]);
        }
    }
    update_values.push_back(pk_value);
    
    co_await execute_update_query(sql, update_values);
}

template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::update(const Entity& entity, const QueryCondition<Entity>& condition) {
    co_await ensure_table_created();
    
    if (condition.empty()) {
        throw std::invalid_argument("Update condition cannot be empty");
    }
    
    // Find primary key column index
    auto pk_it = std::find(m_metadata.column_names.begin(), m_metadata.column_names.end(), m_metadata.primary_key_field);
    if (pk_it == m_metadata.column_names.end()) {
        throw std::invalid_argument("Primary key field '" + m_metadata.primary_key_field + "' not found in entity columns");
    }
    size_t pk_index = std::distance(m_metadata.column_names.begin(), pk_it);
    
    // Build non-primary-key columns list
    std::vector<std::string> non_pk_columns;
    for (size_t i = 0; i < m_metadata.column_names.size(); ++i) {
        if (i != pk_index) {
            non_pk_columns.push_back(m_metadata.column_names[i]);
        }
    }
    
    std::string sql = QueryBuilder::update_where(m_metadata.table_name, non_pk_columns, condition.sql());
    
    // Build parameter values (non-PK values + condition parameters)
    std::vector<QueryValue> all_values = m_metadata.extract_values(entity);
    std::vector<QueryValue> update_values;
    for (size_t i = 0; i < all_values.size(); ++i) {
        if (i != pk_index) {
            update_values.push_back(all_values[i]);
        }
    }
    update_values.insert(update_values.end(), condition.parameters().begin(), condition.parameters().end());
    
    co_await execute_update_query(sql, update_values);
}



template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::remove(const QueryCondition<Entity>& condition) {
    co_await ensure_table_created();
    
    if (condition.empty()) {
        throw std::invalid_argument("Delete condition cannot be empty");
    }
    
    std::string sql = QueryBuilder::delete_where(m_metadata.table_name, condition.sql());
    co_await execute_update_query(sql, condition.parameters());
}

template<typename Entity>
boost::asio::awaitable<size_t> Dao<Entity>::count() {
    co_await ensure_table_created();
    
    std::string sql = QueryBuilder::select_count(m_metadata.table_name);
    co_return co_await execute_count_query(sql);
}

template<typename Entity>
boost::asio::awaitable<size_t> Dao<Entity>::count(const QueryCondition<Entity>& condition) {
    co_await ensure_table_created();
    
    if (condition.empty()) {
        co_return co_await count();
    }
    
    std::string sql = QueryBuilder::select_count_where(m_metadata.table_name, condition.sql());
    co_return co_await execute_count_query(sql, condition.parameters());
}



template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::ensureTableExists() {
    co_await ensure_table_created();
}

template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::dropTable() {
    co_await boost::asio::post(m_db_strand, boost::asio::use_awaitable);
    
    std::string sql = QueryBuilder::drop_table(m_metadata.table_name);
    m_db->exec(sql);
    m_table_created = false;
}

template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::ensure_table_created() {
    if (m_table_created) {
        co_return;
    }
    
    co_await boost::asio::post(m_db_strand, boost::asio::use_awaitable);
    
    std::string sql = QueryBuilder::create_table(m_metadata.table_name, m_metadata.column_definitions);
    m_db->exec(sql);
    m_table_created = true;
}

template<typename Entity>
boost::asio::awaitable<std::vector<Entity>> Dao<Entity>::execute_select_query(const std::string& sql, const std::vector<QueryValue>& params) {
    co_await boost::asio::post(m_db_strand, boost::asio::use_awaitable);
    
    std::vector<Entity> results;
    SQLite::Statement statement(*m_db, sql);
    
    bind_parameters(statement, params);
    
    while (statement.executeStep()) {
        results.push_back(create_entity_from_statement(statement));
    }
    
    co_return results;
}

template<typename Entity>
boost::asio::awaitable<void> Dao<Entity>::execute_update_query(const std::string& sql, const std::vector<QueryValue>& params) {
    co_await boost::asio::post(m_db_strand, boost::asio::use_awaitable);
    
    SQLite::Statement statement(*m_db, sql);
    bind_parameters(statement, params);
    statement.exec();
}

template<typename Entity>
boost::asio::awaitable<size_t> Dao<Entity>::execute_count_query(const std::string& sql, const std::vector<QueryValue>& params) {
    co_await boost::asio::post(m_db_strand, boost::asio::use_awaitable);
    
    SQLite::Statement statement(*m_db, sql);
    bind_parameters(statement, params);
    
    if (statement.executeStep()) {
        co_return static_cast<size_t>(statement.getColumn(0).getInt64());
    }
    
    co_return 0;
}

template<typename Entity>
void Dao<Entity>::bind_parameters(SQLite::Statement& statement, const std::vector<QueryValue>& params) {
    for (size_t i = 0; i < params.size(); ++i) {
        int param_index = static_cast<int>(i + 1);
        
        std::visit([&statement, param_index](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                statement.bind(param_index, value);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                statement.bind(param_index, value);
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                statement.bind(param_index, static_cast<int64_t>(value));
            } else if constexpr (std::is_same_v<T, double>) {
                statement.bind(param_index, value);
            } else if constexpr (std::is_same_v<T, bool>) {
                statement.bind(param_index, value ? 1 : 0);
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                statement.bind(param_index); // Bind NULL
            }
        }, params[i]);
    }
}

template<typename Entity>
Entity Dao<Entity>::create_entity_from_statement(SQLite::Statement& statement) {
    std::vector<QueryValue> values;
    
    for (int i = 0; i < statement.getColumnCount(); ++i) {
        SQLite::Column column = statement.getColumn(i);
        
        switch (column.getType()) {
            case SQLITE_INTEGER:
                values.emplace_back(column.getInt64());
                break;
            case SQLITE_FLOAT:
                values.emplace_back(column.getDouble());
                break;
            case SQLITE_TEXT:
                values.emplace_back(column.getString());
                break;
            case SQLITE_NULL:
                values.emplace_back(std::monostate{});
                break;
            default:
                values.emplace_back(std::string{});
                break;
        }
    }
    
    return m_metadata.create_entity(values);
}
