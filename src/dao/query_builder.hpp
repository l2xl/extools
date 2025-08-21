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

#ifndef QUERY_BUILDER_HPP
#define QUERY_BUILDER_HPP

#include <string>
#include <vector>
#include <variant>
#include <cstdint>

namespace scratcher::dao {

/**
 * Represents a value that can be bound to a SQL parameter
 */
using QueryValue = std::variant<std::monostate, std::string, int64_t, uint64_t, double, bool>;

/**
 * Query condition operators
 */
enum class QueryOperator {
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    Like,
    In,
    Between,
    IsNull,
    IsNotNull
};

/**
 * Type-safe query condition builder
 */
template<typename Entity>
class QueryCondition {
private:
    std::string m_sql;
    std::vector<QueryValue> m_parameters;

public:
    QueryCondition() = default;
    
    // Static factory methods for creating conditions
    static QueryCondition<Entity> where(const std::string& field, QueryOperator op, const QueryValue& value) {
        std::string sql = field + " " + operator_to_sql(op) + " ?";
        std::vector<QueryValue> params = {value};
        return QueryCondition<Entity>(std::move(sql), std::move(params));
    }
    
    static QueryCondition<Entity> where_null(const std::string& field) {
        std::string sql = field + " IS NULL";
        return QueryCondition<Entity>(std::move(sql), {});
    }
    
    static QueryCondition<Entity> where_not_null(const std::string& field) {
        std::string sql = field + " IS NOT NULL";
        return QueryCondition<Entity>(std::move(sql), {});
    }
    
    static QueryCondition<Entity> where_in(const std::string& field, const std::vector<QueryValue>& values) {
        if (values.empty()) {
            return QueryCondition<Entity>("1=0", {});
        }
        
        std::string sql = field + " IN (";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) sql += ", ";
            sql += "?";
        }
        sql += ")";
        
        return QueryCondition<Entity>(std::move(sql), values);
    }
    
    static QueryCondition<Entity> where_between(const std::string& field, const QueryValue& min, const QueryValue& max) {
        std::string sql = field + " BETWEEN ? AND ?";
        std::vector<QueryValue> params = {min, max};
        return QueryCondition<Entity>(std::move(sql), std::move(params));
    }
    
    // Logical operators
    QueryCondition<Entity> and_(const QueryCondition<Entity>& other) const {
        return combine(other, true);
    }
    
    QueryCondition<Entity> or_(const QueryCondition<Entity>& other) const {
        return combine(other, false);
    }
    
    // Getters
    const std::string& sql() const { return m_sql; }
    const std::vector<QueryValue>& parameters() const { return m_parameters; }
    
    // Check if condition is empty
    bool empty() const { return m_sql.empty(); }

private:
    QueryCondition(std::string sql, std::vector<QueryValue> parameters) 
        : m_sql(std::move(sql)), m_parameters(std::move(parameters)) {}
    
    QueryCondition<Entity> combine(const QueryCondition<Entity>& other, bool is_and) const {
        if (empty()) return other;
        if (other.empty()) return *this;
        
        std::string combined_sql = "(" + m_sql + ")" + (is_and ? " AND " : " OR ") + "(" + other.m_sql + ")";
        std::vector<QueryValue> combined_params = m_parameters;
        combined_params.insert(combined_params.end(), other.m_parameters.begin(), other.m_parameters.end());
        
        return QueryCondition<Entity>(std::move(combined_sql), std::move(combined_params));
    }
    
    static std::string operator_to_sql(QueryOperator op) {
        switch (op) {
            case QueryOperator::Equal: return "=";
            case QueryOperator::NotEqual: return "!=";
            case QueryOperator::LessThan: return "<";
            case QueryOperator::LessThanOrEqual: return "<=";
            case QueryOperator::GreaterThan: return ">";
            case QueryOperator::GreaterThanOrEqual: return ">=";
            case QueryOperator::Like: return "LIKE";
            case QueryOperator::In: return "IN";
            case QueryOperator::Between: return "BETWEEN";
            case QueryOperator::IsNull: return "IS NULL";
            case QueryOperator::IsNotNull: return "IS NOT NULL";
            default: return "=";
        }
    }
};

/**
 * SQL query builder for generating type-safe queries
 */
class QueryBuilder {
public:
    // SELECT queries
    static std::string select_all(const std::string& table_name);
    static std::string select_where(const std::string& table_name, const std::string& where_clause);
    static std::string select_count(const std::string& table_name);
    static std::string select_count_where(const std::string& table_name, const std::string& where_clause);
    
    // INSERT queries
    static std::string insert(const std::string& table_name, const std::vector<std::string>& columns);
    static std::string insert_batch(const std::string& table_name, const std::vector<std::string>& columns, size_t batch_size);
    
    // UPDATE queries
    static std::string update_where(const std::string& table_name, const std::vector<std::string>& columns, const std::string& where_clause);
    
    // DELETE queries
    static std::string delete_where(const std::string& table_name, const std::string& where_clause);
    
    // DDL queries
    static std::string create_table(const std::string& table_name, const std::vector<std::string>& column_definitions);
    static std::string drop_table(const std::string& table_name);
    static std::string table_exists(const std::string& table_name);

private:
    static std::string join_columns(const std::vector<std::string>& columns, const std::string& separator = ", ");
    static std::string generate_placeholders(size_t count);
};

} // namespace scratcher::dao

#endif // QUERY_BUILDER_HPP
