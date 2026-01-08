# IBM IMS (Information Management System) Emulation - Improvements
Version 3.6.3

This document details the improvements and new features in version 3.6.0,
which includes all features from versions 3.1.0 through 3.5.0,
plus new components introduced in v3.6.3.

---

## Table of Contents

1. [Overview](#overview)
2. [Version 3.6.3 New Components](#version-360-new-components)
3. [Version 3.5.0 Components](#version-350-components)
4. [Previous Version Components](#previous-version-components)
5. [Build System Improvements](#build-system-improvements)
6. [Documentation Improvements](#documentation-improvements)
7. [Usage Examples](#usage-examples)

---

## Overview

Version 3.6.3 introduces significant new functionality while maintaining full
backward compatibility with previous versions. The primary focus areas are:

### From Previous Versions
- **Data Validation**: Mainframe-compatible input validation
- **Diagnostics**: Enhanced debugging and troubleshooting tools
- **Performance**: Caching, retry mechanisms, and benchmarking
- **String Handling**: Mainframe-style string operations
- **Time Management**: IBM-compatible timestamp formats
- **Storage Abstraction**: Cross-platform persistent storage

### New in Version 3.6.3
- **Event Bus**: Type-safe publish-subscribe pattern
- **Test Coverage**: Comprehensive tests for new modules
- **Bug Fixes**: Duplicate enum naming, version references

### From Version 3.5.0
- **Configuration Persistence**: JSON-based configuration with validation
- **Connection Pooling**: Generic connection pool with health checks
- **Metrics Collection**: Real-time performance monitoring
- **Command Processing**: Interactive CLI framework
- **State Machine**: Generic finite state machine
- **Data Compression**: RLE and dictionary-based compression

---

## Version 3.6.3 New Components

### 1. Event Bus (IMP-017)

**Header:** `<ims/common/event_bus.hpp>`

Type-safe event bus for publish-subscribe patterns:

```cpp
#include <ims/common/event_bus.hpp>
using namespace ims::common;

// Define event type
struct OrderCreated {
    String order_id;
    double amount;
};

// Create event bus
EventBus bus;

// Subscribe to events
auto handle = bus.subscribe<OrderCreated>([](Event<OrderCreated>& e) {
    std::cout << "Order: " << e.data.order_id 
              << " Amount: $" << e.data.amount << std::endl;
});

// Publish events
bus.publish(OrderCreated{"ORD-001", 99.99});

// Priority-based subscription (higher = earlier)
bus.subscribe<OrderCreated>([](Event<OrderCreated>& e) {
    // This runs first
    if (e.data.amount > 1000) {
        e.cancel();  // Prevent further handlers
    }
}, 10);

// One-shot subscription
bus.subscribe_once<OrderCreated>([](Event<OrderCreated>&) {
    std::cout << "First order received!" << std::endl;
});
```

**Features:**
- Type-safe event handling
- Priority-based dispatch
- Event cancellation
- One-shot subscriptions
- Async event bus option
- Topic-based pub/sub

---

## Version 3.5.0 Components

### 2. Configuration Persistence (IMP-011)

**Header:** `<ims/common/config_persistence.hpp>`

JSON-based configuration management with schema validation:

```cpp
#include <ims/common/config_persistence.hpp>
using namespace ims::common;

// Load configuration
ConfigPersistence config("app_config.json");
config.set_auto_backup(true);
config.set_max_backups(5);

if (config.load_with_env()) {  // Interpolates ${ENV_VAR}
    auto db_host = config.get("database.host", ConfigValue("localhost"));
    auto port = config.get("database.port", ConfigValue(5432));
}

// Set values using dot notation
config.set("app.name", ConfigValue("IMS Emulator"));
config.set("app.version", ConfigValue("3.5.0"));
config.save();

// Schema validation
ConfigValidator validator;
ConfigValue schema;
schema["type"] = ConfigValue("object");
// ... define schema
if (!validator.validate(config.root(), schema)) {
    std::cerr << validator.error_report();
}
```

### 2. Connection Pool (IMP-012)

**Header:** `<ims/common/connection_pool.hpp>`

Generic connection pooling with health monitoring:

```cpp
#include <ims/common/connection_pool.hpp>
using namespace ims::common;

// Create pool with factory
auto pool = ConnectionPoolBuilder<DatabaseConnection>()
    .min_connections(2)
    .max_connections(10)
    .connection_timeout(Milliseconds{5000})
    .idle_timeout(Milliseconds{60000})
    .validate_on_borrow(true)
    .factory([]() { return std::make_unique<DatabaseConnection>(); })
    .validator([](DatabaseConnection& c) { return c.ping(); })
    .build();

pool->start_health_checks();

// Acquire connection with RAII
{
    auto conn = pool->acquire_scoped("worker-1");
    if (conn) {
        conn->execute("SELECT * FROM table");
    }
}  // Connection automatically returned

// Check statistics
const auto& stats = pool->stats();
std::cout << "Utilization: " << stats.utilization() << "%\n";
```

### 3. Metrics Collector (IMP-013)

**Header:** `<ims/common/metrics_collector.hpp>`

Real-time performance metrics with histograms:

```cpp
#include <ims/common/metrics_collector.hpp>
using namespace ims::common;

MetricsRegistry metrics("app");

// Counter
auto& requests = metrics.counter("requests_total", "Total requests");
requests.increment();

// Gauge
auto& connections = metrics.gauge("active_connections");
connections.set(42);

// Histogram for latencies
auto& latency = metrics.histogram("request_latency_ms");
latency.observe(15.5);
latency.observe(22.3);

// Timer with RAII
{
    ScopedMetricTimer timer(metrics.timer("operation_duration"));
    // ... operation ...
}  // Duration automatically recorded

// Get statistics
auto summary = latency.summary();
std::cout << "p99: " << summary.p99 << "ms\n";

// Export to CSV
metrics.export_to_file("metrics.csv");
```

### 4. Command Processor (IMP-014)

**Header:** `<ims/common/command_processor.hpp>`

Interactive CLI framework:

```cpp
#include <ims/common/command_processor.hpp>
using namespace ims::common;

CommandProcessor cli;
cli.set_prompt("ims> ");

// Register custom command
CommandBuilder("status")
    .description("Show system status")
    .usage("status [--verbose]")
    .category("System")
    .handler([](CommandContext& ctx) {
        bool verbose = ctx.has_flag("verbose");
        ctx.out << "System OK\n";
        return CommandResult::ok();
    })
    .register_to(cli);

// Add alias
cli.add_alias("stat", "status");

// Run interactive loop
cli.run();

// Or execute single command
auto result = cli.execute("status --verbose");
```

### 5. State Machine (IMP-015)

**Header:** `<ims/common/state_machine.hpp>`

Generic finite state machine:

```cpp
#include <ims/common/state_machine.hpp>
using namespace ims::common;

enum class OrderState { Pending, Processing, Shipped, Delivered, Cancelled };
struct OrderEvent { std::string type; std::string data; };
struct OrderContext { std::string order_id; double total; };

auto fsm = StateMachineBuilder<OrderState, OrderEvent, OrderContext>()
    .state(OrderState::Pending, "Pending",
        [](OrderContext& ctx, const OrderEvent*) {
            std::cout << "Order " << ctx.order_id << " created\n";
        })
    .state(OrderState::Processing, "Processing")
    .state(OrderState::Shipped, "Shipped")
    .terminal_state(OrderState::Delivered, "Delivered")
    .terminal_state(OrderState::Cancelled, "Cancelled")
    .initial(OrderState::Pending)
    .transition(OrderState::Pending, OrderState::Processing,
        [](const OrderEvent& e) { return e.type == "process"; })
    .transition(OrderState::Processing, OrderState::Shipped,
        [](const OrderEvent& e) { return e.type == "ship"; })
    .guarded_transition(OrderState::Pending, OrderState::Cancelled,
        [](const OrderEvent& e) { return e.type == "cancel"; },
        [](const OrderContext& ctx, const OrderEvent&) { return ctx.total < 1000; })
    .build();

fsm->start();
fsm->process_event(OrderEvent{"process", ""});
fsm->process_event(OrderEvent{"ship", ""});
```

### 6. Data Compression (IMP-016)

**Header:** `<ims/common/compression.hpp>`

Compression optimized for mainframe records:

```cpp
#include <ims/common/compression.hpp>
using namespace ims::common;

// RLE compression
RleCompressor rle;
ByteBuffer data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x42, 0x43};
auto result = rle.compress(data);
std::cout << "Ratio: " << (result.ratio() * 100) << "%\n";

auto decompressed = rle.decompress(result.data);

// Dictionary compression
DictionaryCompressor dict;
dict.set_window_size(4096);
auto dict_result = dict.compress(large_data);

// Record compression (optimized for fixed-format records)
RecordCompressor rec;
rec.set_record_length(80);
rec.set_reference_record(template_record);
auto rec_result = rec.compress_record(current_record);

// Block compression for multiple records
Vector<ByteBuffer> records = {rec1, rec2, rec3};
auto block_result = rec.compress_block(records);

// Automatic compression type recommendation
auto type = recommend_compression(data);
```

---

## Previous Version Components

### 1. Data Validation Framework

**Header:** `<ims/common/validation.hpp>`

Provides comprehensive validation for mainframe data formats:

```cpp
#include <ims/common/validation.hpp>
using namespace ims::validation;

// Validate dataset name (44-character IBM convention)
auto result = DatasetNameValidator::validate("USER.PROD.CUSTOMER.DATA");
if (!result) {
    for (const auto& error : result.errors) {
        std::cerr << "Error: " << error << '\n';
    }
}

// Validate volume serial
auto volser_result = VolumeSerialValidator::validate("PROD01");

// Validate record size
auto size_result = RecordSizeValidator::validate_lrecl(500, true);

// Composite validation
CompositeValidator validator;
validator.add([&]{ return DatasetNameValidator::validate(name); })
         .add([&]{ return RecordSizeValidator::validate_lrecl(lrecl, true); });
auto combined = validator.validate();
```

### 2. Diagnostic Utilities

**Header:** `<ims/common/diagnostics.hpp>`

Tools for debugging and troubleshooting:

```cpp
#include <ims/common/diagnostics.hpp>
using namespace ims::diagnostics;

// Hex dump
ByteBuffer data = {0x00, 0x01, 0x02, 0x03, 0x48, 0x65, 0x6C, 0x6C, 0x6F};
std::cout << HexDump::format(data) << '\n';
// Output:
// 00000000  00 01 02 03 48 65 6C 6C  6F                      |....Hello|

// EBCDIC translation
auto ebcdic = EbcdicTranslator::ascii_to_ebcdic(ascii_data);
auto ascii = EbcdicTranslator::ebcdic_to_ascii(ebcdic_data);

// Debug tracing
DebugTrace::enable(DebugTrace::DEBUG);
IMS_TRACE_INFO("Processing record: {}", record_id);
IMS_TRACE_DEBUG("Buffer size: {} bytes", buffer.size());
```

### 3. LRU Cache

**Header:** `<ims/common/cache.hpp>`

Thread-safe caching with LRU eviction:

```cpp
#include <ims/common/cache.hpp>
using namespace ims;

// Create cache with 1000 entry capacity
LruCache<String, CatalogEntry> cache(1000);

// Add entries
cache.put("USER.DATA.SET1", entry1);
cache.put("USER.DATA.SET2", entry2);

// Retrieve entries
if (auto entry = cache.get("USER.DATA.SET1")) {
    process(*entry);
}

// Check statistics
auto stats = cache.statistics();
std::cout << "Hit rate: " << stats.hit_rate() << "%\n";

// Time-based cache with 5-minute TTL
TimedCache<String, SecurityToken> token_cache(Seconds(300));
token_cache.put("user123", token);
```

### 4. Retry Mechanism

**Header:** `<ims/common/retry.hpp>`

Exponential backoff with jitter for transient failures:

```cpp
#include <ims/common/retry.hpp>
using namespace ims;

// Configure retry policy
RetryPolicy policy;
policy.max_attempts = 5;
policy.initial_delay = Milliseconds(100);
policy.multiplier = 2.0;
policy.jitter_factor = 0.1;

// Execute with retry
RetryExecutor executor(policy);
auto result = executor.execute([&]() {
    return database.connect();
});

if (result.succeeded) {
    std::cout << "Connected after " << result.attempts << " attempts\n";
} else {
    for (const auto& error : result.errors) {
        std::cerr << error << '\n';
    }
}

// Convenience function
auto data = with_retry([&]{ return fetch_data(key); }, 3);
```

### 5. String Utilities

**Header:** `<ims/common/string_utils.hpp>`

Mainframe-compatible string operations:

```cpp
#include <ims/common/string_utils.hpp>
using namespace ims::strings;

// Padding
auto padded = pad_right("HELLO", 10);      // "HELLO     "
auto left_pad = pad_left("123", 8);         // "     123"
auto zero_pad = pad_zero(42, 6);            // "000042"
auto centered = center("IMS", 10);          // "   IMS    "

// Trimming
auto trimmed = trim("  Hello World  ");     // "Hello World"

// Case conversion
auto upper = to_upper("hello");             // "HELLO"
auto lower = to_lower("WORLD");             // "world"

// Split and join
auto parts = split("A.B.C", '.');           // {"A", "B", "C"}
auto joined = join(parts, ".");             // "A.B.C"

// Mainframe formats
auto pic9 = format_pic9(12345, 8);          // "00012345"
auto packed = format_packed(12345, 4);       // COMP-3 format
auto value = parse_packed(packed);           // 12345

// Human-readable
auto size = format_bytes(1234567);           // "1.18 MB"
auto duration = format_duration(Milliseconds(3661000)); // "1h 1m 1s"
```

### 6. Time Utilities

**Header:** `<ims/common/time_utils.hpp>`

IBM-compatible time formats:

```cpp
#include <ims/common/time_utils.hpp>
using namespace ims::time;

// STCK (Store Clock)
auto stck = Stck::now();
auto hex = stck.to_hex();                   // 16-character hex string
auto system_time = stck.to_system_time();

// Julian dates
auto julian = JulianDate::today();
auto yyddd = julian.to_yyddd();             // "25356"
auto yyyyddd = julian.to_yyyyddd();         // "2025356"
auto gregorian = julian.to_gregorian();

// Timestamp formatting
auto iso = TimestampFormatter::now_iso8601();
// "2025-12-22T15:30:45.123Z"

auto mf = TimestampFormatter::now_mainframe();
// "2025-12-22-15.30.45.123456"

auto db2 = TimestampFormatter::now_db2();
// "2025-12-22 15:30:45.123456"

// Duration parsing
auto dur = DurationUtils::parse("2h30m");   // Milliseconds(9000000)
auto fmt = DurationUtils::format(dur);       // "2h 30m"
```

### 7. Benchmark Framework

**Header:** `<ims/common/benchmark.hpp>`

Performance measurement and analysis:

```cpp
#include <ims/common/benchmark.hpp>
using namespace ims::benchmark;

// Simple benchmark
auto result = Benchmark("VSAM Read")
    .warmup(100)
    .iterations(10000)
    .run([&]() {
        vsam.read(key);
    });

std::cout << result.to_string();
// Benchmark: VSAM Read
//   Iterations: 10000
//   Ops/sec: 125000
//   Avg: 8.00 us
//   P95: 12.5 us
//   P99: 18.2 us

// Benchmark suite
BenchmarkSuite suite("VSAM Operations");
suite.run("Sequential Read", [&]{ vsam.read_next(); });
suite.run("Random Read", [&]{ vsam.read(random_key()); });
suite.run("Insert", [&]{ vsam.insert(record); });

std::cout << suite.to_string();
auto csv = suite.to_csv();  // For analysis
```

### 8. Storage Abstraction

**Header:** `<ims/common/storage.hpp>`

Cross-platform persistent storage:

```cpp
#include <ims/common/storage.hpp>
using namespace ims::storage;

// File storage
FileStorage storage("data.dat", StorageOptions{
    .create_if_missing = true,
    .sync_on_write = true
});

storage.write(buffer.data(), buffer.size(), offset);
storage.read(read_buffer.data(), size, offset);
storage.sync();

// Memory storage
MemoryStorage mem_storage(1024 * 1024);  // 1MB
mem_storage.write(data.data(), data.size(), 0);
```

---

## Enhanced Existing Components

### Types Enhancement

- Added `ScopedTimer` for easy performance measurement
- Added `ScopeGuard` for RAII cleanup
- Added additional container aliases (Set, HashSet, Queue, Deque)
- Enhanced `PerformanceMetrics` with I/O tracking
- Added version constants

### Error Handling

- Improved error messages with more context
- Added source location tracking
- Enhanced ErrorResult with mapping operations

---

## Build System Improvements

### New CMake Options

```cmake
option(IMS_BUILD_BENCHMARKS "Build benchmark tests" ON)
option(IMS_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
```

### Enhanced Platform Support

- Added compile definitions for platform detection
- Added SIMD detection (SSE4.2, AVX2)
- Added multi-config generator support
- Added installation targets

---

## Documentation Improvements

### New Documentation

- Comprehensive BACKGROUND.md with Table of Contents
- Enhanced API_REFERENCE.md with new components
- Updated BUILD_NOTES.md with platform-specific instructions
- This IMPROVEMENTS.md document

### Documentation Structure

All documentation files are now in the `docs/` directory:
- BACKGROUND.md
- README.md
- API_REFERENCE.md
- BUILD_NOTES.md
- CHANGELOG.md
- IMPROVEMENTS.md
- LICENSE.txt

---

## Usage Examples

### Complete Validation Example

```cpp
#include <ims/common/validation.hpp>
#include <ims/common/string_utils.hpp>

void process_dataset(const std::string& name, uint32_t lrecl) {
    using namespace ims::validation;
    using namespace ims::strings;
    
    // Normalize and validate name
    auto normalized = to_upper(name);
    auto name_result = DatasetNameValidator::validate(normalized);
    
    if (!name_result) {
        throw std::invalid_argument(name_result.to_string());
    }
    
    // Validate record length
    auto size_result = RecordSizeValidator::validate_lrecl(lrecl, true);
    if (!size_result) {
        throw std::invalid_argument(size_result.to_string());
    }
    
    // Process warnings
    for (const auto& warning : size_result.warnings) {
        std::cerr << "Warning: " << warning << '\n';
    }
    
    // Continue processing...
}
```

### Complete Caching Example

```cpp
#include <ims/common/cache.hpp>
#include <ims/catalog/catalog_types.hpp>

class CatalogCache {
    ims::LruCache<std::string, ims::catalog::CatalogEntry> cache_{10000};
    
public:
    std::optional<ims::catalog::CatalogEntry> get(const std::string& name) {
        return cache_.get(name);
    }
    
    void put(const std::string& name, const ims::catalog::CatalogEntry& entry) {
        cache_.put(name, entry);
    }
    
    void print_stats() {
        auto stats = cache_.statistics();
        std::cout << stats.to_string() << '\n';
    }
};
```

---

## Migration Notes

### From Previous Versions to 3.6.0

Version 3.6.3 is fully backward compatible. No code changes are required
to existing applications.

To use new features:
1. Include the appropriate new headers
2. Link against the same libraries (no new libraries required)

---

*Copyright (c) 2025 Bennie Shearer - MIT License*
