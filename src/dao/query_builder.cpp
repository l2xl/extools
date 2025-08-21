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

// QueryBuilder implementation
std::string QueryBuilder::select_all(const std::string& table_name) {
    return "SELECT * FROM " + table_name;
}

std::string QueryBuilder::select_where(const std::string& table_name, const std::string& where_clause) {
    return "SELECT * FROM " + table_name + " WHERE " + where_clause;
}

std::string QueryBuilder::select_count(const std::string& table_name) {
    return "SELECT COUNT(*) FROM " + table_name;
}

std::string QueryBuilder::select_count_where(const std::string& table_name, const std::string& where_clause) {
    return "SELECT COUNT(*) FROM " + table_name + " WHERE " + where_clause;
}

std::string QueryBuilder::insert(const std::string& table_name, const std::vector<std::string>& columns) {
    return "INSERT INTO " + table_name + " (" + join_columns(columns) + ") VALUES (" + 
           generate_placeholders(columns.size()) + ")";
}

std::string QueryBuilder::insert_batch(const std::string& table_name, const std::vector<std::string>& columns, size_t batch_size) {
    std::ostringstream sql;
    sql << "INSERT INTO " << table_name << " (" << join_columns(columns) << ") VALUES ";
    
    for (size_t i = 0; i < batch_size; ++i) {
        if (i > 0) sql << ", ";
        sql << "(" << generate_placeholders(columns.size()) << ")";
    }
    
    return sql.str();
}

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
