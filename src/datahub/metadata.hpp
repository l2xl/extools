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

#ifndef SCRATCHER_DAO_METADATA_HPP
#define SCRATCHER_DAO_METADATA_HPP

#include "query_builder.hpp"
#include <glaze/glaze.hpp>
#include <functional>
#include <type_traits>
#include <algorithm>
#include <tuple>
#include <optional>

namespace datahub {

// Glaze-based reflection utilities
namespace detail {


// Helper to find field index by member pointer at runtime
template<typename Entity, auto MemberPtr>
constexpr std::size_t find_member_index() {
    constexpr auto field_count = glz::reflect<Entity>::size;
    constexpr auto member_names = glz::member_names<Entity>;
    
    // For now, we'll use a simple approach - find by field name
    // This assumes the member pointer corresponds to the field name
    // We'll iterate through the fields and find the matching one
    
    // Create a dummy entity to get field addresses
    Entity dummy{};
    auto tie = glz::to_tie(dummy);
    
    // Get the address of our target member
    const void* target_addr = &(dummy.*MemberPtr);
    
    // Check each field
    std::size_t result = 0;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        auto check_field = [&]<std::size_t Idx>() {
            const void* field_addr = &glz::get<Idx>(tie);
            if (field_addr == target_addr) {
                result = Idx;
            }
        };
        (check_field.template operator()<I>(), ...);
    }(std::make_index_sequence<field_count>{});
    
    return result;
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
    } else if constexpr (std::is_enum_v<T>) {
        return static_cast<T>(std::stol(str));
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
    } else if constexpr (std::is_enum_v<T>){
        return "INTEGER";
    } else if constexpr (requires { typename T::value_type; } && std::is_same_v<T, std::optional<typename T::value_type>>) {
        // Handle std::optional types - use the underlying type
        return get_sql_type<typename T::value_type>();
    } else {
        return "TEXT"; // Default to TEXT for complex types
    }
}

// Helper to create entity from tuple using Glaze reflection
template<typename Entity, typename ValueTuple>
Entity create_entity_from_tuple(const ValueTuple &values)
{
    constexpr auto field_count = glz::reflect<Entity>::size;
    constexpr auto tuple_size = std::tuple_size_v<ValueTuple>;
    static_assert(field_count == tuple_size, "Entity field count must match tuple size");

    if constexpr (field_count == 0) {
        return Entity{};
    } else {
        Entity entity{};
        auto tie = glz::to_tie(entity);

        // Assign values from tuple to fields using Glaze tuple access
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            auto assign_field = [&]<std::size_t Idx>() {
                glz::get<Idx>(tie) = std::get<Idx>(values);
            };
            (assign_field.template operator()<I>(), ...);
        }(std::make_index_sequence<field_count>{});

        return entity;
    }
}


// Helper to extract values from entity as tuple using Glaze reflection
template<typename Entity>
auto extract_entity_values_as_tuple(const Entity &entity)
{
    constexpr auto field_count = glz::reflect<Entity>::size;

    if constexpr (field_count > 0) {
        auto tie = glz::to_tie(entity);

        // Extract values as tuple using compile-time iteration
        return [&]<std::size_t... I>(std::index_sequence<I...>) {
            return std::make_tuple(glz::get<I>(tie)...);
        }(std::make_index_sequence<field_count>{});
    } else {
        return std::tuple<>{};
    }
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

} // namespace detail

/**
 * Entity metadata interface for schema generation and field access
 *
 * @tparam PrimaryKeyPtr Pointer to member field that should be the primary key
 *                       If nullptr, the first field will be used as primary key
 */
template<typename Entity, auto PrimaryKeyPtr>
class EntityMetadata
{
    struct EnsurePrivate {};
public:
    EntityMetadata(EnsurePrivate) {}

    EntityMetadata(const EntityMetadata& entity) = default;
    EntityMetadata(EntityMetadata&& entity) = default;

    EntityMetadata& operator=(const EntityMetadata& entity) = default;
    EntityMetadata& operator=(EntityMetadata&& entity) = default;

    std::string table_name;
    std::vector<std::string> column_names;
    std::vector<std::string> column_definitions;

    // Function to create entity from database tuple
    // template<typename ValueTuple>
    // Entity create_entity_from_tuple(const ValueTuple& values) const {
    //     return detail::create_entity_from_tuple<Entity>(values);
    // }

    // Function to create entity from generic database values
    template<typename GenericValue>
    Entity create_entity_from_generic_values(const std::vector<GenericValue>& values) const {
        constexpr auto field_count = glz::reflect<Entity>::size;

        if constexpr (field_count == 0) {
            return Entity{};
        } else {
            Entity entity{};
            auto tie = glz::to_tie(entity);

            // Convert generic values and assign to fields using Glaze tuple access
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                auto assign_field = [&]<std::size_t Idx>() {
                    if (Idx < values.size()) {
                        using FieldType = std::decay_t<decltype(glz::get<Idx>(tie))>;

                        // Convert from generic variant to actual field type
                        std::visit([&](const auto& variant_value) {
                            using VT = std::decay_t<decltype(variant_value)>;
                            if constexpr (std::is_same_v<VT, std::monostate>) {
                                glz::get<Idx>(tie) = FieldType{}; // Default value for null
                            } else if constexpr (std::is_same_v<FieldType, VT>) {
                                glz::get<Idx>(tie) = variant_value; // Direct assignment
                            } else if constexpr (std::is_convertible_v<VT, FieldType>) {
                                glz::get<Idx>(tie) = static_cast<FieldType>(variant_value); // Safe conversion
                            } else if constexpr (std::is_same_v<FieldType, bool> && std::is_integral_v<VT>) {
                                glz::get<Idx>(tie) = variant_value != 0; // Convert integer to bool
                            } else {
                                glz::get<Idx>(tie) = FieldType{}; // Default for incompatible types
                            }
                        }, values[Idx]);
                    }
                };
                (assign_field.template operator()<I>(), ...);
            }(std::make_index_sequence<field_count>{});

            return entity;
        }
    }
    
    // Extract values from entity as tuple for database operations
    auto extract_values_as_tuple(const Entity& entity) const {
        return detail::extract_entity_values_as_tuple(entity);
    }
    
    // Extract values from entity as vector for backward compatibility
    // std::vector<QueryValue> extract_values(const Entity& entity) const {
    //     return detail::extract_entity_values(entity);
    // }

    std::size_t primary_key_index;
    std::string primary_key_name;

    /**
     * Glaze-based metadata creation using automatic reflection
     * Automatically generates table schema from entity structure
     */
    static EntityMetadata Create()
    {
        EntityMetadata metadata(EnsurePrivate{});

        // Find primary key index
        metadata.primary_key_index = detail::find_member_index<Entity, PrimaryKeyPtr>();
        
        // Get primary key name
        constexpr auto field_names = glz::member_names<Entity>;
        metadata.primary_key_name = std::string(field_names[metadata.primary_key_index]);

        // Get type name using Glaze reflection
        constexpr auto type_name = glz::type_name<Entity>;
        metadata.table_name = std::string(type_name);

        // Convert to lowercase and replace :: with _
        std::transform(metadata.table_name.begin(), metadata.table_name.end(),
                       metadata.table_name.begin(), ::tolower);
        std::replace(metadata.table_name.begin(), metadata.table_name.end(), ':', '_');

        // Get field information using Glaze reflection
        constexpr auto field_count = glz::reflect<Entity>::size;

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
                std::string column_def = field_name + " " + detail::get_sql_type<FieldType>();

                // Add PRIMARY KEY constraint using compile-time field pointer comparison
                    // Use specified field pointer as PRIMARY KEY
                    if (metadata.primary_key_index == Idx) {
                        column_def += " PRIMARY KEY";
                    }

                metadata.column_definitions.emplace_back(std::move(column_def));
            };
            (process_field.template operator()<I>(), ...);
        }(std::make_index_sequence<field_count>{});


        return metadata;
    }
};

}

#endif //SCRATCHER_DAO_METADATA_HPP
