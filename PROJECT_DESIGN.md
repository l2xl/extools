# Project Design & Development Context

## Architecture Overview
- Qt-based modular GUI application
- Trading client with plugin architecture
- Core modules: app, qml, core, data, common, widget
- Layered architecture with strict dependency management
  - Include paths are managed by CMakeLists.txt to enforce proper layer dependencies
  - Includes with arbitrary paths in project are forbidden
  - Never use paths that explicitly refer to up-folder (/..)
  - Relative paths are allowed only in case it does not name an architecture layer

## Current Session Changes
*This section tracks changes made in current development session*

### Connect Component Implementation - Phase 1 Complete
**Changes Made:**
1. **Generic Connection Concept**: Redesigned connection interface to be completely generic
   - `setup()` method receives generic data handler and variable arguments
   - `cancel()` method for operation/subscription cancellation
   - `is_active()` method for connection state checking
   - Connection implementations interpret arguments independently

2. **ConnectionContext Class**: Shared connection infrastructure
   - Holds AsioScheduler reference for async operations
   - Manages host resolution and caching (DNS resolution on creation)
   - Stores connection parameters (host, port, base_path, timeout)
   - Provides `wait_for_resolution()` for async DNS completion

3. **JsonRpcConnection Implementation**: Single-time HTTP JSON-RPC requests
   - Created per request by data provider
   - Setup arguments: `handler, request_path, [url_params]`
   - Handler signature: `std::function<void(std::string)>` (receives JSON response)
   - SSL/HTTPS support with proper SNI handling
   - Cancellation support with atomic flags

4. **WebSocketConnection Implementation**: Persistent WebSocket subscriptions
   - Shared between multiple DataSinks using factory pattern (`WebSocketConnection::create()`)
   - Setup arguments: `handler, subscription_id, subscription_message`
   - Handler signature: `std::function<void(const std::string&)>` (receives JSON messages)
   - Connection opens after first setup() call
   - Message distribution to all registered handlers
   - Heartbeat/ping support for connection maintenance
   - Subscription management with removal support
   - Private constructor with `EnsurePrivate` parameter prevents external instantiation

**Technical Implementation:**
- Generic setup() method using variadic templates and perfect forwarding
- SSL/TLS support for both HTTP and WebSocket connections
- Async DNS resolution with caching in ConnectionContext
- Thread-safe subscription management for WebSocket connections
- Proper error handling and cancellation support
- Integration with existing AsioScheduler and SSL context
- Factory pattern for `std::enable_shared_from_this` classes with private constructors
- `EnsurePrivate` parameter prevents external instantiation of shared objects

**Directory Structure:**
```
src/connect/
├── connect.hpp                    # Main header with usage documentation
├── connection_concept.hpp         # Generic Connection concept definition
├── connection_context.hpp/.cpp    # Shared connection infrastructure
├── json_rpc_connection.hpp/.cpp   # Single-time HTTP requests
└── websocket_connection.hpp/.cpp  # Persistent WebSocket subscriptions
```

### DAO Layer Implementation - Phase 1 Complete
**Changes Made:**
1. **RAII Constructor Implementation**: Updated `Dao<Entity>` template class to follow RAII principles
   - Primary constructor now automatically creates metadata from Entity type using Glaze reflection
   - Legacy constructor maintained for backward compatibility
   - Automatic null-check validation in constructors

2. **Glaze Reflection-based Metadata Creation**: Implemented `create_entity_metadata<Entity>()` function
   - Uses Glaze library reflection capabilities (`glz::reflect<T>`, `glz::member_names<T>`, `glz::to_tie()`)
   - Automatic table name generation from entity type name (lowercase, :: replaced with _)
   - Automatic column definitions based on C++ type introspection
   - Support for basic types: string (TEXT), integral (INTEGER), floating-point (REAL), bool (INTEGER)
   - Support for std::optional types with proper null handling
   - First field automatically designated as PRIMARY KEY (ID field assumption)
   - Automatic value extraction and entity creation functions using Glaze reflection

3. **Helper Functions**: Implemented comprehensive type conversion utilities using Glaze
   - `extract_entity_values()` / `create_entity_from_values()`: Direct member access using `glz::to_tie()`
   - `convert_to_query_value()` / `convert_query_value()`: QueryValue type conversions with optional support
   - `convert_to_string()` / `convert_from_string()`: Type-safe string conversions
   - `get_first_field_as_string()` / `set_first_field_from_string()`: ID field access helpers
   - `get_field_as_string()` / `set_field_from_string()`: Named field access using `glz::member_names<T>`

4. **Interface Alignment**: Updated method names in dao.cpp to match IDao interface
   - `find_by_id()` → `query_by_id()`
   - `find_all()` → `query_all()`
   - `find_where()` → `query()`

**Technical Implementation:**
- Zero-configuration approach: No manual metadata definition required
- Compile-time reflection using Glaze library (already included in project)
- Type-safe automatic schema generation with proper field name extraction
- RAII compliance with automatic resource management
- Leverages existing Glaze dependency for consistent reflection approach
- Supports up to 128 fields per entity (Glaze's max_pure_reflection_count)
- Proper handling of std::optional types with null/empty value mapping

## Design Decisions & Patterns

### DAO Layer Architecture
**Purpose**: Generic Data Access Object layer providing type-safe database operations with automatic schema generation.

**Key Components**:
- **IDao<Entity>**: Generic interface for CRUD operations
- **GenericDao<Entity>**: Template-based DAO implementation using C++23 reflection
- **EntityMetadata**: Automatic schema introspection from entity structure using C++23 reflection
- **QueryBuilder & QueryCondition**: Type-safe query construction
- **Transaction**: RAII-based transaction management with automatic rollback
- **DaoFactory**: DAO registry and factory with caching

**Design Principles**:
- **Zero Configuration**: Automatic table creation and schema generation using C++23 reflection
- **Type Safety**: Compile-time type checking for all database operations
- **RAII Compliance**: Automatic resource management for transactions and connections
- **Generic & Extensible**: Works with any entity type through template specialization
- **Lazy Initialization**: Tables created on-demand when first accessed
- **Performance**: Batch operations, prepared statements, and connection pooling
- **Thread Safety**: Database-specific concurrency model with strand-based serialization for SQLite

**Directory Structure**:
```
src/data/dao/
├── dao_interface.hpp          # Core DAO interface
├── generic_dao.hpp           # Generic DAO implementation
├── generic_dao.cpp
├── transaction.hpp           # RAII transaction management
├── transaction.cpp
├── entity_metadata.hpp       # Entity introspection using C++23 reflection
├── entity_metadata.cpp
├── query_builder.hpp         # Query building utilities
├── query_builder.cpp
├── dao_factory.hpp          # DAO factory and registry
└── dao_factory.cpp
```

**Integration**: Extends existing DbStorage class without breaking changes, leveraging existing SQLiteCpp dependency.

**Thread Safety Model**:
- **SQLite**: All database operations serialized through existing DbStorage strand (boost::asio::strand)
- **Connection Management**: Single connection per DbStorage instance with strand-based access control
- **Transaction Isolation**: Transactions bound to the database strand to prevent concurrent access
- **DAO Thread Safety**: All DAO operations are async and executed on the database strand
- **Future Database Support**: Extensible design allows different concurrency models for other databases (PostgreSQL, MySQL)

**Usage Pattern**:
```cpp
auto dao_factory = db_storage->dao_factory();
auto kline_dao = dao_factory.get_dao<bybit::KlineData>();

// Simple operations
co_await kline_dao->insert(kline_data);
auto klines = co_await kline_dao->find_all();

// Transactional operations
co_await dao_factory.with_transaction([&](auto& tx) -> boost::asio::awaitable<void> {
    co_await kline_dao->insert_batch(kline_batch);
    co_await instrument_dao->update(instrument);
    tx.commit();
});
```

## Implementation Guidelines
- Follow extremal programming paradigm (minimal required implementation)
- Use CMake with Ninja for builds
- Leverage existing build cache in cmake-build-debug-gcc-14/

### Coroutine Safety Pattern
**Mandatory Pattern for Async Methods**: All coroutine methods must follow the static method pattern with shared_ptr parameters to ensure object lifetime safety.

**Pattern Requirements**:
```cpp
class MyClass : public std::enable_shared_from_this<MyClass> {
public:
    // ✅ CORRECT: Static coroutine with shared_ptr parameter
    static boost::asio::awaitable<ResultType> co_async_operation(
        std::shared_ptr<MyClass> self,
        ParameterType param);

    // ❌ WRONG: Non-static coroutine - object can be destroyed during suspension
    boost::asio::awaitable<ResultType> co_async_operation(ParameterType param);
};
```

**Rationale**:
- **Object Lifetime Safety**: The `shared_ptr` parameter ensures the object remains alive for the entire coroutine duration
- **No `this` Pointer Risks**: Eliminates risk of accessing destroyed object members through `this` during coroutine suspension points
- **Clear Ownership Semantics**: The coroutine explicitly holds a reference to the required object
- **Thread Safety**: Works seamlessly with strand-based thread safety patterns

**Usage Pattern**:
```cpp
// In the calling code:
co_await MyClass::co_async_operation(shared_from_this(), parameters);

// In the coroutine implementation:
static boost::asio::awaitable<ResultType> MyClass::co_async_operation(
    std::shared_ptr<MyClass> self, ParameterType param)
{
    // Optional: Enforce strand execution for thread safety
    co_await boost::asio::post(self->m_strand, boost::asio::use_awaitable);

    // Safe to access self-> members - object lifetime guaranteed
    co_await some_async_operation();
    self->member_variable = result;
    co_return result;
}
```

**When to Use**:
- All coroutine methods that access class members
- Any async operation that might outlive the object's normal scope
- Methods that require strand-based thread safety

**Examples in Codebase**:
- `context::co_resolve()` in `src/connect/connection_context.cpp`
- `http_query::co_request()` in `src/connect/http_query.cpp`

## Development Workflow

### DAO Layer Implementation Plan
**Phase 1: Core Infrastructure**
1. Create `src/data/dao/` directory structure
2. Implement `dao_interface.hpp` - Core DAO interface template
3. Implement `entity_metadata.hpp` - C++23 reflection for automatic schema generation
4. Implement `query_builder.hpp` - Type-safe query construction

**Phase 2: DAO Implementation**
5. Implement `transaction.hpp` - RAII transaction management
6. Implement `generic_dao.hpp` - Template-based DAO with automatic schema generation
7. Implement `dao_factory.hpp` - DAO registry and caching

**Phase 3: Integration**
8. Extend `DbStorage` class to include DAO factory
9. Add DAO factory access methods to existing DbStorage interface
10. Update CMakeLists.txt to include new DAO source files

**Phase 4: Testing & Validation**
11. Create unit tests for DAO components using existing Catch2 framework
12. Test with existing ByBit entities (InstrumentInfo, PublicTrade)
13. Performance testing and optimization

**Implementation Notes**:
- Each phase should be implemented and tested independently
- Maintain backward compatibility with existing DbStorage usage
- Use C++23 reflection to automatically introspect ByBit entity structures
- Follow project's layered architecture constraints
- All DAO operations must be async and execute on DbStorage strand for SQLite thread safety
- Transaction objects must be bound to the database strand to prevent concurrent access

## Technical Debt & Future Considerations
*Known compromises and planned improvements*

# Exchange Scratcher Information

## Summary
Exchange Scratcher is a general-purpose standalone exchange client created by engineers for engineers. It provides a highly modular and flexible GUI built with Qt, offering a clean trading widget with undistorted trading views, universal trading API for automated strategies, a testing engine for historical data, and an extensible plugin system for connecting to different exchanges (primarily crypto-exchanges).

## Structure
- **src/**: Main source code directory
  - **app/**: Application-level components including the main window
  - **qml/**: QML UI components
  - **core/**: Core functionality including scheduler
  - **data/**: Data providers and storage
  - **common/**: Common utilities and data structures
  - **widget/**: UI widgets for trading interface
- **test/**: Test files organized by module
- **cmake/**: CMake modules and utilities
- **contrib/**: Third-party libraries
- **db/**: Database-related files

## Language & Runtime
**Language**: C++
**Version**: C++23
**Build System**: CMake (3.21+)
**Package Manager**: CPM (CMake Package Manager)

## Dependencies
**Main Dependencies**:
- Qt6 (Core, Widgets, Svg)
- Boost 1.83+ (context, system)
- OpenSSL
- oneTBB (v2022.1.0)
- SQLiteCpp (3.3.3)
- Glaze (v5.5.4)

**Development Dependencies**:
- Catch2 (3.5.4) for testing

## Build & Installation
```bash
# Configure with CMake using Ninja
cmake -B cmake-build-debug-clang-19 -G Ninja

# Build the project
cmake --build cmake-build-debug-clang-19

# Install (optional)
cmake --install cmake-build-debug-clang-19
```

## Testing
**Framework**: Catch2 (v3.5.4)
**Test Location**: test/ directory with subdirectories matching source structure
**Naming Convention**: test_*.cpp
**Configuration**: Tests configured in CMakeLists.txt using add_test function

## Application Structure
**Entry Point**: src/main.cpp
**Main Components**:
- **Config**: Application configuration
- **AsioScheduler**: Handles asynchronous operations
- **TradeCockpitWindow**: Main application window
- **ViewController** executes coordination in terms View/Controller pattern
  - There may by diferent view controllers derive it
- **IDataController** subclasses manages data retrieval and caching. It also manages sets of IDataProviders used by th view controllers to retrieve data 
- **ByBitApi**: API client for ByBit exchange

## Layers Organization
- **app**: Contains the main application window and controllers
- **core**: Core functionality including scheduler
- **data**: Data providers, storage, and exchange-specific implementations
- **common**: Common utilities and data structures
- **widget**: UI components for trading interface
