# DataHub Component Design

## Overview

The DataHub component provides comprehensive data model management with local DAO layer storage and remote backend synchronization capabilities. It follows Boost.ASIO design principles using static polymorphism, generic utility functions, and distributed configuration over uniform generic classes. No global singletons or classic OOP hierarchies.

## Requirements

### Core Requirements
1. **Data Storage**: Utilize current DAO layer implementation with potential improvements
2. **Reflection**: Use Glaze library reflection (avoid requiring glz::meta helpers by default)
3. **Async Operations**: Use AsioScheduler with Boost Beast/ASIO libraries and C++ coroutines
4. **Architecture**: Fully independent, functional per-model classes with RAII concept
5. **Modern C++**: Target latest C++23 features available with clang-19
6. **No Global Singletons**: Each component should be self-contained and composable

### Design Principles
- **RAII Compliance**: Automatic resource management and cleanup
- **Static Polymorphism**: Template-based design following Boost.ASIO patterns
- **Generic Functions**: Utility functions for common operations
- **Distributed State**: Configuration and state managed by individual components
- **Functional Style**: Avoid classic OOP hierarchies, prefer composition
- **Type Safety**: Compile-time type checking using Glaze reflection

## Architecture Overview

### Core Design Pattern

Following Boost.ASIO style, the architecture uses:
- **Generic Factory Functions**: `make_data_hub<Entity>()` style API
- **Template-based Components**: Static polymorphism instead of virtual inheritance where appropriate
- **Distributed Configuration**: Each component manages its own state
- **RAII Guarantees**: Successful creation implies full initialization
- **Composable Operations**: Generic utility functions for common patterns

### Core Components

#### 1. DataHub<Entity, Connection, SyncPolicy>
**Purpose**: Self-contained data management unit with DAO and sync capabilities.

**Key Features**:
- Template-based design with static polymorphism
- Automatic DAO creation and table initialization
- Background sync started immediately upon creation
- Connection-agnostic through template parameters
- Policy-based sync behavior configuration

#### 2. Connection Concepts
**Purpose**: Template concepts defining connection behavior (REST, WebSocket, etc.).

**Key Features**:
- Static polymorphism through concepts and templates
- Protocol-specific implementations (HttpConnection, WebSocketConnection)
- Uniform interface through concept requirements

#### 3. Sync Policy Templates
**Purpose**: Template policies defining synchronization behavior.

**Key Features**:
- Compile-time policy selection
- Configurable through template parameters
- Strategy patterns implemented as template specializations
- No runtime overhead from virtual calls

#### 4. Generic Utility Functions
**Purpose**: Free functions for common operations across different entity types.

**Key Features**:
- Template-based generic operations
- Type deduction and concept constraints
- Composable building blocks for complex operations

## Draft API Interface

### Main Factory Function

```cpp
namespace scratcher::datahub {

// Primary API - creates fully initialized DataSink with RAII guarantees
template<typename Entity, auto PrimaryKey, typename Connection, typename SyncPolicy = bidirectional_sync>
auto make_data_sink(...) -> std::shared_ptr<DataSink<Entity, PrimaryKey, Connection, SyncPolicy>>;

}
```

### DataSink Template Class

```cpp
template<typename Entity, auto PrimaryKey, typename Connection, typename SyncPolicy>
class DataSink
{
public:
    using entity_type = Entity;
    using dao_type = dao::Dao<Entity, PrimaryKey>;
    using connection_type = Connection;
    using sync_policy_type = SyncPolicy;

    // DAO access - primary interface for data operations
    std::shared_ptr<dao_type> dao();
    std::shared_ptr<const dao_type> dao() const;

    // Data subscription interface
    enum class SourceType { Local, Remote };
    template<typename Handler>
    void subscribe(Handler&& handler);
    
    // Error handling
    void set_error_handler(ErrorHandler&& handler);
    
    // Connection management
    enum class SyncStatus { OutOfSync, RemoteDataReceived, LocalDataPushed, FullSync };
    enum class SyncScheduleStatus { NotScheduled, Scheduled };
    void force_sync();
    void schedule_sync()
    
    SyncStatus SyncStatus() const;
    SyncScheduleStatus() const;
};
```

### Connection Concepts and Implementations

```cpp
// Connection concept requirements
template<typename T>
concept Connection = requires(T conn) {
    { conn.connect(...) } -> std::same_as<boost::asio::awaitable<void>>;
    { conn.disconnect() } -> std::same_as<boost::asio::awaitable<void>>;
    { conn.is_connected() } -> std::same_as<bool>;
    
    // Generic CRUD operations
    { conn.template fetch<Entity>() } -> std::same_as<boost::asio::awaitable<std::vector<Entity>>>;
    { conn.template create<Entity>(...) } -> std::same_as<boost::asio::awaitable<Entity>>;
    { conn.template update<Entity>(...) } -> std::same_as<boost::asio::awaitable<Entity>>;
    { conn.template remove<Entity>(...) } -> std::same_as<boost::asio::awaitable<void>>;
};

// HTTP REST connection implementation
struct http_connection {
    std::string base_url;
    std::string auth_token;
    std::chrono::milliseconds timeout;

    boost::asio::awaitable<void> connect(...);
    boost::asio::awaitable<void> disconnect();
    bool is_connected() const;

    template<typename Entity>
    boost::asio::awaitable<std::vector<Entity>> fetch();
    
    template<typename Entity>
    boost::asio::awaitable<Entity> create(...);
    
    template<typename Entity>
    boost::asio::awaitable<Entity> update(...);
    
    template<typename Entity>
    boost::asio::awaitable<void> remove(...);
};

// WebSocket subscription connection implementation
struct websocket_connection {
    std::string ws_url;
    std::string subscription_topic;
    std::function<void(const std::string&)> message_handler;

    boost::asio::awaitable<void> connect(...);
    boost::asio::awaitable<void> disconnect();
    bool is_connected() const;
    
    boost::asio::awaitable<void> subscribe();
    boost::asio::awaitable<void> unsubscribe();

    template<typename Entity>
    boost::asio::awaitable<std::vector<Entity>> fetch();
    
    // Other CRUD operations adapted for WebSocket context
};
```

### Sync Policy Templates

Sync scenarios are managed as separate customizable objects. Here are the basic predefined policies:

```cpp
// Sync policy concept - customizable sync scenarios
template<typename T>
concept SyncPolicy = requires(T policy) {
    // Core sync behavior
    { policy.sync_interval() } -> std::same_as<std::chrono::milliseconds>;
    { policy.should_sync_on_startup() } -> std::same_as<bool>;
    
    // Async sync operations - policy controls the sync strategy
    { policy.template perform_sync<Entity>(...) } -> std::same_as<boost::asio::awaitable<void>>;
};

// 1. Query on init - load initial data from remote
struct query_on_init_sync {
    bool query_on_startup = true;
    
    std::chrono::milliseconds sync_interval() const;
    bool should_sync_on_startup() const;
    
    template<typename Entity, typename Dao, typename Connection>
    boost::asio::awaitable<void> perform_sync(...);
};

// 2. WebSocket subscription - real-time updates
struct websocket_subscription_sync {
    bool subscribe_on_init = true;
    
    std::chrono::milliseconds sync_interval() const;
    bool should_sync_on_startup() const;
    
    template<typename Entity, typename Dao, typename Connection>
    boost::asio::awaitable<void> perform_sync(...);
};

// 3. Push after write - sync local changes to remote (async)
struct push_after_write_sync {
    std::chrono::milliseconds push_delay;
    
    std::chrono::milliseconds sync_interval() const;
    bool should_sync_on_startup() const;
    
    template<typename Entity, typename Dao, typename Connection>
    boost::asio::awaitable<void> perform_sync(...);
};

// 4. Periodic sync by REST/JSON-RPC
struct periodic_rest_sync {
    std::chrono::milliseconds interval;
    bool query_on_init = true;
    
    std::chrono::milliseconds sync_interval() const;
    bool should_sync_on_startup() const;
    
    template<typename Entity, typename Dao, typename Connection>
    boost::asio::awaitable<void> perform_sync(...);
};

// 5. Bidirectional sync - combination of push and pull
struct bidirectional_sync {
    std::chrono::milliseconds interval;
    
    std::chrono::milliseconds sync_interval() const;
    bool should_sync_on_startup() const;
    
    template<typename Entity, typename Dao, typename Connection>
    boost::asio::awaitable<void> perform_sync(...);
};
```

### In-Memory Cache Support

DataSink can optionally include in-memory caching:

```cpp
// Optional cache policy template
template<typename Entity>
struct in_memory_cache {
    std::unordered_map<auto, Entity> cache;
    std::chrono::milliseconds cache_ttl;
    
    std::optional<Entity> get(...) const;
    void put(...);
    void invalidate(...);
    void clear();
};

// DataSink with cache support
template<typename Entity, auto PrimaryKey, typename Connection, typename SyncPolicy, typename Cache = no_cache>
class DataSink { /* ... */ };
```

## Usage Examples

### Basic Usage with HTTP REST API

```cpp
#include "datahub/datahub.hpp"
#include "data/bybit/entities/instrument.hpp"

// Create DataSink for ByBit instrument data with periodic REST sync
auto instrument_sink = scratcher::datahub::create_data_sink<
    bybit::InstrumentInfo, 
    &bybit::InstrumentInfo::symbol
>(
    io_context,
    database,
    scratcher::datahub::http_connection{...},
    scratcher::datahub::periodic_rest_sync{...}
);

// Access DAO for data operations
auto dao = instrument_sink->dao();
dao->insert(instrument_data);
auto instruments = dao->query();

// Subscribe to data changes
instrument_sink->subscribe([](const auto& instrument, DataSink::SourceType type) {
    std::cout << "Instrument " << instrument.symbol << " changed by: " << static_cast<int>(type) << std::endl;
});

// Set error handler for automatic reconnection
instrument_sink->set_error_handler([&instrument_sink](const std::exception& e) {
    std::cerr << "Sync error: " << e.what() << std::endl;
    co_spawn(instrument_sink->reconnect(), detached);
});
```

### WebSocket Subscription Example

```cpp
// Create DataSink for real-time trade data via WebSocket
auto trade_sink = scratcher::datahub::create_data_sink<
    bybit::PublicTrade,
    &bybit::PublicTrade::trade_id
>(
    io_context,
    database,
    scratcher::datahub::websocket_connection{...},
    scratcher::datahub::websocket_subscription_sync{}
);

// Set error handler with automatic WebSocket reset
trade_sink->set_error_handler([&trade_sink](const std::exception& e) {
    std::cerr << "WebSocket error: " << e.what() << std::endl;
    co_spawn(trade_sink->reconnect(), detached);
});
```

### ByBit Data Provider Integration

ByBit data provider integrating DataSink through composition:

```cpp
// ByBit data provider integrating DataSink
class ByBitInstrumentProvider {
private:
    using InstrumentSink = DataSink<bybit::InstrumentInfo, &bybit::InstrumentInfo::symbol, 
                                   http_connection, periodic_rest_sync>;
    std::shared_ptr<InstrumentSink> m_data_sink;

public:
    ByBitInstrumentProvider(...) {
        m_data_sink = create_data_sink<bybit::InstrumentInfo, &bybit::InstrumentInfo::symbol>(...);
        
        // Set up provider-specific event handling
        m_data_sink->subscribe([this](const auto& instrument, auto type) {
            on_instrument_update(instrument, type);
        });
    }
    
    // Provider-specific interface
    boost::asio::awaitable<std::vector<bybit::InstrumentInfo>> get_active_instruments() {
        auto dao = m_data_sink->dao();
        co_return co_await dao->query(...);
    }
    
    // Direct access to underlying DataSink if needed
    std::shared_ptr<InstrumentSink> data_sink() { return m_data_sink; }
    
private:
    void on_instrument_update(const bybit::InstrumentInfo& instrument, DataSink::SourceType type) {
        // Provider-specific logic
    }
};
```

### In-Memory Cache Usage

```cpp
// DataSink with in-memory cache for performance
auto cached_sink = scratcher::datahub::create_data_sink<
    bybit::InstrumentInfo, 
    &bybit::InstrumentInfo::symbol,
    http_connection,
    periodic_rest_sync,
    in_memory_cache<bybit::InstrumentInfo>
>(...);
```


## Design Review - Addressed Requirements

The design has been updated to address all key requirements:

### ✅ **Sync Strategy** - Customizable Sync Scenarios
**Requirement**: *Sync scenario should be managed as separate customizable object which is passed to DataSink*

**Solution**: Implemented template-based sync policies with the required scenarios:
- `query_on_init_sync` - Load initial data from remote
- `websocket_subscription_sync` - Real-time WebSocket updates  
- `push_after_write_sync` - Async push of local changes to remote
- `periodic_rest_sync` - Periodic sync by REST/JSON-RPC
- `bidirectional_sync` - Combination of push and pull

Each policy is a separate customizable object with full control over sync behavior.

### ✅ **Error Handling** - Automatic Recovery
**Requirement**: *API should allow to set error handlers and have functionality to automatically reset network subscriptions (for WebSocket)*

**Solution**: 
- `set_error_handler()` method for custom error handling
- Automatic WebSocket reconnection and subscription reset
- Built-in connection recovery mechanisms
- Error handlers can trigger manual reconnection via `reconnect()` method

### ✅ **Performance** - In-Memory Caching
**Requirement**: *Should allow to have in-memory cache (possibly as in-memory table aside of main DAO object)*

**Solution**:
- Optional `in_memory_cache<Entity>` template parameter
- Cache sits alongside DAO object, not replacing it
- Configurable TTL and cache management
- Template-based design with zero overhead when not used

### ✅ **Integration** - ByBit Data Provider Integration
**Requirement**: *ByBit data provider should derive or nest DataSink*

**Solution**:
- Demonstrated `ByBitInstrumentProvider` class that nests DataSink
- Provider-specific interface while leveraging DataSink capabilities
- Direct access to underlying DataSink when needed
- Clean separation between provider logic and sync management

### ✅ **Architecture** - Boost.ASIO Style
**Requirements**: 
- No global singletons or classic OOP hierarchies
- Static polymorphism with templates
- Generic utility functions
- Distributed configuration

**Solution**:
- `create_data_sink<Entity>()` factory function following Boost.ASIO patterns
- Template-based static polymorphism throughout
- Concept-based connection and sync policy interfaces
- Each DataSink manages its own state independently
- RAII guarantees with automatic initialization

## Next Steps

The design now addresses all your requirements:

1. **Customizable sync scenarios** as separate template objects
2. **Error handling** with automatic WebSocket reconnection
3. **In-memory caching** as optional template parameter
4. **ByBit integration** through composition/nesting pattern
5. **Boost.ASIO style** architecture with static polymorphism

The API provides the right balance of:
- **Simplicity**: `create_data_sink<Entity>()` for basic usage
- **Flexibility**: Customizable connections, sync policies, and caching
- **Performance**: Zero-overhead abstractions and optional optimizations
- **Reliability**: Built-in error recovery and connection management

Ready for implementation or further refinement based on additional feedback.
