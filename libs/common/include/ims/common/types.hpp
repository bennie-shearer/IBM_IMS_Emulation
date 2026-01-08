#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Common Types
// Version: 3.6.3
// =============================================================================
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>
#include <memory>
#include <functional>
#include <chrono>
#include <filesystem>
#include <variant>
#include <array>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <span>
#include <format>
#include <queue>
#include <deque>
#include <set>
#include <unordered_set>
#include <numeric>
#include <algorithm>
#include <random>

namespace ims {

// =============================================================================
// Version Information
// =============================================================================

constexpr const char* IMS_VERSION = "3.6.3";
constexpr int IMS_VERSION_MAJOR = 3;
constexpr int IMS_VERSION_MINOR = 6;
constexpr int IMS_VERSION_PATCH = 3;

// =============================================================================
// Fundamental Type Aliases
// =============================================================================

using Int8    = std::int8_t;
using Int16   = std::int16_t;
using Int32   = std::int32_t;
using Int64   = std::int64_t;

using UInt8   = std::uint8_t;
using UInt16  = std::uint16_t;
using UInt32  = std::uint32_t;
using UInt64  = std::uint64_t;

using Float32 = float;
using Float64 = double;

using Byte    = std::uint8_t;
using Size    = std::size_t;

// =============================================================================
// String Types
// =============================================================================

using String      = std::string;
using StringView  = std::string_view;
using WString     = std::wstring;
using WStringView = std::wstring_view;

// =============================================================================
// Container Types
// =============================================================================

template<typename T>
using Vector = std::vector<T>;

template<typename T, Size N>
using Array = std::array<T, N>;

template<typename K, typename V>
using Map = std::map<K, V>;

template<typename K, typename V>
using HashMap = std::unordered_map<K, V>;

template<typename T>
using Set = std::set<T>;

template<typename T>
using HashSet = std::unordered_set<T>;

template<typename T>
using Optional = std::optional<T>;

template<typename... Ts>
using Variant = std::variant<Ts...>;

template<typename T>
using Span = std::span<T>;

template<typename T>
using Queue = std::queue<T>;

template<typename T>
using Deque = std::deque<T>;

template<typename T, typename Container = Vector<T>, typename Compare = std::less<typename Container::value_type>>
using PriorityQueue = std::priority_queue<T, Container, Compare>;

// =============================================================================
// Smart Pointer Types
// =============================================================================

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// =============================================================================
// Function Types
// =============================================================================

template<typename Signature>
using Function = std::function<Signature>;

// =============================================================================
// Time Types
// =============================================================================

using Clock           = std::chrono::steady_clock;
using SystemClock     = std::chrono::system_clock;
using HighResClock    = std::chrono::high_resolution_clock;
using TimePoint       = Clock::time_point;
using SystemTimePoint = SystemClock::time_point;
using Duration        = std::chrono::nanoseconds;
using Milliseconds    = std::chrono::milliseconds;
using Seconds         = std::chrono::seconds;
using Minutes         = std::chrono::minutes;
using Hours           = std::chrono::hours;
using Microseconds    = std::chrono::microseconds;

// =============================================================================
// Filesystem Types
// =============================================================================

using Path = std::filesystem::path;

// =============================================================================
// Threading Types
// =============================================================================

using Mutex           = std::mutex;
using RecursiveMutex  = std::recursive_mutex;
using SharedMutex     = std::shared_mutex;
using ConditionVar    = std::condition_variable;
using Thread          = std::thread;

template<typename T>
using Future = std::future<T>;

template<typename T>
using Promise = std::promise<T>;

template<typename T>
using Atomic = std::atomic<T>;

using AtomicBool   = std::atomic<bool>;
using AtomicInt32  = std::atomic<Int32>;
using AtomicInt64  = std::atomic<Int64>;
using AtomicUInt32 = std::atomic<UInt32>;
using AtomicUInt64 = std::atomic<UInt64>;

// =============================================================================
// Lock Types
// =============================================================================

template<typename MutexT>
using UniqueLock = std::unique_lock<MutexT>;

template<typename MutexT>
using SharedLock = std::shared_lock<MutexT>;

template<typename MutexT>
using LockGuard = std::lock_guard<MutexT>;

// =============================================================================
// Byte Buffer Types
// =============================================================================

using ByteBuffer = Vector<Byte>;
using ByteSpan   = Span<Byte>;
using ConstByteSpan = Span<const Byte>;

// =============================================================================
// Record Key Types (Mainframe Compatible)
// =============================================================================

struct RecordKey {
    ByteBuffer data;
    UInt32 length{0};
    UInt32 offset{0};
    
    RecordKey() = default;
    explicit RecordKey(Size size) : data(size), length(static_cast<UInt32>(size)) {}
    RecordKey(const Byte* ptr, Size size) : data(ptr, ptr + size), length(static_cast<UInt32>(size)) {}
    RecordKey(const String& str) : data(str.begin(), str.end()), length(static_cast<UInt32>(str.size())) {}
    
    bool operator==(const RecordKey& other) const { return data == other.data; }
    bool operator<(const RecordKey& other) const { return data < other.data; }
    
    String to_string() const { return String(data.begin(), data.end()); }
    bool empty() const { return data.empty(); }
    Size size() const { return data.size(); }
};

// =============================================================================
// Data Record Type
// =============================================================================

struct DataRecord {
    ByteBuffer data;
    UInt32 length{0};
    UInt64 rba{0};          // Relative Byte Address
    UInt32 slot{0};         // Slot number for RRDS
    RecordKey key;          // Key for KSDS
    SystemTimePoint timestamp;
    UInt32 flags{0};        // Record flags
    
    DataRecord() : timestamp(SystemClock::now()) {}
    explicit DataRecord(Size size) : data(size), length(static_cast<UInt32>(size)), timestamp(SystemClock::now()) {}
    DataRecord(const Byte* ptr, Size size) 
        : data(ptr, ptr + size), length(static_cast<UInt32>(size)), timestamp(SystemClock::now()) {}
    
    bool empty() const { return data.empty(); }
    Size size() const { return data.size(); }
};

// =============================================================================
// Performance Metrics
// =============================================================================

struct PerformanceMetrics {
    AtomicUInt64 total_operations{0};
    AtomicUInt64 successful_operations{0};
    AtomicUInt64 failed_operations{0};
    AtomicInt64 min_response_time_ns{INT64_MAX};
    AtomicInt64 max_response_time_ns{0};
    AtomicInt64 total_response_time_ns{0};
    AtomicUInt64 bytes_read{0};
    AtomicUInt64 bytes_written{0};
    SystemTimePoint start_time;
    
    PerformanceMetrics() : start_time(SystemClock::now()) {}
    
    void record_operation(bool success, Int64 response_time_ns) {
        total_operations.fetch_add(1, std::memory_order_relaxed);
        if (success) {
            successful_operations.fetch_add(1, std::memory_order_relaxed);
        } else {
            failed_operations.fetch_add(1, std::memory_order_relaxed);
        }
        total_response_time_ns.fetch_add(response_time_ns, std::memory_order_relaxed);
        
        // Update min
        Int64 current_min = min_response_time_ns.load(std::memory_order_relaxed);
        while (response_time_ns < current_min && 
               !min_response_time_ns.compare_exchange_weak(current_min, response_time_ns,
                   std::memory_order_relaxed, std::memory_order_relaxed)) {}
        
        // Update max
        Int64 current_max = max_response_time_ns.load(std::memory_order_relaxed);
        while (response_time_ns > current_max && 
               !max_response_time_ns.compare_exchange_weak(current_max, response_time_ns,
                   std::memory_order_relaxed, std::memory_order_relaxed)) {}
    }
    
    void record_io(UInt64 read_bytes, UInt64 write_bytes) {
        bytes_read.fetch_add(read_bytes, std::memory_order_relaxed);
        bytes_written.fetch_add(write_bytes, std::memory_order_relaxed);
    }
    
    void reset() {
        total_operations.store(0, std::memory_order_relaxed);
        successful_operations.store(0, std::memory_order_relaxed);
        failed_operations.store(0, std::memory_order_relaxed);
        min_response_time_ns.store(INT64_MAX, std::memory_order_relaxed);
        max_response_time_ns.store(0, std::memory_order_relaxed);
        total_response_time_ns.store(0, std::memory_order_relaxed);
        bytes_read.store(0, std::memory_order_relaxed);
        bytes_written.store(0, std::memory_order_relaxed);
        start_time = SystemClock::now();
    }
    
    double get_success_rate() const {
        UInt64 total = total_operations.load(std::memory_order_relaxed);
        if (total == 0) return 100.0;
        UInt64 success = successful_operations.load(std::memory_order_relaxed);
        return static_cast<double>(success) / static_cast<double>(total) * 100.0;
    }
    
    double get_average_response_time_ms() const {
        UInt64 total = total_operations.load(std::memory_order_relaxed);
        if (total == 0) return 0.0;
        Int64 total_time = total_response_time_ns.load(std::memory_order_relaxed);
        return static_cast<double>(total_time) / static_cast<double>(total) / 1000000.0;
    }
    
    double get_operations_per_second() const {
        auto elapsed = std::chrono::duration_cast<Seconds>(SystemClock::now() - start_time).count();
        if (elapsed == 0) return 0.0;
        UInt64 total = total_operations.load(std::memory_order_relaxed);
        return static_cast<double>(total) / static_cast<double>(elapsed);
    }
    
    double get_throughput_mb_per_sec() const {
        auto elapsed = std::chrono::duration_cast<Seconds>(SystemClock::now() - start_time).count();
        if (elapsed == 0) return 0.0;
        UInt64 total_bytes = bytes_read.load() + bytes_written.load();
        return static_cast<double>(total_bytes) / (1024.0 * 1024.0) / static_cast<double>(elapsed);
    }
    
    String to_string() const {
        return std::format(
            "Operations: {} (Success: {:.2f}%), Avg Response: {:.3f}ms, Ops/sec: {:.2f}, Throughput: {:.2f} MB/s",
            total_operations.load(), get_success_rate(), 
            get_average_response_time_ms(), get_operations_per_second(),
            get_throughput_mb_per_sec());
    }
};

// =============================================================================
// Utility Functions
// =============================================================================

inline String bytes_to_hex(const ByteBuffer& bytes) {
    static const char hex_chars[] = "0123456789ABCDEF";
    String result;
    result.reserve(bytes.size() * 2);
    for (Byte b : bytes) {
        result += hex_chars[(b >> 4) & 0x0F];
        result += hex_chars[b & 0x0F];
    }
    return result;
}

inline ByteBuffer hex_to_bytes(StringView hex) {
    ByteBuffer result;
    result.reserve(hex.size() / 2);
    for (Size i = 0; i + 1 < hex.size(); i += 2) {
        char c1 = static_cast<char>(std::toupper(static_cast<unsigned char>(hex[i])));
        char c2 = static_cast<char>(std::toupper(static_cast<unsigned char>(hex[i+1])));
        Byte high = static_cast<Byte>((c1 >= 'A') ? (c1 - 'A' + 10) : (c1 - '0'));
        Byte low = static_cast<Byte>((c2 >= 'A') ? (c2 - 'A' + 10) : (c2 - '0'));
        result.push_back(static_cast<Byte>((high << 4) | low));
    }
    return result;
}

// =============================================================================
// Scoped Timer for Performance Measurement
// =============================================================================

class ScopedTimer {
private:
    TimePoint start_;
    Int64* result_ns_;
    
public:
    explicit ScopedTimer(Int64* result_ns) : start_(Clock::now()), result_ns_(result_ns) {}
    ~ScopedTimer() {
        if (result_ns_) {
            *result_ns_ = std::chrono::duration_cast<Duration>(Clock::now() - start_).count();
        }
    }
    
    Int64 elapsed_ns() const {
        return std::chrono::duration_cast<Duration>(Clock::now() - start_).count();
    }
    
    double elapsed_ms() const {
        return static_cast<double>(elapsed_ns()) / 1000000.0;
    }
};

// =============================================================================
// RAII Scope Guard
// =============================================================================

template<typename Func>
class ScopeGuard {
private:
    Func func_;
    bool active_{true};
    
public:
    explicit ScopeGuard(Func&& func) : func_(std::forward<Func>(func)) {}
    ~ScopeGuard() { if (active_) func_(); }
    
    void dismiss() { active_ = false; }
    
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& other) noexcept : func_(std::move(other.func_)), active_(other.active_) {
        other.active_ = false;
    }
};

template<typename Func>
ScopeGuard<Func> make_scope_guard(Func&& func) {
    return ScopeGuard<Func>(std::forward<Func>(func));
}

} // namespace ims
