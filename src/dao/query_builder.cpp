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

#include "query_builder.hpp"
#include <sstream>

namespace scratcher::dao {

std::string QueryBuilder::select_where(const std::string& table_name, const std::string& where_clause) {
    return where_clause.empty() ? ("SELECT * FROM " + table_name) : ("SELECT * FROM " + table_name + " WHERE " + where_clause);
}

std::string QueryBuilder::select_count_where(const std::string& table_name, const std::string& where_clause) {
    return where_clause.empty() ? "SELECT COUNT(*) FROM " + table_name : "SELECT COUNT(*) FROM " + table_name + " WHERE " + where_clause;
}

std::string QueryBuilder::insert(const std::string& table_name, const std::vector<std::string>& columns) {
    return "INSERT INTO " + table_name + " (" + join_columns(columns) + ") VALUES (" +
           generate_placeholders(columns.size()) + ")";
}

std::string QueryBuilder::insert_or_replace(const std::string& table_name, const std::vector<std::string>& columns, const std::string& primary_key_name, size_t primary_key_index) {
    std::ostringstream sql;

    // INSERT INTO table (cols...) VALUES (vals...)
    sql << "INSERT INTO " << table_name << " (" << join_columns(columns) << ") VALUES ("
        << generate_placeholders(columns.size()) << ")";

    // ON CONFLICT(primary_key) DO UPDATE SET
    sql << " ON CONFLICT(" << primary_key_name << ") DO UPDATE SET ";

    // Build SET clause for all non-primary-key columns
    bool first = true;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i != primary_key_index) {
            if (!first) sql << ", ";
            sql << columns[i] << " = excluded." << columns[i];
            first = false;
        }
    }

    // Add WHERE clause to detect if any column actually changed
    sql << " WHERE ";
    first = true;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i != primary_key_index) {
            if (!first) sql << " OR ";
            // Use IS NOT to handle NULL values correctly
            sql << table_name << "." << columns[i] << " IS NOT excluded." << columns[i];
            first = false;
        }
    }

    return sql.str();
}

// std::string QueryBuilder::insert_batch(const std::string& table_name, const std::vector<std::string>& columns, size_t batch_size) {
//     std::ostringstream sql;
//     sql << "INSERT INTO " << table_name << " (" << join_columns(columns) << ") VALUES ";
//
//     for (size_t i = 0; i < batch_size; ++i) {
//         if (i > 0) sql << ", ";
//         sql << "(" << generate_placeholders(columns.size()) << ")";
//     }
//
//     return sql.str();
// }

std::string QueryBuilder::update_where(const std::string& table_name, const std::vector<std::string>& columns, const std::string& where_clause) {
    std::ostringstream sql;
    sql << "UPDATE " << table_name << " SET ";
    
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << columns[i] << " = ?";
    }
    
    sql << " WHERE " << where_clause;
    return sql.str();
}

std::string QueryBuilder::delete_where(const std::string& table_name, const std::string& where_clause) {
    return "DELETE FROM " + table_name + " WHERE " + where_clause;
}

std::string QueryBuilder::create_table(const std::string& table_name, const std::vector<std::string>& column_definitions) {
    return "CREATE TABLE IF NOT EXISTS " + table_name + " (" + join_columns(column_definitions) + ")";
}

std::string QueryBuilder::drop_table(const std::string& table_name) {
    return "DROP TABLE IF EXISTS " + table_name;
}

std::string QueryBuilder::table_exists(const std::string& table_name) {
    return "SELECT name FROM sqlite_master WHERE type='table' AND name='" + table_name + "'";
}

std::string QueryBuilder::join_columns(const std::vector<std::string>& columns, const std::string& separator) {
    if (columns.empty()) return "";
    
    std::ostringstream result;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) result << separator;
        result << columns[i];
    }
    return result.str();
}

std::string QueryBuilder::generate_placeholders(size_t count) {
    if (count == 0) return "";
    
    std::ostringstream result;
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) result << ", ";
        result << "?";
    }
    return result.str();
}

} // namespace scratcher::dao
