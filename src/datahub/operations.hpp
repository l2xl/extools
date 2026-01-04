#ifndef SCRATCHER_DAO_OPERATIONS_HPP
#define SCRATCHER_DAO_OPERATIONS_HPP

#include "data_model.hpp"
#include "query_builder.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>
#include <utility>
#include <deque>

#include "sqlite3.h"

namespace datahub {


/**
 * Base class for all database operations
 * Encapsulates compiled SQLite statement and provides common functionality
 */
template<typename DAO>
class BaseOperation {
protected:
    SQLite::Statement m_statement;
    std::shared_ptr<DAO> m_dao;

public:
    explicit BaseOperation(const std::string& sql, std::shared_ptr<DAO> dao)
        : m_statement(dao->database(), sql), m_dao(std::move(dao)) {}
    
    virtual ~BaseOperation() = default;
    
    // Non-copyable but movable
    BaseOperation(const BaseOperation&) = delete;
    BaseOperation& operator=(const BaseOperation&) = delete;
    BaseOperation(BaseOperation&&) = default;
    BaseOperation& operator=(BaseOperation&&) = default;

protected:
    // Variadic template version for direct parameter binding
    template<typename... Args>
    void bind_parameters(Args&&... args) {
        m_statement.reset();
        bind_parameter_impl<1>(std::forward<Args>(args)...);
    }
    
    // Helper to bind parameters from tuple (when we need to convert tuple to variadic args)
    template<typename... Args>
    void bind_parameters_from_tuple(const std::tuple<Args...>& params) {
        m_statement.reset();
        std::apply([this](const auto&... args) {
            bind_parameter_impl<1>(args...);
        }, params);
    }
    
    // Bind a single parameter at the given index (1-based)
    template<typename T>
    void bind_parameter(int index, T&& value) {
        bind_single_parameter_dynamic(index, std::forward<T>(value));
    }

private:
    // Recursive template implementation for parameter binding
    template<int Index>
    void bind_parameter_impl() {
        // Base case - no more parameters to bind
    }
    
    template<int Index, typename T, typename... Rest>
    void bind_parameter_impl(T&& first, Rest&&... rest) {
        bind_single_parameter<Index>(std::forward<T>(first));
        bind_parameter_impl<Index + 1>(std::forward<Rest>(rest)...);
    }
    
    // Bind a single parameter at the given index
    template<int Index, typename T>
    void bind_single_parameter(T&& value) {
        using DecayedT = std::decay_t<T>;
        
        if constexpr (std::is_same_v<DecayedT, std::string>) {
            m_statement.bind(Index, std::forward<T>(value));
        } else if constexpr (std::is_same_v<DecayedT, const char*>) {
            m_statement.bind(Index, std::string(value));
        } else if constexpr (std::is_same_v<DecayedT, int64_t>) {
            m_statement.bind(Index, std::forward<T>(value));
        } else if constexpr (std::is_same_v<DecayedT, uint64_t>) {
            m_statement.bind(Index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, int>) {
            m_statement.bind(Index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, unsigned int>) {
            m_statement.bind(Index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, double>) {
            m_statement.bind(Index, std::forward<T>(value));
        } else if constexpr (std::is_same_v<DecayedT, float>) {
            m_statement.bind(Index, static_cast<double>(value));
        } else if constexpr (std::is_same_v<DecayedT, bool>) {
            m_statement.bind(Index, value ? 1 : 0);
        } else if constexpr (std::is_enum_v<DecayedT>) {
            m_statement.bind(Index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, std::monostate>) {
            m_statement.bind(Index); // Bind NULL
        } else if constexpr (std::is_same_v<DecayedT, std::optional<typename DecayedT::value_type>>) {
            // Handle std::optional types
            if (value.has_value()) {
                bind_single_parameter<Index>(value.value());
            } else {
                m_statement.bind(Index); // Bind NULL
            }
        } else {
            static_assert(sizeof(T) == 0, "Unsupported parameter type for bind_parameters");
        }
    }
    
    // Runtime version of bind_single_parameter
    template<typename T>
    void bind_single_parameter_dynamic(int index, T&& value) {
        using DecayedT = std::decay_t<T>;
        
        if constexpr (std::is_same_v<DecayedT, std::string>) {
            m_statement.bind(index, std::forward<T>(value));
        } else if constexpr (std::is_same_v<DecayedT, const char*>) {
            m_statement.bind(index, std::string(value));
        } else if constexpr (std::is_same_v<DecayedT, int64_t>) {
            m_statement.bind(index, std::forward<T>(value));
        } else if constexpr (std::is_same_v<DecayedT, uint64_t>) {
            m_statement.bind(index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, int>) {
            m_statement.bind(index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, unsigned int>) {
            m_statement.bind(index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, double>) {
            m_statement.bind(index, std::forward<T>(value));
        } else if constexpr (std::is_same_v<DecayedT, float>) {
            m_statement.bind(index, static_cast<double>(value));
        } else if constexpr (std::is_same_v<DecayedT, bool>) {
            m_statement.bind(index, value ? 1 : 0);
        } else if constexpr (std::is_enum_v<DecayedT>) {
            m_statement.bind(index, static_cast<int64_t>(value));
        } else if constexpr (std::is_same_v<DecayedT, std::monostate>) {
            m_statement.bind(index); // Bind NULL
        } else if constexpr (std::is_same_v<DecayedT, std::optional<typename DecayedT::value_type>>) {
            // Handle std::optional types
            if (value.has_value()) {
                bind_single_parameter_dynamic(index, value.value());
            } else {
                m_statement.bind(index); // Bind NULL
            }
        } else {
            static_assert(sizeof(T) == 0, "Unsupported parameter type for bind_parameters");
        }
    }
};

/**
 * Insert operation - compiles INSERT statement in constructor
 */
template<typename DAO>
class Insert : public BaseOperation<DAO> {

public:
    explicit Insert(std::shared_ptr<DAO> dao)
        : BaseOperation<DAO>(QueryBuilder::insert(dao->name(), dao->metadata().column_names), std::move(dao))
    {}

    // Execute single entity insert - reuses precompiled statement
    void operator()(const typename DAO::entity_type& entity) {
        auto values_tuple = BaseOperation<DAO>::m_dao->metadata().extract_values_as_tuple(entity);
        BaseOperation<DAO>::bind_parameters_from_tuple(values_tuple);
        BaseOperation<DAO>::m_statement.exec();
    }
};

/**
 * InsertOrReplace operation - compiles INSERT ON CONFLICT DO UPDATE WHERE statement in constructor
 * Returns true if data was inserted or updated, false if data was unchanged
 */
template<typename DAO>
class InsertOrReplace : public BaseOperation<DAO> {

public:
    explicit InsertOrReplace(std::shared_ptr<DAO> dao)
        : BaseOperation<DAO>(QueryBuilder::insert_or_replace(dao->name(), dao->metadata().column_names, dao->metadata().primary_key_name, dao->metadata().primary_key_index), std::move(dao))
    {}

    // Execute single entity insert or replace - reuses precompiled statement
    // Returns true if data was modified (inserted or updated), false if unchanged
    bool operator()(const typename DAO::entity_type& entity) {
        auto values_tuple = BaseOperation<DAO>::m_dao->metadata().extract_values_as_tuple(entity);
        BaseOperation<DAO>::bind_parameters_from_tuple(values_tuple);
        BaseOperation<DAO>::m_statement.exec();

        // Get the number of rows modified (0 if WHERE clause prevented update)
        sqlite3* db_handle = const_cast<SQLite::Database&>(BaseOperation<DAO>::m_dao->database()).getHandle();
        int changes = sqlite3_changes(db_handle);

        // changes > 0 means either INSERT (1) or UPDATE (1) happened
        // changes == 0 means WHERE clause prevented update (data unchanged)
        return changes > 0;
    }
};

/**
 * Query operation - compiles SELECT statement in constructor
 */
template<typename DAO>
class Query : public BaseOperation<DAO> {
    size_t m_required_param_count;
    
public:
    // Constructor for SELECT * FROM table WHERE condition
    Query(std::shared_ptr<DAO> dao, const QueryCondition& condition = {})
        : BaseOperation<DAO>(QueryBuilder::select_where(dao->name(), condition.sql()), std::move(dao))
        , m_required_param_count(condition.parameter_count()) {}
    
    // Execute query - accepts variable arguments pack
    template<typename... Args>
    std::deque<typename DAO::entity_type> operator()(Args&&... args) {
        if (sizeof...(args) != m_required_param_count) {
            throw std::logic_error("Query requires exactly " + std::to_string(m_required_param_count) + " parameters, got " + std::to_string(sizeof...(args)));
        }
        return execute_query(std::forward<Args>(args)...);
    }
    
private:
    template<typename... Args>
    std::deque<typename DAO::entity_type> execute_query(Args&&... args) {
        std::deque<typename DAO::entity_type> results;
        BaseOperation<DAO>::bind_parameters(std::forward<Args>(args)...);
        
        while (BaseOperation<DAO>::m_statement.executeStep()) {
            results.push_back(create_entity_from_statement());
        }
        
        return results;
    }

    //TODO: This uses wrong bahavior: It gets DB statement and extract types from it.
    // Instead it should get metadata and extract its types
    typename DAO::entity_type create_entity_from_statement() {
        // Use a generic approach to create a tuple from database columns
        // We'll create a generic tuple and let the metadata handle the conversion
        auto create_tuple_from_columns = [&]() {
            int column_count = BaseOperation<DAO>::m_statement.getColumnCount();

            // Create a vector to hold generic values, then convert to tuple in metadata
            std::vector<std::variant<std::string, int64_t, double, bool, std::monostate>> generic_values;
            generic_values.reserve(column_count);

            for (int i = 0; i < column_count; ++i) {
                SQLite::Column column = BaseOperation<DAO>::m_statement.getColumn(i);

                switch (column.getType()) {
                    case SQLITE_INTEGER:
                        generic_values.emplace_back(column.getInt64());
                        break;
                    case SQLITE_FLOAT:
                        generic_values.emplace_back(column.getDouble());
                        break;
                    case SQLITE_TEXT:
                        generic_values.emplace_back(column.getString());
                        break;
                    case SQLITE_NULL:
                        generic_values.emplace_back(std::monostate{});
                        break;
                    default:
                        generic_values.emplace_back(std::string{});
                        break;
                }
            }

            return generic_values;
        };

        auto values = create_tuple_from_columns();

        // Convert the generic values to a proper tuple using the metadata
        return BaseOperation<DAO>::m_dao->metadata().create_entity_from_generic_values(values);
    }
};

// Helper to remove element from tuple by index
template <std::size_t N, typename Tuple, std::size_t... Is>
auto remove_element_impl(Tuple&& t, std::index_sequence<Is...>) {
    // This creates a new tuple by taking elements from the original tuple
    // based on the index sequence, effectively skipping index N.
    return std::make_tuple(std::get<(Is < N ? Is : Is + 1)>(std::forward<Tuple>(t))...);
}

template <std::size_t N, typename... Args>
auto remove_element(const std::tuple<Args...>& t) {
    static_assert(N < sizeof...(Args), "Index out of bounds for tuple.");
    // Create an index sequence for the new tuple (one element less).
    return remove_element_impl<N>(t, std::make_index_sequence<sizeof...(Args) - 1>{});
}


/**
 * Update operation - compiles UPDATE statement in constructor
 * Updates entity by primary key (extracted from entity itself)
 */
template<typename DAO>
class Update : public BaseOperation<DAO> {
    std::vector<std::string> m_non_pk_columns;

    using BaseOperation<DAO>::m_dao;
public:
    // Constructor for UPDATE by primary key - compiles statement once
    explicit Update(std::shared_ptr<DAO> dao)
        : BaseOperation<DAO>("", std::move(dao)) {
        
        // Build non-primary-key columns list
        for (size_t i = 0; i < m_dao->metadata().column_names.size(); ++i) {
            if (i != m_dao->metadata().primary_key_index) {
                m_non_pk_columns.push_back(m_dao->metadata().column_names[i]);
            }
        }
        
        // Build WHERE clause for primary key and compile statement
        std::string where_clause = m_dao->metadata().primary_key_name + " = ?";
        std::string sql = QueryBuilder::update_where(m_dao->name(), m_non_pk_columns, where_clause);
        BaseOperation<DAO>::m_statement = SQLite::Statement(m_dao->database(), sql);
    }
    
    // Execute update using primary key from entity
    void operator()(const typename DAO::entity_type& entity) {
        auto all_values = m_dao->metadata().extract_values_as_tuple(entity);

        // Reset statement before binding
        BaseOperation<DAO>::m_statement.reset();

        // Use tuple_size to get the number of elements
        constexpr auto tuple_size = std::tuple_size_v<decltype(all_values)>;

        // Bind non-primary key values first
        int param_index = 1;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            auto bind_if_not_pk = [&]<std::size_t Idx>() mutable {
                if (Idx != m_dao->metadata().primary_key_index) {
                    BaseOperation<DAO>::bind_parameter(param_index++, std::get<Idx>(all_values));
                }
            };
            (bind_if_not_pk.template operator()<I>(), ...);
        }(std::make_index_sequence<tuple_size>{});

        // Bind primary key value for WHERE clause using tuple access
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            auto bind_pk = [&]<std::size_t Idx>() {
                if (Idx == m_dao->metadata().primary_key_index) {
                    BaseOperation<DAO>::bind_parameter(param_index, std::get<Idx>(all_values));
                }
            };
            (bind_pk.template operator()<I>(), ...);
        }(std::make_index_sequence<tuple_size>{});

        BaseOperation<DAO>::m_statement.exec();
    }
};

/**
 * Delete operation - compiles DELETE statement in constructor
 */
template<typename DAO>
class Delete : public BaseOperation<DAO> {
    size_t m_required_param_count;
    
public:
    // Constructor compiles DELETE statement once with condition
    Delete(std::shared_ptr<DAO> dao, const QueryCondition& condition = {})
        : BaseOperation<DAO>(QueryBuilder::delete_where(dao->name(), condition.sql()), std::move(dao))
        , m_required_param_count(condition.parameter_count())
    {}
    
    // Execute delete with variable arguments pack - reuses precompiled statement
    template<typename... Args>
    void operator()(Args&&... args) {
        if (sizeof...(args) != m_required_param_count) {
            throw std::logic_error("Delete requires exactly " + std::to_string(m_required_param_count) + " parameters, got " + std::to_string(sizeof...(args)));
        }
//        std::vector<QueryValue> params = {QueryValue(std::forward<Args>(args))...};
        BaseOperation<DAO>::bind_parameters(std::forward<Args>(args)...);
        BaseOperation<DAO>::m_statement.exec();
    }
};

/**
 * Count operation - compiles SELECT COUNT(*) statement in constructor
 */
template<typename DAO>
class Count : public BaseOperation<DAO> {
    size_t m_required_param_count;
    
public:
    // Constructor for COUNT(*) - compiles statement once
    // explicit Count(std::shared_ptr<DAO> dao)
    //     : BaseOperation<DAO>(QueryBuilder::select_count(dao->name()), std::move(dao))
    //     , m_required_param_count(0)
    // {}
    
    // Constructor for COUNT(*) WHERE condition - compiles statement once
    Count(std::shared_ptr<DAO> dao, const QueryCondition& condition = {})
        : BaseOperation<DAO>(QueryBuilder::select_count_where(dao->name(), condition.sql()), std::move(dao))
        , m_required_param_count(condition.parameter_count()) {}
    
    // Execute count query - accepts variable arguments pack
    template<typename... Args>
    size_t operator()(Args&&... args) {
        if (sizeof...(args) != m_required_param_count) {
            throw std::logic_error("Count requires exactly " + std::to_string(m_required_param_count) + " parameters, got " + std::to_string(sizeof...(args)));
        }
        return execute_count(std::forward<Args>(args)...);
    }
    
private:
    template<typename... Args>
    size_t execute_count(Args&&... args) {
        BaseOperation<DAO>::bind_parameters(std::forward<Args>(args)...);
        
        if (BaseOperation<DAO>::m_statement.executeStep()) {
            return static_cast<size_t>(BaseOperation<DAO>::m_statement.getColumn(0).getInt64());
        }
        
        return 0;
    }
};

/**
 * Transaction class following RAII and builder pattern
 * Collects operations and executes them atomically in operator()
 */
// class Transaction {
//     std::shared_ptr<SQLite::Database> m_db;
//     std::vector<std::function<void()>> m_operations;
//     bool m_executed = false;
//
// public:
//     explicit Transaction(std::shared_ptr<SQLite::Database> db)
//         : m_db(std::move(db)) {}
//
//     // Non-copyable but movable
//     Transaction(const Transaction&) = delete;
//     Transaction& operator=(const Transaction&) = delete;
//     Transaction(Transaction&&) = default;
//     Transaction& operator=(Transaction&&) = default;
//
//     // Add operation to transaction
//     template<typename Operation, typename... Args>
//     Transaction& add(Operation&& operation, Args&&... args) {
//         if (m_executed) {
//             throw std::logic_error("Cannot add operations to already executed transaction");
//         }
//
//         m_operations.emplace_back([op = std::forward<Operation>(operation), args...]() mutable {
//             op(args...);
//         });
//
//         return *this;
//     }
//
//     // Execute all operations in transaction
//     void operator()() {
//         if (m_executed) {
//             throw std::logic_error("Transaction already executed");
//         }
//
//         SQLite::Transaction transaction(*m_db);
//
//         try {
//             for (auto& operation : m_operations) {
//                 operation();
//             }
//             transaction.commit();
//             m_executed = true;
//         } catch (...) {
//             // Transaction automatically rolls back on destruction if not committed
//             m_executed = false;
//             throw;
//         }
//     }
//
//     bool is_executed() const { return m_executed; }
// };

} // namespace datahub

#endif // SCRATCHER_DAO_OPERATIONS_HPP
