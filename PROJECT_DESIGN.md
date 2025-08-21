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
cmake -B cmake-build-debug-gcc-14 -G Ninja

# Build the project
cmake --build cmake-build-debug-gcc-14

# Install (optional)
cmake --install cmake-build-debug-gcc-14
```

## Testing
**Framework**: Catch2 (v3.5.4)
**Test Location**: test/ directory with subdirectories matching source structure
**Naming Convention**: test_*.cpp
**Configuration**: Tests configured in CMakeLists.txt using add_test function
**Run Command**:
```bash
# Run a specific test
./cmake-build-debug-gcc-14/test_currency
./cmake-build-debug-gcc-14/test_buoy_candle
./cmake-build-debug-gcc-14/test_instrument
```

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
