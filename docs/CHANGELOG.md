# IBM IMS (Information Management System) Emulation - Change Log
Version 3.6.2

All notable changes to IBM IMS Emulation Enterprise are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [3.6.2] - 2026-01-08

### Added

#### New Modules
- **Object Pool** (`object_pool.hpp`)
  - High-performance object pooling for frequent allocations
  - Pre-allocated pools with automatic growth
  - RAII scoped object acquisition
  - Statistics tracking (hit rate, usage)

- **Async Queue** (`async_queue.hpp`)
  - Thread-safe unbounded queue with blocking/non-blocking ops
  - Bounded queue with back-pressure and drop counting
  - Priority queue for ordered processing
  - Work-stealing queue for load balancing

- **Task Scheduler** (`task_scheduler.hpp`)
  - Delayed and immediate task execution
  - Periodic task scheduling
  - Priority-based scheduling
  - Cron-like scheduling with CronSpec

- **Data Mapper** (`data_mapper.hpp`)
  - Field-to-field data mapping
  - Packed decimal and zoned decimal support
  - Record layout definitions
  - Built-in transforms (uppercase, trim, scale)

#### Test Coverage
- Added tests for Object Pool (basic, scoped)
- Added tests for Async Queue (basic, bounded, priority)
- Added tests for Data Mapper (field values, record defs, transforms)

### Fixed

#### Version Consistency
- Fixed version strings in examples (was 3.6.0, now 3.6.2)
- Updated all version numbers to 3.6.2

### Changed

#### Documentation
- Updated test file header with new module list
- Total common headers: 47 (was 43)

---

## [3.6.1] - 2026-01-08

### Fixed

#### Cross-Platform Compatibility
- **BUG-007:** Fixed ConfigValue constructor overload ambiguity on Windows/MinGW
  - Removed duplicate `long long` constructor that conflicts with `Int64` on some platforms
  - Added explicit `static_cast<Int64>` for `std::stoll()` return value
  - Windows/MinGW and Linux/GCC now compile without errors

#### Code Quality
- Fixed unused variable warning in `connection_pool.hpp` shutdown method
- Fixed unused variable warning in connection pool tests
- Removed `BoolTag` struct that was no longer needed

#### Version Consistency
- Updated all version numbers from 3.6.0 to 3.6.1
- Updated CMakeLists.txt project version to 3.6.1
- Updated IMS_VERSION constants in types.hpp to 3.6.1

---

## [3.6.0] - 2026-01-08

### Added

#### New Modules
- **Event Bus** (`event_bus.hpp`)
  - Type-safe event bus for publish-subscribe pattern
  - Synchronous and asynchronous event dispatch
  - Event prioritization and cancellation
  - Topic-based pub/sub with wildcard matching
  - RAII subscription handles

#### Test Coverage
- Added comprehensive test suite for v3.5.0/v3.6.2 modules
  - ConfigValue and JSON parser/writer tests
  - Metrics collector tests (counter, gauge, histogram)
  - Command processor tests (tokenize, parse, execute)
  - State machine tests (transitions, guards, callbacks)
  - Compression tests (RLE, dictionary)
  - Event bus tests (subscribe, publish, priority, cancel)
  - Connection pool tests (acquire, release, scoped)

### Changed

#### Bug Fixes
- **BUG-001:** Renamed `CompressionType` to `CompressionAlgorithm` in compression.hpp
  to avoid naming conflict with `ims::dfsmshsm::CompressionType`
- **BUG-006:** Fixed version reference "3.4.0" in IMPROVEMENTS.md migration notes

#### Version Consistency
- Updated all version numbers from 3.5.0 to 3.6.0
- Updated CMakeLists.txt project version to 3.6.0
- Updated IMS_VERSION constants in types.hpp to 3.6.0
- Updated all header @version tags to 3.6.0

#### Documentation
- Updated API_REFERENCE.md with new module documentation
- Added RECOMMENDATIONS_v3.6.2.md with analysis findings
- Updated IMPROVEMENTS.md migration notes

### Fixed

#### Code Quality
- Verified no duplicate enum names across namespaces
- Added thread safety documentation for modules
- Ensured all switch statements cover all enum cases

---

## [3.5.0] - 2026-01-07

### Added

#### DL/I Database Type Support
- Added `GSAM` case to `database_type_to_string()` function
- Added `DEDB` case to `database_type_to_string()` function
- Complete coverage of all `DatabaseType` enum values

#### New Recommended Improvements
- **Configuration Persistence** (`config_persistence.hpp`)
  - JSON-based configuration file support
  - Automatic backup and restore capabilities
  - Schema validation for configuration files
  - Environment variable interpolation

- **Connection Pool** (`connection_pool.hpp`)
  - Generic connection pooling for database connections
  - Automatic connection health checks
  - Configurable pool sizing and timeouts
  - Connection leak detection

- **Metrics Collector** (`metrics_collector.hpp`)
  - Real-time performance metrics collection
  - Histogram and percentile calculations
  - Time-series data aggregation
  - Export to CSV format

- **Command Processor** (`command_processor.hpp`)
  - Interactive command-line interface framework
  - Command history and auto-completion support
  - Built-in help system
  - Extensible command registration

- **State Machine** (`state_machine.hpp`)
  - Generic finite state machine implementation
  - Event-driven state transitions
  - Guard conditions and actions
  - State entry/exit callbacks

- **Data Compression** (`compression.hpp`)
  - Run-length encoding (RLE) compression
  - Dictionary-based compression
  - Optimized for mainframe record formats
  - Zero external dependencies

### Changed

#### Project Naming
- Renamed project from `IMS-Emulation` to `IBM_IMS_Emulation`
- Updated all references to use "IBM IMS Emulation Enterprise"

#### Version Consistency
- Updated all version numbers from 3.4.0 to 3.5.0
- Updated CMakeLists.txt project version to 3.5.0
- Updated IMS_VERSION constants in types.hpp to 3.5.0
- Updated all header @version tags to 3.5.0
- Updated all source file version comments to 3.5.0
- Fixed version strings in example applications

#### Documentation
- Moved all .md and .txt files to docs directory
- Updated all documentation headers with "IBM" prefix
- Comprehensive API documentation updates

### Fixed

#### Bug Fixes
- Fixed missing `GSAM` and `DEDB` cases in `database_type_to_string()`
- Fixed version inconsistencies in 12 files that still showed v3.3.0
- Fixed version strings in storage-example and diagnostic-example

#### Code Quality
- Verified all source files use ASCII-only characters
- Ensured cross-platform text file compatibility
- Consistent copyright notices across all files

---

## [3.4.0] - 2026-01-02

### Changed

#### Documentation Updates
- Standardized all documentation headers with consistent naming convention
- Updated API_REFERENCE.md header to "IMS (Information Management System) Emulation - API Reference"
- Updated BACKGROUND.md header to "IMS (Information Management System) Emulation - Background"
- Updated BUILD_NOTES.md header to "IMS (Information Management System) Emulation - Build Notes"
- Updated CHANGELOG.md header to "IMS (Information Management System) Emulation - Change Log"
- Updated IMPROVEMENTS.md header to "IMS (Information Management System) Emulation - Improvements"
- Updated RECOMMENDATIONS.md header to "IMS (Information Management System) Emulation - Recommended Improvement"

#### Version Consistency
- Updated all version numbers from 3.3.0 to 3.4.0
- Updated CMakeLists.txt project version
- Updated IMS_VERSION constants in types.hpp
- Updated all header and source file comments

### Fixed

#### Encoding Consistency
- Verified all source files use ASCII-only characters
- Ensured cross-platform text file compatibility

---

## [3.3.0] - 2025-12-25

### Added

#### New Common Library Modules
- **Byte Buffer Utilities** (`byte_buffer.hpp`)
  - Dynamic and fixed-size byte buffers
  - Big-endian and little-endian read/write operations
  - String handling with padding and null-termination
  - Binary search and pattern matching
  - Hexdump formatting for debugging
  - Zero-copy buffer views

- **Result Type with Error Chaining** (`result.hpp`)
  - Type-safe error handling without exceptions
  - Monadic operations (map, and_then, or_else)
  - Error context chaining for debugging
  - Void specialization for operations without return value

- **File Utilities** (`file_utils.hpp`)
  - Cross-platform file system operations
  - Directory creation/deletion (recursive)
  - File reading/writing (text and binary)
  - Path manipulation (join, parent, filename, extension)
  - Temporary file/directory creation with RAII cleanup
  - Atomic file operations for safe updates

- **Signal Handling** (`signal_handler.hpp`)
  - Cross-platform signal management (POSIX/Win32)
  - SIGINT/SIGTERM handling for graceful shutdown
  - Custom signal handler registration
  - Thread-safe signal flag checking
  - RAII signal blocking guard

- **UUID Generator** (`uuid.hpp`)
  - Version 4 (random) UUID generation per RFC 4122
  - UUID parsing and formatting
  - Thread-safe generation
  - Hash support for STL containers

- **Progress Tracking** (`progress.hpp`)
  - Thread-safe progress tracker
  - Percentage and item-based progress
  - Estimated time remaining (ETA)
  - Console progress bar rendering
  - RAII progress scope for automatic increment

- **Command-Line Argument Parser** (`argparse.hpp`)
  - Short (-v) and long (--verbose) options
  - Required and optional arguments with defaults
  - Positional arguments support
  - Auto-generated help text
  - Choice validation

- **Simple Profiler** (`profiler.hpp`)
  - Function timing with nanosecond precision
  - Call count and statistics tracking
  - RAII timing scopes
  - Formatted report generation
  - PROFILE_FUNCTION() and PROFILE_SCOPE() macros

- **Enhanced Data Validators** (`validators.hpp`)
  - Chainable validation builders
  - String validation (length, format, charset)
  - Numeric range validation
  - Mainframe-specific validators:
    - Dataset name (44-char IBM convention)
    - DD name, member name, job name
    - VSAM key, volume serial
    - LRECL and BLKSIZE

- **Memory Pool Allocator** (`memory_pool.hpp`)
  - O(1) allocation and deallocation
  - Pre-allocated memory chunks
  - Thread-safe variant available
  - STL-compatible allocator
  - Smart pointer with automatic pool return

#### Documentation
- Comprehensive BACKGROUND.md with full Table of Contents
- RECOMMENDATIONS_v3.6.2.md with all implemented improvements

### Fixed

#### Copyright Consistency
- All files now use "Copyright (c) 2025 Bennie Shearer"
- Removed Unicode copyright symbols (replaced with ASCII)
- Fixed incorrect "IMS Systems" and "CICS Systems" attributions

#### Unicode Character Cleanup
- Replaced Unicode box-drawing characters with ASCII equivalents
- Replaced Unicode arrows with ASCII arrows (->)
- Replaced Greek letters (mu) with ASCII alternatives (u)
- All source files are now ASCII-only for maximum compatibility

#### Version Consistency
- All files updated to version 3.3.0
- CMakeLists.txt project version updated
- IMS_VERSION constants in types.hpp updated
- All header and source file comments updated

### Changed
- Documentation files moved to docs/ directory
- Updated all version numbers from 3.2.0 to 3.3.0
- Improved code organization with new modules

### Technical Notes
- All new modules are header-only for easy integration
- Zero external dependencies maintained
- Cross-platform compatibility verified (Windows/Linux/macOS)
- C++20 features used throughout

---

## [3.2.0] - 2025-12-25

### Added

#### New Common Library Features
- **EBCDIC Conversion Module** (`ebcdic.hpp`)
  - Full EBCDIC/ASCII bidirectional conversion tables (Code Page 037)
  - Character classification functions for EBCDIC
  - Packed decimal conversion utilities
  - EBCDIC special character constants

- **JCL Parser Foundation** (`jcl_parser.hpp`)
  - Basic JCL parsing for DD statement extraction
  - Support for JOB, EXEC, DD, PROC statements
  - DISP, DCB, SPACE parameter parsing
  - Continuation line handling

- **SMF Record Generation** (`smf_records.hpp`)
  - SMF Type 14/15 (Dataset Open records)
  - SMF Type 60 (VSAM statistics)
  - SMF Type 199 (IMS Emulation custom records)
  - Thread-safe SMF writer and reader
  - Convenience functions for common record types

#### Enhanced IMS/DL-I Support
- Added Get Hold variants: GHU, GHN, GHNP
- Added GSAM calls: OPEN, CLSE
- Added SYNC (Sync Point) call
- Added POS (Position) call
- Added FLD (Field) call for Fast Path
- New database types: GSAM, DEDB
- New status codes: GK, DJ, DA, AX
- Helper functions: `is_get_hold_call()`, `requires_prior_hold()`

#### Documentation
- Comprehensive ANALYSIS_v3.2.0.md with full code review
- Updated all version comments to 3.2.0

### Fixed

#### Critical Bugs
- **BUG-001**: Fixed `SteadyClock` not defined error in `circuit_breaker.hpp`
  - Changed all `SteadyClock` references to `Clock` (defined in types.hpp)
  - This was a compilation-breaking bug

- **BUG-002**: Fixed incorrect copyright notice in `circuit_breaker.hpp`
  - Changed from "CICS Systems" to "Bennie Shearer"

#### Version Inconsistencies
- Fixed `platform.cpp` showing version 3.0.11 (now 3.2.0)
- Fixed `test_common.cpp` showing version 3.0.11 (now 3.2.0)
- All source files now consistently show version 3.2.0

#### Code Quality
- Removed redundant `#ifndef` guard in `circuit_breaker.hpp` (uses `#pragma once`)

### Changed
- Updated all version numbers from 3.1.0 to 3.2.0
- DL/I call enum values reorganized for logical grouping
- DL/I status codes now use proper 2-byte EBCDIC encoding

### MinGW Compatibility Notes
- Verified std::format compatibility (requires GCC 13+ with MinGW-w64)
- Confirmed thread_local storage support
- Documented _WIN32_WINNT=0x0601 requirement (Windows 7+)
- All std::shared_mutex usage verified compatible

---

## [3.1.0] - 2025-12-22

### Added

#### New Common Library Features
- **Data Validation Framework** (`validation.hpp`)
  - Dataset name validation (44-character IBM convention)
  - Member name validation (8-character PDS members)
  - Volume serial validation (6-character VOLSER)
  - Record size validation for VSAM and QSAM
  - Key field validation for KSDS
  - JCL parameter validation (DD names, job names)
  - Composite validator for multiple rules

- **Diagnostic Utilities** (`diagnostics.hpp`)
  - Hex dump utility with configurable formatting
  - EBCDIC/ASCII translation tables
  - Diagnostic information collector
  - Memory dump utilities
  - Debug trace system with log levels

- **Retry Mechanism** (`retry.hpp`)
  - Exponential backoff with jitter
  - Configurable retry policies
  - Support for retryable error codes
  - Result tracking (attempts, delays, errors)

- **LRU Cache** (`cache.hpp`)
  - Thread-safe LRU cache implementation
  - Time-based cache with TTL
  - Cache statistics (hits, misses, evictions)
  - Configurable capacity

- **String Utilities** (`string_utils.hpp`)
  - Mainframe-style padding (left, right, zero, center)
  - Trimming operations
  - Case conversion
  - Split and join operations
  - COBOL PIC 9 formatting
  - Packed decimal (COMP-3) formatting and parsing
  - Zoned decimal formatting
  - Human-readable byte and duration formatting

- **Time Utilities** (`time_utils.hpp`)
  - STCK (Store Clock) IBM format support
  - Julian date conversion (YYDDD, YYYYDDD)
  - Multiple timestamp formats (ISO 8601, mainframe, SMF, DB2)
  - Duration parsing and formatting
  - Timestamp parsing utilities

- **Benchmark Framework** (`benchmark.hpp`)
  - Performance measurement utilities
  - Statistical analysis (min, max, avg, median, percentiles)
  - Warmup and measurement phases
  - CSV report generation
  - Throughput benchmarking

- **Storage Abstraction** (`storage.hpp`)
  - Cross-platform file storage
  - Memory-based storage
  - Journal entries for write-ahead logging
  - Storage statistics tracking

#### Enhanced Types
- Added `ScopedTimer` for performance measurement
- Added `ScopeGuard` for RAII cleanup
- Added additional container aliases (Set, HashSet, Queue, Deque, PriorityQueue)
- Added byte I/O tracking to `PerformanceMetrics`
- Added version constants (`IMS_VERSION`, `IMS_VERSION_MAJOR`, etc.)

#### Build System
- Added `IMS_BUILD_BENCHMARKS` CMake option
- Added `IMS_ENABLE_WARNINGS_AS_ERRORS` CMake option
- Enhanced platform detection with compile definitions
- Added SIMD detection for SSE4.2 and AVX2
- Added multi-config generator support (Visual Studio, Xcode)
- Added installation targets

#### Documentation
- Comprehensive BACKGROUND.md with Table of Contents
- Enhanced API_REFERENCE.md with new components
- Updated BUILD_NOTES.md with platform-specific instructions
- Added IMPROVEMENTS.md documenting v3.1.0 features

### Changed
- Updated all version numbers to 3.1.0
- Improved hex_to_bytes function with case-insensitive parsing
- Enhanced error handling throughout

### Fixed
- Consistent copyright notices across all files
- Improved thread safety in cache implementations
- Better error messages with context

---

## [3.0.12] - 2025-12-13

### Added
- BACKGROUND.md document covering IMS history from Apollo program
- Hierarchical database model documentation
- DL/I call interface documentation

### Changed
- Updated documentation structure
- Improved code organization

---

## [3.0.11] - 2025-12-11

### Added
- **Enterprise Features**
  - `ResourcePool` - Generic resource pooling with health checks
  - `TokenBucketLimiter` - Rate limiting implementation
  - `CircuitBreaker` - Fault tolerance pattern
  - `HealthCheckManager` - System health monitoring
  - `ThreadPool` - Configurable thread pool
  - `BufferPool` - Memory buffer management
  - `BinaryWriter/Reader` - Binary serialization
  - `EventDispatcher` - Event-driven architecture support
  - `BatchProcessor` - Batch operation processing
  - `AuditManager` - Audit trail management

### Fixed
- hex_to_bytes type conversion warnings
- BatchResult double conversion issues
- Unused parameter warnings throughout
- CustomerRecord null terminator handling

---

## [3.0.10] - 2025-12-08

### Added
- Cross-platform build verification
- MinGW-w64 support for Windows

### Fixed
- Windows-specific compilation issues
- Platform macro conflicts

---

## [3.0.9] - 2025-12-05

### Added
- GDG (Generation Data Group) support
- DFSMShsm storage management simulation

### Changed
- Improved VSAM record handling
- Enhanced catalog operations

---

## [3.0.8] - 2025-12-01

### Added
- IMS database types (HIDAM, HDAM, HISAM, HSAM)
- DL/I status codes
- Segment definition structures

### Changed
- Refactored IMS core module
- Improved error reporting

---

## [3.0.7] - 2025-11-25

### Added
- Security context framework
- Authentication and authorization
- Session management

---

## [3.0.6] - 2025-11-20

### Added
- Master catalog implementation
- Dataset organization types
- Volume management

---

## [3.0.5] - 2025-11-15

### Added
- VSAM KSDS implementation
- Key-sequenced dataset operations
- Index management

---

## [3.0.4] - 2025-11-10

### Added
- VSAM ESDS implementation
- Entry-sequenced operations

---

## [3.0.3] - 2025-11-05

### Added
- VSAM RRDS implementation
- Relative record operations

---

## [3.0.2] - 2025-11-01

### Added
- VSAM LDS implementation
- Linear dataset support

---

## [3.0.1] - 2025-10-25

### Added
- Basic VSAM types
- Record and buffer structures
- Initial error handling

---

## [3.0.0] - 2025-10-20

### Added
- Initial enterprise release
- Core library structure
- C++20 implementation
- Cross-platform CMake build system
- Basic documentation

---

## Version Numbering

This project uses Semantic Versioning:
- **MAJOR**: Incompatible API changes
- **MINOR**: New functionality (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

---

## Upgrade Notes

### From 3.0.x to 3.1.0

Version 3.1.0 is fully backward compatible with 3.0.x. New features are additive
and existing APIs remain unchanged.

To use new features:
1. Include the appropriate new headers
2. No changes required to existing code

### Recommended Updates
- Consider using `validation` module for input validation
- Use `cache` module for frequently accessed data
- Implement `retry` for transient failure handling
- Add `benchmark` tests for performance regression detection

---

*Copyright (c) 2025 Bennie Shearer - MIT License*
