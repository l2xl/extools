# DataHub Component Design

## Overview

The DataHub component provides a lightweight data management layer with local DAO storage and callback-based notification system. It follows Boost.ASIO design principles with template-based static polymorphism, RAII guarantees, and minimal runtime overhead.

## Requirements

### Core Requirements
1. **Data Storage**: Integrate with existing DAO layer (`datahub::data_model`)
2. **Reflection**: Use Glaze library for JSON serialization/deserialization
3. **Async Operations**: Integrate with Boost.ASIO executors for async dispatch
4. **Architecture**: Self-contained, composable components with RAII guarantees
5. **Modern C++**: C++23 features (concepts, ranges, fold expressions)
6. **No Global State**: Each component is independently composable

### Design Principles
- **RAII Compliance**: Automatic initialization (cache loading) on creation
- **Static Polymorphism**: Template-based design, zero runtime overhead
- **Callback-based**: User-provided callables for data and error handling
- **Composable**: Building blocks that can be combined flexibly
- **Type Safety**: Compile-time type checking via concepts

## Architecture Overview

### Current Implementation

The datahub provides three main building blocks:

1. **data_sink** - Core data management with DAO integration
2. **data_adapter** - JSON-to-type conversion with callback
3. **data_dispatcher** - Multi-type JSON dispatch to multiple adapters

### Design Pattern

Following Boost.ASIO style:
- **Factory functions**: `make_data_sink<Entity, PrimaryKey>(db, data_cb, error_cb)`
- **Template-based**: Static polymorphism via templates and concepts
- **RAII guarantees**: Object creation implies full initialization (cache loaded)
- **Executor-based**: Works with any `boost::asio::any_io_executor`

## Core Components

### 1. data_sink<Entity, PrimaryKey, DataCallable, ErrorCallable>

**Purpose**: Self-contained data management unit with automatic DAO integration and cache management.

**Template Parameters**:
- `Entity` - The entity type to manage
- `PrimaryKey` - Pointer-to-member for the primary key field
- `DataCallable` - Callback type for data notifications: `void(std::deque<Entity>&&, DataSource)`
- `ErrorCallable` - Callback type for error notifications: `void(std::exception_ptr)`

**Key Features**:
- **Automatic DAO creation**: Creates `datahub::data_model` internally
- **RAII cache loading**: Loads existing data from DB on creation, calls callback with `DataSource::CACHE`
- **Deduplication**: Uses `insert_or_replace` to avoid duplicates
- **Data source tracking**: Distinguishes `CACHE` (from DB) vs `SERVER` (from network)
- **Type-safe acceptors**: `data_acceptor<Range>()` creates lambda for range-based data input
- **Exception safety**: All exceptions forwarded to error callback

**Usage Pattern**:
```cpp
auto sink = make_data_sink<PublicTrade, &PublicTrade::execId>(
    db,
    [](std::deque<PublicTrade>&& trades, DataSource source) {
        // Handle new/cached trades
        // source indicates if from CACHE or SERVER
    },
    [](std::exception_ptr e) {
        // Handle errors
    }
);

// Create acceptor for incoming data
auto acceptor = sink->data_acceptor<std::deque<PublicTrade>>();
acceptor(incoming_trades);  // Stores and notifies
```

**RAII Behavior**:
- On creation: Automatically loads cache from database
- Cache callback: Invoked immediately with `DataSource::CACHE` if data exists
- Server callback: Invoked when new data arrives via acceptor with `DataSource::SERVER`

### 2. data_adapter<DataType, Handler>

**Purpose**: JSON string to typed data conversion with callback invocation.

**Template Parameters**:
- `DataType` - The type to deserialize from JSON
- `Handler` - Callable invoked with deserialized data: `void(DataType&&)`

**Key Features**:
- **Glaze integration**: Uses `glz::read_json<DataType>()` for deserialization
- **Error tolerance**: Returns `false` on parse failure (allows dispatcher fallthrough)
- **Zero overhead**: Template-based, no virtual calls
- **Movable/copyable**: Standard value semantics

**Usage Pattern**:
```cpp
auto adapter = make_data_adapter<ApiResponse<ListResult<Trade>>>(
    [](auto&& response) {
        // Handle deserialized response
        process(response.result.list);
    }
);

bool success = adapter(json_string);  // Deserializes and invokes callback
```

### 3. data_dispatcher<Acceptor...>

**Purpose**: Dispatches JSON strings to multiple typed adapters, trying each in sequence.

**Template Parameters**:
- `Acceptor...` - Variadic list of adapter types to try

**Key Features**:
- **Multi-type dispatch**: Tries each acceptor until one succeeds
- **Async processing**: Uses strand-serialized queue for thread-safe dispatch
- **Lock-free queue**: `boost::lockfree::spsc_queue` for producer-consumer pattern
- **Executor-based**: Accepts `boost::asio::any_io_executor` for async work
- **Copyable**: Multiple dispatchers can share the same acceptors

**Usage Pattern**:
```cpp
auto dispatcher = make_data_dispatcher(
    io_ctx.get_executor(),
    make_data_adapter<TypeA>([](auto&& a) { /* handle A */ }),
    make_data_adapter<TypeB>([](auto&& b) { /* handle B */ }),
    make_data_adapter<TypeC>([](auto&& c) { /* handle C */ })
);

dispatcher(json_string);  // Tries A, then B, then C until one succeeds
```

**Thread Safety**:
- Uses `boost::asio::strand` for serialized execution
- Lock-free queue between producer and consumer
- Safe to call `operator()` from multiple threads

## Data Flow Patterns

### Pattern 1: WebSocket → Adapter → Sink
```cpp
// 1. Create sink with callbacks
auto trade_sink = make_data_sink<PublicTrade, &PublicTrade::execId>(
    db, data_callback, error_callback
);

// 2. Create acceptor from sink
auto trade_acceptor = trade_sink->data_acceptor<std::deque<PublicTrade>>();

// 3. Create adapter that feeds sink
auto ws_adapter = make_data_adapter<WsPayload<PublicTrade>>(
    [=](auto&& payload) mutable {
        trade_acceptor(std::move(payload.data));
    }
);

// 4. Create dispatcher for WebSocket messages
auto ws_dispatcher = make_data_dispatcher(executor, ws_adapter);

// 5. Feed WebSocket messages
websocket->on_message([=](std::string msg) {
    ws_dispatcher(std::move(msg));
});
```

### Pattern 2: HTTP → Adapter → Sink
```cpp
// HTTP response adapter feeding same sink
auto http_adapter = make_data_adapter<ApiResponse<ListResult<PublicTrade>>>(
    [=](auto&& resp) mutable {
        trade_acceptor(std::move(resp.result.list));
    }
);

auto http_dispatcher = make_data_dispatcher(executor, http_adapter);

// Feed HTTP responses
http_query->execute([=](std::string response) {
    http_dispatcher(std::move(response));
});
```

### Pattern 3: Multi-Entity Dispatcher
```cpp
// Single dispatcher handling multiple entity types
auto multi_dispatcher = make_data_dispatcher(
    executor,
    make_data_adapter<WsPayload<PublicTrade>>(
        [=](auto&& payload) { trade_acceptor(std::move(payload.data)); }
    ),
    make_data_adapter<ApiResponse<ListResult<InstrumentInfo>>>(
        [=](auto&& resp) { instrument_acceptor(std::move(resp.result.list)); }
    ),
    make_data_adapter<WsPayload<OrderBook>>(
        [=](auto&& payload) { orderbook_acceptor(std::move(payload.data)); }
    )
);
```

## Key Design Decisions

### ✅ Callback-Based Instead of Sync Policies

**Rationale**: Callbacks provide maximum flexibility without template complexity.

**Benefits**:
- Users control exactly what happens with data
- No need for predefined sync policy types
- Can easily compose multiple operations in callback
- Direct, explicit data flow

**Trade-off**: Users must implement their own sync logic, but gain full control.

### ✅ Executor Parameter Instead of Scheduler Dependency

**Rationale**: Accept `boost::asio::any_io_executor` following Boost.ASIO best practices.

**Benefits**:
- No coupling to scheduler implementation
- Works with any executor type
- Lightweight, copyable handle
- Follows Boost.ASIO idioms

**Implementation**: `data_dispatcher` accepts executor by value in constructor.

### ✅ data_acceptor() Pattern Instead of Connection Template

**Rationale**: Provide lambda-based acceptors instead of connection type parameter.

**Benefits**:
- Connection-agnostic: works with WebSocket, HTTP, message queues, etc.
- Flexible composition: can chain multiple acceptors
- Type-safe via concepts: `std::ranges::input_range` with convertible entities
- No coupling to connection implementations

**Pattern**:
```cpp
auto acceptor = sink->data_acceptor<std::deque<Entity>>();
// acceptor is callable: void(std::deque<Entity>&&)
```

### ✅ DataSource Enum for Cache/Server Distinction

**Rationale**: Notify users whether data is from cache (DB) or fresh from server.

**Benefits**:
- Users can handle initial cache load differently (e.g., build UI state)
- Users can handle server updates differently (e.g., animations, notifications)
- Simple, explicit, type-safe

**Values**:
- `DataSource::CACHE` - Data loaded from database on sink creation
- `DataSource::SERVER` - New data received via acceptor

### ✅ No In-Memory Cache (Yet)

**Rationale**: Keep implementation simple; users can add caching in callbacks if needed.

**Future**: Could add optional `std::optional<in_memory_cache<Entity>>` member.

## Implementation Status

### ✅ Implemented
- `data_sink<Entity, PrimaryKey, DataCallable, ErrorCallable>` with RAII cache loading
- `data_adapter<DataType, Handler>` for JSON deserialization
- `data_dispatcher<Acceptor...>` for multi-type dispatch
- `DataSource` enum for cache/server distinction
- Factory functions (`make_data_sink`, `make_data_adapter`, `make_data_dispatcher`)
- Automatic DAO creation and table initialization
- Deduplication via `insert_or_replace`
- Exception safety with error callbacks
- Executor-based async dispatch
- Concepts for type safety (`std::ranges::input_range`, `std::convertible_to`)

### ❌ Not Implemented (Design Document Features)
- Sync policy templates (e.g., `query_on_init_sync`, `websocket_subscription_sync`)
- Connection template parameter
- `set_error_handler()` method (error handler is constructor parameter)
- In-memory cache template parameter
- Automatic reconnection (handled externally by connection layer)

### 🔄 Different Implementation
- **Sync strategies**: Implemented via user callbacks, not template policies
- **Connection abstraction**: Uses `data_acceptor()` pattern, not connection template
- **Error handling**: Constructor parameter, not setter method
- **Cache management**: Automatic on creation, no explicit cache API

## Integration Examples

### ByBit Data Manager Integration
```cpp
class ByBitDataManager {
    std::shared_ptr<data_sink<PublicTrade, &PublicTrade::execId>> m_trade_sink;
    std::shared_ptr<data_sink<InstrumentInfo, &InstrumentInfo::symbol>> m_instrument_sink;

    ByBitDataManager(io_context& io, std::shared_ptr<SQLite::Database> db) {
        // Create trade sink
        m_trade_sink = make_data_sink<PublicTrade, &PublicTrade::execId>(
            db,
            [this](auto&& trades, DataSource src) { on_trades(std::move(trades), src); },
            [this](auto e) { on_error(e); }
        );

        // Create instrument sink
        m_instrument_sink = make_data_sink<InstrumentInfo, &InstrumentInfo::symbol>(
            db,
            [this](auto&& instruments, DataSource src) { on_instruments(std::move(instruments), src); },
            [this](auto e) { on_error(e); }
        );

        // Set up WebSocket → Sink pipeline
        setup_websocket_pipeline(io);
    }
};
```

## Future Enhancements

### Potential Additions
1. **Optional in-memory cache**: `std::optional<flat_map<KeyType, Entity>>`
2. **Query interface**: `sink->query_by_key(key)` or `sink->query_range(start, end)`
3. **Batch operations**: Optimize bulk inserts
4. **Metrics**: Track cache hit rate, data throughput
5. **Compression**: Optional transparent compression in DAO layer

### Backward Compatibility
All enhancements should maintain:
- Existing factory function signatures
- RAII guarantees
- Callback-based notification pattern
- Zero overhead when features not used

## Summary

The datahub implementation provides a pragmatic, lightweight foundation:

**Strengths**:
- ✅ Simple, composable building blocks
- ✅ RAII guarantees with automatic cache loading
- ✅ Type-safe via concepts
- ✅ Flexible callback-based design
- ✅ Minimal dependencies (only DAO, Glaze, Boost.ASIO)
- ✅ Zero overhead abstractions

**Design Philosophy**:
- Provide mechanisms, not policies
- Composition over configuration
- Explicit over implicit
- Type safety over runtime flexibility

**Typical Usage**:
```cpp
// 1. Create sink (loads cache automatically)
auto sink = make_data_sink<Entity, &Entity::id>(db, data_cb, error_cb);

// 2. Create acceptor
auto acceptor = sink->data_acceptor<std::deque<Entity>>();

// 3. Create adapter + dispatcher
auto dispatcher = make_data_dispatcher(
    executor,
    make_data_adapter<ResponseType>([=](auto&& resp) {
        acceptor(std::move(resp.data));
    })
);

// 4. Feed data
websocket->on_message([=](auto msg) { dispatcher(msg); });
```

Ready for production use with clear paths for future enhancements.