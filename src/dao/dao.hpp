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

#ifndef GENERIC_DAO_HPP
#define GENERIC_DAO_HPP

#include "query_builder.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <glaze/glaze.hpp>
#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <type_traits>
#include <algorithm>
#include <tuple>
#include <optional>

namespace scratcher::dao {
// Forward declarations
template<typename T>
class QueryCondition;

/**
 * Entity metadata interface for schema generation and field access
 */
template<typename Entity>
struct EntityMetadata
{
    std::string table_name;
    std::vector<std::string> column_names;
    std::vector<std::string> column_definitions;

    // Function to extract values from entity for database operations
    std::function<std::vector<QueryValue>(const Entity &)> extract_values;

    // Function to create entity from database values
    std::function<Entity(const std::vector<QueryValue> &)> create_entity;

    // Primary key field name
    std::string primary_key_field;
};

// Glaze-based reflection utilities
namespace detail {

// Convert QueryValue back to C++ types
template<typename T>
T convert_query_value(const QueryValue &value)
{
    if constexpr (std::is_same_v<T, std::string>) {
        return std::get<std::string>(value);
    } else if constexpr (std::is_same_v<T, bool>) {
        return std::get<bool>(value);
    } else if constexpr (std::is_integral_v<T>) {
        return static_cast<T>(std::get<int64_t>(value));
    } else if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(std::get<double>(value));
    } else if constexpr (requires { typename T::value_type; } && std::is_same_v<T, std::optional<typename T::value_type>>) {
        // Handle std::optional types
        using UnderlyingType = typename T::value_type;
        if (std::holds_alternative<std::monostate>(value)) {
            return T{}; // Return empty optional for null values
        } else {
            return T{convert_query_value<UnderlyingType>(value)};
        }
    } else {
        return T{}; // Default construction for complex types
    }
}

// Convert C++ types to QueryValue
template<typename T>
QueryValue convert_to_query_value(const T &value)
{
    if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else if constexpr (std::is_same_v<T, bool>) {
        return value;
    } else if constexpr (std::is_integral_v<T>) {
        return static_cast<int64_t>(value);
    } else if constexpr (std::is_floating_point_v<T>) {
        return static_cast<double>(value);
    } else if constexpr (requires { typename T::value_type; } && std::is_same_v<T, std::optional<typename T::value_type>>) {
        // Handle std::optional types
        if (value.has_value()) {
            return convert_to_query_value(value.value());
        } else {
            return std::monostate{}; // Return null for empty optional
        }
    } else {
        return std::string{}; // Default for complex types
    }
}

// Helper functions for value conversion
template<typename T>
std::string convert_to_string(const T &value)
{
    if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else if constexpr (requires { typename T::value_type; } && std::is_same_v<T, std::optional<typename T::value_type>>) {
        // Handle std::optional types
        if (value.has_value()) {
            return convert_to_string(value.value());
        } else {
            return std::string{}; // Empty string for null optional
        }
    } else {
        return std::string{}; // Default for complex types
    }
}

template<typename T>
T convert_from_string(const std::string &str)
{
    if constexpr (std::is_same_v<T, std::string>) {
        return str;
    } else if constexpr (std::is_integral_v<T>) {
        return static_cast<T>(std::stoll(str));
    } else if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(std::stod(str));
    } else if constexpr (requires { typename T::value_type; } && std::is_same_v<T, std::optional<typename T::value_type>>) {
        // Handle std::optional types
        if (str.empty()) {
            return T{}; // Return empty optional for empty strings
        } else {
            return T{convert_from_string<typename T::value_type>(str)};
        }
    } else {
        return T{}; // Default construction for complex types
    }
}

// Generate SQL column type from C++ type
template<typename T>
std::string get_sql_type()
{
    if constexpr (std::is_same_v<T, std::string>) {
        return "TEXT";
    } else if constexpr (std::is_integral_v<T>) {
        return "INTEGER";
    } else if constexpr (std::is_floating_point_v<T>) {
        return "REAL";
    } else if constexpr (std::is_same_v<T, bool>) {
        return "INTEGER"; // SQLite stores booleans as integers
    } else if constexpr (requires { typename T::value_type; } && std::is_same_v<T, std::optional<typename T::value_type>>) {
        // Handle std::optional types - use the underlying type
        return get_sql_type<typename T::value_type>();
    } else {
        return "TEXT"; // Default to TEXT for complex types
    }
}

// Helper to create entity from values using Glaze reflection
template<typename Entity>
Entity create_entity_from_values(const std::vector<QueryValue> &values)
{
    constexpr auto field_count = glz::reflect<Entity>::size;
    
    if constexpr (field_count == 0) {
        return Entity{};
    } else {
        Entity entity{};
        auto tie = glz::to_tie(entity);
        
        // Convert values and assign to fields using Glaze tuple access
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            auto assign_field = [&]<std::size_t Idx>() {
                if (Idx < values.size()) {
                    using FieldType = std::decay_t<decltype(glz::get<Idx>(tie))>;
                    glz::get<Idx>(tie) = convert_query_value<FieldType>(values[Idx]);
                }
            };
            (assign_field.template operator()<I>(), ...);
        }(std::make_index_sequence<field_count>{});
        
        return entity;
    }
}

// Helper to extract values from entity using Glaze reflection
template<typename Entity>
std::vector<QueryValue> extract_entity_values(const Entity &entity)
{
    std::vector<QueryValue> values;
    constexpr auto field_count = glz::reflect<Entity>::size;
    
    if constexpr (field_count > 0) {
        auto tie = glz::to_tie(entity);
        values.reserve(field_count);
        
        // Extract values using compile-time iteration
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (values.emplace_back(convert_to_query_value(glz::get<I>(tie))), ...);
        }(std::make_index_sequence<field_count>{});
    }
    
    return values;
}



// Helper to get field by name as string (for custom primary keys)
template<typename Entity>
std::string get_field_as_string(const Entity &entity, const std::string &field_name)
{
    constexpr auto field_count = glz::reflect<Entity>::size;
    constexpr auto field_names = glz::member_names<Entity>;
    
    std::string result;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        auto check_field = [&]<std::size_t Idx>() {
            if (field_names[Idx] == field_name) {
                auto tie = glz::to_tie(entity);
                result = convert_to_string(glz::get<Idx>(tie));
            }
        };
        (check_field.template operator()<I>(), ...);
    }(std::make_index_sequence<field_count>{});
    
    return result;
}

// Helper to set field by name from string (for custom primary keys)
template<typename Entity>
void set_field_from_string(Entity &entity, const std::string &value, const std::string &field_name)
{
    constexpr auto field_count = glz::reflect<Entity>::size;
    constexpr auto field_names = glz::member_names<Entity>;
    
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        auto set_field = [&]<std::size_t Idx>() {
            if (field_names[Idx] == field_name) {
                auto tie = glz::to_tie(entity);
                using FieldType = std::decay_t<decltype(glz::get<Idx>(tie))>;
                glz::get<Idx>(tie) = convert_from_string<FieldType>(value);
            }
        };
        (set_field.template operator()<I>(), ...);
    }(std::make_index_sequence<field_count>{});
}

/**
 * Glaze-based metadata creation using automatic reflection
 * Automatically generates table schema from entity structure
 */
template<typename Entity>
EntityMetadata<Entity> create_entity_metadata(const std::string &primary_key_field = "")
{
    EntityMetadata<Entity> metadata;

    // Get type name using Glaze reflection
    constexpr auto type_name = glz::type_name<Entity>;
    metadata.table_name = std::string(type_name);

    // Convert to lowercase and replace :: with _
    std::transform(metadata.table_name.begin(), metadata.table_name.end(),
                   metadata.table_name.begin(), ::tolower);
    std::replace(metadata.table_name.begin(), metadata.table_name.end(), ':', '_');

    // Get field information using Glaze reflection
    constexpr auto field_count = glz::reflect<Entity>::size;
    constexpr auto field_names = glz::member_names<Entity>;

    metadata.column_names.reserve(field_count);
    metadata.column_definitions.reserve(field_count);

    // Process each field using compile-time iteration
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        auto process_field = [&]<std::size_t Idx>() {
            std::string field_name{field_names[Idx]};
            metadata.column_names.emplace_back(field_name);

            // Get field type using Glaze reflection
            Entity dummy{};
            auto tie = glz::to_tie(dummy);
            using FieldType = std::decay_t<decltype(glz::get<Idx>(tie))>;

            // Generate SQL column definition
            std::string column_def = field_name + " " + get_sql_type<FieldType>();

            // Add PRIMARY KEY constraint
            if (primary_key_field.empty()) {
                // Default: first column is PRIMARY KEY
                if constexpr (Idx == 0) {
                    column_def += " PRIMARY KEY";
                }
            } else {
                // Use specified field as PRIMARY KEY
                if (field_name == primary_key_field) {
                    column_def += " PRIMARY KEY";
                }
            }

            metadata.column_definitions.emplace_back(std::move(column_def));
        };
        (process_field.template operator()<I>(), ...);
    }(std::make_index_sequence<field_count>{});

    // Set up extraction and creation functions using Glaze reflection
    metadata.extract_values = [](const Entity &entity) -> std::vector<QueryValue> {
        return extract_entity_values(entity);
    };

    metadata.create_entity = [](const std::vector<QueryValue> &values) -> Entity {
        return create_entity_from_values<Entity>(values);
    };

    // Store primary key field name
    metadata.primary_key_field = primary_key_field.empty() ? metadata.column_names[0] : primary_key_field;

    return metadata;
}

} // namespace detail

/**
 * Generic Data Access Object interface providing type-safe CRUD operations
 * All operations are async and execute on the database strand for thread safety
 */
template<typename Entity>
class IDao
{
public:
    virtual ~IDao() = default;

    // Basic CRUD operations
    virtual boost::asio::awaitable<void> insert(const Entity &entity) = 0;

    virtual boost::asio::awaitable<void> insert(const std::vector<Entity> &entities) = 0;

    virtual boost::asio::awaitable<std::vector<Entity> > queryAll() = 0;

    virtual boost::asio::awaitable<std::vector<Entity> > query(const QueryCondition<Entity> &condition) = 0;

    virtual boost::asio::awaitable<void> update(const Entity &entity) = 0;

    virtual boost::asio::awaitable<void> update(const Entity &entity, const QueryCondition<Entity> &condition) = 0;

    virtual boost::asio::awaitable<void> remove(const QueryCondition<Entity> &condition) = 0;

    // Utility operations
    virtual boost::asio::awaitable<size_t> count() = 0;

    virtual boost::asio::awaitable<size_t> count(const QueryCondition<Entity> &condition) = 0;

    // Schema management
    virtual boost::asio::awaitable<void> ensureTableExists() = 0;

    virtual boost::asio::awaitable<void> dropTable() = 0;
};

/**
 * Generic DAO implementation providing CRUD operations for any entity type
 * Uses Glaze reflection for automatic schema generation and data mapping
 * Follows RAII principles - automatically creates metadata from Entity type
 */
template<typename Entity>
class Dao : public IDao<Entity>
{
private:
    std::shared_ptr<SQLite::Database> m_db;
    boost::asio::strand<boost::asio::any_io_executor> m_db_strand;
    EntityMetadata<Entity> m_metadata;
    bool m_table_created = false;

public:
    // RAII constructor - automatically creates metadata from Entity type using Glaze reflection
    explicit Dao(std::shared_ptr<SQLite::Database> db,
                 boost::asio::strand<boost::asio::any_io_executor> db_strand,
                 const std::string &primary_key_field = "")
        : m_db(std::move(db)), m_db_strand(std::move(db_strand)),
          m_metadata(detail::create_entity_metadata<Entity>(primary_key_field))
    {
        if (!m_db) {
            throw std::invalid_argument("Database connection cannot be null");
        }
    }

    // Legacy constructor for backward compatibility
    explicit Dao(std::shared_ptr<SQLite::Database> db,
                 boost::asio::strand<boost::asio::any_io_executor> db_strand,
                 EntityMetadata<Entity> metadata)
        : m_db(std::move(db)), m_db_strand(std::move(db_strand)), m_metadata(std::move(metadata))
    {
        if (!m_db) {
            throw std::invalid_argument("Database connection cannot be null");
        }
    }

    // IDao interface implementation
    boost::asio::awaitable<void> insert(const Entity &entity) override;

    boost::asio::awaitable<void> insert(const std::vector<Entity> &entities) override;

    boost::asio::awaitable<std::vector<Entity> > queryAll() override;

    boost::asio::awaitable<std::vector<Entity> > query(const QueryCondition<Entity> &condition) override;

    boost::asio::awaitable<void> update(const Entity &entity) override;

    boost::asio::awaitable<void> update(const Entity &entity, const QueryCondition<Entity> &condition) override;

    boost::asio::awaitable<void> remove(const QueryCondition<Entity> &condition) override;

    boost::asio::awaitable<size_t> count() override;

    boost::asio::awaitable<size_t> count(const QueryCondition<Entity> &condition) override;

    boost::asio::awaitable<void> ensureTableExists() override;

    boost::asio::awaitable<void> dropTable() override;

private:
    // Helper methods
    boost::asio::awaitable<void> ensure_table_created();

    boost::asio::awaitable<std::vector<Entity> > execute_select_query(const std::string &sql,
                                                                      const std::vector<QueryValue> &params = {});

    boost::asio::awaitable<void> execute_update_query(const std::string &sql, const std::vector<QueryValue> &params);

    boost::asio::awaitable<size_t> execute_count_query(const std::string &sql,
                                                       const std::vector<QueryValue> &params = {});

    void bind_parameters(SQLite::Statement &statement, const std::vector<QueryValue> &params);

    Entity create_entity_from_statement(SQLite::Statement &statement);
};

// Template method implementations
#include "dao_impl.hpp"

} // namespace scratcher::dao

#endif // GENERIC_DAO_HPP
