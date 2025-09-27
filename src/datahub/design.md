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

## Architecture Overview

### Core Design Pattern

Following Boost.ASIO style, the architecture uses:
- **Generic Factory Functions**: `make_data_sink<Entity, Connection, SyncPolicy>(shared_ptr<Connection>)` style API
- **Template-based Components**: Static polymorphism instead of virtual inheritance where appropriate
- **Distributed Configuration**: Each component manages its own state
- **RAII Guarantees**: Successful creation implies full initialization
- **Composable Operations**: Generic utility functions for common patterns

### Core Components

#### 1. DataSink<Entity, Connection, SyncPolicy>
**Purpose**: Self-contained data management unit with DAO and sync capabilities.

**Key Features**:
- Template-based design with static polymorphism
- Automatic DAO creation and table initialization
- Background sync started immediately upon creation according to RAII
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


## Design Review - Addressed Requirements

The design has been updated to address all key requirements:

### ✅ **Sync Strategy** - Customizable Sync Scenarios
**Requirement**: *Sync scenario should be managed as separate customizable object which is passed to DataSink*

**Solution**: Implemented template-based sync policies with the required scenarios:
- `query_on_init_sync` - Load initial data from remote
- `websocket_subscription_sync` - Real-time WebSocket updates  
- `push_after_write_sync` - Async push of local changes to remote
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
**Requirement**: *ByBit data provider should nest DataSink objects*

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
