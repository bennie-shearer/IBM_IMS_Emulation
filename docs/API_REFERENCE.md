# IMS (Information Management System) Emulation - API Reference
Version 3.6.2

This document provides a comprehensive reference for all public APIs in the IBM IMS Emulation Enterprise library.

---

## Table of Contents

1. [Common Types](#common-types)
2. [Error Handling](#error-handling)
3. [Validation](#validation)
4. [Diagnostics](#diagnostics)
5. [Caching](#caching)
6. [Retry Mechanism](#retry-mechanism)
7. [String Utilities](#string-utilities)
8. [Time Utilities](#time-utilities)
9. [Benchmarking](#benchmarking)
10. [Storage](#storage)
11. [Catalog Types](#catalog-types)
12. [VSAM Types](#vsam-types)
13. [IMS Database Types](#ims-database-types)
14. [Security](#security)
15. [GDG Types](#gdg-types)

---

## Common Types

**Header:** `<ims/common/types.hpp>`

### Fundamental Types

```cpp
namespace ims {
    using Int8    = std::int8_t;
    using Int16   = std::int16_t;
    using Int32   = std::int32_t;
    using Int64   = std::int64_t;
    using UInt8   = std::uint8_t;
    using UInt16  = std::uint16_t;
    using UInt32  = std::uint32_t;
    using UInt64  = std::uint64_t;
    using Byte    = std::uint8_t;
    using Size    = std::size_t;
}
```

### Container Types

```cpp
template<typename T> using Vector = std::vector<T>;
template<typename K, typename V> using Map = std::map<K, V>;
template<typename K, typename V> using HashMap = std::unordered_map<K, V>;
template<typename T> using Optional = std::optional<T>;
template<typename T> using UniquePtr = std::unique_ptr<T>;
template<typename T> using SharedPtr = std::shared_ptr<T>;
```

### RecordKey

```cpp
struct RecordKey {
    ByteBuffer data;
    UInt32 length{0};
    UInt32 offset{0};
    
    RecordKey();
    explicit RecordKey(Size size);
    RecordKey(const Byte* ptr, Size size);
    RecordKey(const String& str);
    
    bool operator==(const RecordKey& other) const;
    bool operator<(const RecordKey& other) const;
    String to_string() const;
    bool empty() const;
    Size size() const;
};
```

### DataRecord

```cpp
struct DataRecord {
    ByteBuffer data;
    UInt32 length{0};
    UInt64 rba{0};
    UInt32 slot{0};
    RecordKey key;
    SystemTimePoint timestamp;
    UInt32 flags{0};
    
    DataRecord();
    explicit DataRecord(Size size);
    DataRecord(const Byte* ptr, Size size);
    bool empty() const;
    Size size() const;
};
```

### PerformanceMetrics

```cpp
struct PerformanceMetrics {
    void record_operation(bool success, Int64 response_time_ns);
    void record_io(UInt64 read_bytes, UInt64 write_bytes);
    void reset();
    double get_success_rate() const;
    double get_average_response_time_ms() const;
    double get_operations_per_second() const;
    double get_throughput_mb_per_sec() const;
    String to_string() const;
};
```

### ScopedTimer

```cpp
class ScopedTimer {
public:
    explicit ScopedTimer(Int64* result_ns);
    ~ScopedTimer();
    Int64 elapsed_ns() const;
    double elapsed_ms() const;
};
```

### ScopeGuard

```cpp
template<typename Func>
class ScopeGuard {
public:
    explicit ScopeGuard(Func&& func);
    ~ScopeGuard();
    void dismiss();
};

template<typename Func>
ScopeGuard<Func> make_scope_guard(Func&& func);
```

---

## Error Handling

**Header:** `<ims/common/error.hpp>`

### ImsErrorCode

```cpp
enum class ImsErrorCode : Int32 {
    SUCCESS = 0,
    UNKNOWN_ERROR = 1,
    INVALID_ARGUMENT = 2,
    NOT_FOUND = 9,
    FILE_NOT_FOUND = 100,
    DATASET_NOT_FOUND = 200,
    RECORD_NOT_FOUND = 300,
    DUPLICATE_KEY = 306,
    AUTHENTICATION_FAILED = 500,
    TRANSACTION_ABORTED = 600,
    DLI_ERROR = 700,
    SYSTEM_ERROR = 900,
    // ... see header for complete list
};
```

### ErrorResult<T>

```cpp
template<typename T>
class ErrorResult {
public:
    ErrorResult(T value);
    ErrorResult(const char* error);
    
    static ErrorResult make_error(String error);
    static ErrorResult make_success(T value);
    
    bool has_value() const noexcept;
    explicit operator bool() const noexcept;
    bool is_error() const noexcept;
    
    T& value() &;
    const T& value() const&;
    const String& error() const;
    T value_or(T default_value) const;
    
    template<typename F> auto map(F&& f) const;
    template<typename F> auto and_then(F&& f) const;
};
```

### ImsException

```cpp
class ImsException : public std::runtime_error {
public:
    explicit ImsException(const ErrorInfo& info);
    ImsException(ImsErrorCode code, const String& message,
                 const std::source_location& loc = std::source_location::current());
    
    ImsErrorCode code() const noexcept;
    const ErrorInfo& info() const noexcept;
};
```

---

## Validation

**Header:** `<ims/common/validation.hpp>`

### ValidationResult

```cpp
struct ValidationResult {
    bool valid{true};
    Vector<String> errors;
    Vector<String> warnings;
    
    void add_error(const String& msg);
    void add_warning(const String& msg);
    operator bool() const;
    String to_string() const;
};
```

### DatasetNameValidator

```cpp
class DatasetNameValidator {
public:
    static constexpr Size MAX_DATASET_NAME_LENGTH = 44;
    static constexpr Size MAX_QUALIFIER_LENGTH = 8;
    
    static ValidationResult validate(StringView name);
    static bool is_valid(StringView name);
    static String normalize(StringView name);
};
```

### MemberNameValidator

```cpp
class MemberNameValidator {
public:
    static constexpr Size MAX_MEMBER_NAME_LENGTH = 8;
    static ValidationResult validate(StringView name);
    static bool is_valid(StringView name);
};
```

### VolumeSerialValidator

```cpp
class VolumeSerialValidator {
public:
    static constexpr Size VOLSER_LENGTH = 6;
    static ValidationResult validate(StringView volser);
    static bool is_valid(StringView volser);
    static String normalize(StringView volser);
};
```

### RecordSizeValidator

```cpp
class RecordSizeValidator {
public:
    static ValidationResult validate_lrecl(UInt32 lrecl, bool is_vsam = false);
    static ValidationResult validate_blksize(UInt32 blksize, UInt32 lrecl);
};
```

---

## Diagnostics

**Header:** `<ims/common/diagnostics.hpp>`

### HexDump

```cpp
class HexDump {
public:
    struct Options {
        Size bytes_per_line{16};
        Size group_size{4};
        bool show_offset{true};
        bool show_ascii{true};
        bool uppercase{true};
        UInt64 base_offset{0};
    };
    
    static String format(const void* data, Size size, const Options& opts = {});
    static String format(const ByteBuffer& buffer, const Options& opts = {});
    static String format(StringView str, const Options& opts = {});
};
```

### EbcdicTranslator

```cpp
class EbcdicTranslator {
public:
    static Byte to_ebcdic(Byte ascii);
    static Byte to_ascii(Byte ebcdic);
    static ByteBuffer ascii_to_ebcdic(const ByteBuffer& ascii);
    static ByteBuffer ebcdic_to_ascii(const ByteBuffer& ebcdic);
    static String ascii_string_to_ebcdic(StringView ascii);
    static String ebcdic_string_to_ascii(StringView ebcdic);
};
```

### DiagnosticCollector

```cpp
class DiagnosticCollector {
public:
    void log(const String& entry);
    DiagnosticInfo collect(const String& component) const;
    Vector<String> get_log() const;
    void clear_log();
};

DiagnosticCollector& global_diagnostics();
```

### DebugTrace

```cpp
class DebugTrace {
public:
    enum Level : UInt32 { NONE, ERROR, WARNING, INFO, DEBUG, VERBOSE };
    
    static void enable(UInt32 level = INFO);
    static void disable();
    static bool is_enabled(UInt32 level = INFO);
    
    template<typename... Args>
    static void trace(UInt32 level, const String& format_str, Args&&... args);
};

// Macros
#define IMS_TRACE_ERROR(...)
#define IMS_TRACE_WARNING(...)
#define IMS_TRACE_INFO(...)
#define IMS_TRACE_DEBUG(...)
#define IMS_TRACE_VERBOSE(...)
```

---

## Caching

**Header:** `<ims/common/cache.hpp>`

### LruCache<Key, Value>

```cpp
template<typename Key, typename Value>
class LruCache {
public:
    explicit LruCache(Size capacity = 1000);
    
    Optional<Value> get(const Key& key);
    bool put(const Key& key, Value value);
    bool remove(const Key& key);
    bool contains(const Key& key) const;
    void clear();
    Size size() const;
    Size capacity() const;
    void resize(Size new_capacity);
    
    const CacheStatistics& statistics() const;
    void reset_statistics();
    Vector<Key> keys() const;
};
```

### TimedCache<Key, Value>

```cpp
template<typename Key, typename Value>
class TimedCache {
public:
    explicit TimedCache(Duration default_ttl = Seconds(300));
    
    Optional<Value> get(const Key& key);
    void put(const Key& key, Value value, Optional<Duration> ttl = std::nullopt);
    bool remove(const Key& key);
    void clear();
    void cleanup();
    Size size() const;
};
```

---

## Retry Mechanism

**Header:** `<ims/common/retry.hpp>`

### RetryPolicy

```cpp
struct RetryPolicy {
    UInt32 max_attempts{3};
    Milliseconds initial_delay{100};
    Milliseconds max_delay{10000};
    double multiplier{2.0};
    double jitter_factor{0.1};
    bool retry_on_timeout{true};
    Set<ImsErrorCode> retryable_errors;
    
    static RetryPolicy none();
    static RetryPolicy aggressive();
    static RetryPolicy conservative();
    bool is_retryable(ImsErrorCode code) const;
};
```

### RetryExecutor

```cpp
class RetryExecutor {
public:
    explicit RetryExecutor(const RetryPolicy& policy = RetryPolicy{});
    
    template<typename Func>
    auto execute(Func&& func) -> RetryResult<decltype(func())>;
    
    template<typename T, typename Func>
    RetryResult<T> execute_with_result(Func&& func);
};

template<typename Func>
auto with_retry(Func&& func, const RetryPolicy& policy = RetryPolicy{});

template<typename Func>
auto with_retry(Func&& func, UInt32 max_attempts);
```

---

## String Utilities

**Header:** `<ims/common/string_utils.hpp>`

### Padding

```cpp
String pad_right(StringView str, Size width, char pad_char = ' ');
String pad_left(StringView str, Size width, char pad_char = ' ');
String pad_zero(UInt64 value, Size width);
String center(StringView str, Size width, char pad_char = ' ');
```

### Trimming

```cpp
StringView trim_left(StringView str);
StringView trim_right(StringView str);
StringView trim(StringView str);
StringView trim_char(StringView str, char c);
```

### Case Conversion

```cpp
String to_upper(StringView str);
String to_lower(StringView str);
String capitalize(StringView str);
```

### Split and Join

```cpp
Vector<String> split(StringView str, char delimiter);
Vector<String> split(StringView str, StringView delimiter);
String join(const Vector<String>& parts, StringView delimiter);
```

### Mainframe Formatting

```cpp
String format_pic9(Int64 value, Size width);
ByteBuffer format_packed(Int64 value, Size bytes);
Int64 parse_packed(const ByteBuffer& packed);
String format_zoned(Int64 value, Size width);
```

### Parsing

```cpp
Optional<Int64> parse_int(StringView str);
Optional<UInt64> parse_uint(StringView str);
Optional<Float64> parse_double(StringView str);
```

### Formatting

```cpp
String format_bytes(UInt64 bytes);
String format_duration(Milliseconds ms);
```

---

## Time Utilities

**Header:** `<ims/common/time_utils.hpp>`

### Stck (Store Clock)

```cpp
class Stck {
public:
    Stck();
    explicit Stck(UInt64 value);
    
    static Stck from_system_time(SystemTimePoint tp);
    static Stck now();
    
    SystemTimePoint to_system_time() const;
    UInt64 value() const;
    String to_hex() const;
    static Optional<Stck> from_hex(StringView hex);
};
```

### JulianDate

```cpp
struct JulianDate {
    UInt16 year;
    UInt16 day;
    
    static JulianDate from_system_time(SystemTimePoint tp);
    static JulianDate today();
    
    GregorianDate to_gregorian() const;
    String to_yyddd() const;
    String to_yyyyddd() const;
    
    static Optional<JulianDate> from_yyddd(StringView str);
    static Optional<JulianDate> from_yyyyddd(StringView str);
};
```

### TimestampFormatter

```cpp
class TimestampFormatter {
public:
    static String format_iso8601(SystemTimePoint tp);
    static String format_mainframe(SystemTimePoint tp);
    static String format_smf_time(SystemTimePoint tp);
    static String format_smf_date(SystemTimePoint tp);
    static String format_db2(SystemTimePoint tp);
    
    static String now_iso8601();
    static String now_mainframe();
    static String now_db2();
};
```

### DurationUtils

```cpp
class DurationUtils {
public:
    static Optional<Milliseconds> parse(StringView str);
    static String format(Milliseconds ms);
};
```

---

## Benchmarking

**Header:** `<ims/common/benchmark.hpp>`

### Benchmark

```cpp
class Benchmark {
public:
    explicit Benchmark(String name);
    
    Benchmark& warmup(UInt64 iterations);
    Benchmark& iterations(UInt64 iterations);
    Benchmark& samples(bool collect);
    
    template<typename Func>
    BenchmarkResult run(Func&& func);
};
```

### BenchmarkResult

```cpp
struct BenchmarkResult {
    String name;
    UInt64 iterations{0};
    Duration total_time{0};
    Duration min_time;
    Duration max_time;
    Vector<Duration> samples;
    
    double ops_per_second() const;
    double avg_time_ns() const;
    double avg_time_us() const;
    double avg_time_ms() const;
    double stddev_ns() const;
    Duration median_time() const;
    Duration percentile(double p) const;
    String to_string() const;
};
```

### BenchmarkSuite

```cpp
class BenchmarkSuite {
public:
    explicit BenchmarkSuite(String name);
    
    void add_result(BenchmarkResult result);
    
    template<typename Func>
    BenchmarkResult& run(const String& name, Func&& func, UInt64 iterations = 1000);
    
    const Vector<BenchmarkResult>& results() const;
    String to_string() const;
    String to_csv() const;
};
```

---

## Storage

**Header:** `<ims/common/storage.hpp>`

### FileStorage

```cpp
class FileStorage {
public:
    FileStorage();
    explicit FileStorage(const Path& path, const StorageOptions& options = {});
    
    ErrorResult<void> open(const Path& path, const StorageOptions& options = {});
    void close();
    bool is_open() const;
    
    ErrorResult<Size> read(void* buffer, Size size, UInt64 offset);
    ErrorResult<Size> write(const void* buffer, Size size, UInt64 offset);
    ErrorResult<void> sync();
    ErrorResult<UInt64> size() const;
    ErrorResult<void> truncate(UInt64 new_size);
    
    const Path& path() const;
    const StorageStats& stats() const;
};
```

### MemoryStorage

```cpp
class MemoryStorage {
public:
    explicit MemoryStorage(Size initial_capacity = 1024 * 1024);
    
    ErrorResult<Size> read(void* buffer, Size size, UInt64 offset);
    ErrorResult<Size> write(const void* buffer, Size size, UInt64 offset);
    ErrorResult<void> sync();
    Size size() const;
    void clear();
    
    const ByteBuffer& data() const;
    ByteBuffer& data();
};
```

---

## Catalog Types

**Header:** `<ims/catalog/catalog_types.hpp>`

### DatasetOrganization

```cpp
enum class DatasetOrganization : UInt8 {
    UNKNOWN, PS, PO, DA, IS, VS, VSAM, HFS, PDSE, ZFS
};
```

### VsamType

```cpp
enum class VsamType : UInt8 {
    NONE, KSDS, ESDS, RRDS, LDS, VRRDS
};
```

### CatalogEntry

```cpp
struct CatalogEntry {
    String name;
    String alias;
    DatasetOrganization dsorg;
    VsamType vsam_type;
    RecordFormat recfm;
    UInt32 lrecl;
    UInt32 blksize;
    UInt32 keylen;
    UInt32 keyoff;
    String volser;
    // ... additional fields
};
```

---

## IMS Database Types

**Header:** `<ims/ims/ims_types.hpp>`

### DatabaseType

```cpp
enum class DatabaseType : UInt8 {
    HIDAM, HDAM, HISAM, HSAM, SHISAM, SHSAM, INDEX, HALDB
};
```

### DliCall

```cpp
enum class DliCall : UInt8 {
    GU, GN, GNP, ISRT, DLET, REPL, CHKP, XRST, ROLB, ROLL, SETS, SETO
};
```

### DliStatusCode

```cpp
enum class DliStatusCode : UInt16 {
    NORMAL = 0x0000,
    END_OF_DATABASE = 0x0047,
    SEGMENT_NOT_FOUND = 0x0048,
    // ... additional codes
};
```

### DatabaseDefinition

```cpp
struct DatabaseDefinition {
    String name;
    DatabaseType type;
    AccessMethod access_method;
    Vector<SegmentDefinition> segments;
    String index_name;
    String vsam_dataset;
    UInt32 max_segments;
    bool is_partitioned;
};
```

---

*Copyright (c) 2025 Bennie Shearer - MIT License*
