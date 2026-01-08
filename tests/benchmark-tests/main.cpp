// =============================================================================
// IBM IMS Emulation Enterprise - Benchmark Tests
// Version: 3.6.2
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/benchmark.hpp"
#include "ims/common/cache.hpp"
#include "ims/common/string_utils.hpp"
#include "ims/common/validation.hpp"
#include "ims/common/time_utils.hpp"
#include <iostream>
#include <random>

using namespace ims;
using namespace ims::benchmark;

// Helper macro to prevent unused variable warnings while keeping volatile
#define BENCHMARK_USE(x) do { (void)(x); } while(0)

// =============================================================================
// String Operations Benchmarks
// =============================================================================

void benchmark_string_operations(BenchmarkSuite& suite) {
    std::cout << "Running string operation benchmarks...\n";
    
    // Pad right benchmark
    suite.run("strings::pad_right", []() {
        auto result = strings::pad_right("TEST", 80);
        BENCHMARK_USE(result);
    }, 100000);
    
    // Pad left benchmark
    suite.run("strings::pad_left", []() {
        auto result = strings::pad_left("12345", 10, '0');
        BENCHMARK_USE(result);
    }, 100000);
    
    // Split benchmark
    String test_dsn = "SYS1.PROD.DATA.FILE01";
    suite.run("strings::split", [&test_dsn]() {
        auto result = strings::split(test_dsn, '.');
        BENCHMARK_USE(result);
    }, 100000);
    
    // Trim benchmark
    String padded = "   HELLO WORLD   ";
    suite.run("strings::trim", [&padded]() {
        auto result = strings::trim(padded);
        BENCHMARK_USE(result);
    }, 100000);
    
    // To upper benchmark
    String mixed = "Hello World 123";
    suite.run("strings::to_upper", [&mixed]() {
        auto result = strings::to_upper(mixed);
        BENCHMARK_USE(result);
    }, 100000);
}

// =============================================================================
// Validation Benchmarks
// =============================================================================

void benchmark_validation(BenchmarkSuite& suite) {
    std::cout << "Running validation benchmarks...\n";
    
    // Dataset name validation
    String valid_dsn = "SYS1.PROD.PAYROLL.DATA";
    suite.run("DatasetNameValidator::validate", [&valid_dsn]() {
        auto result = validation::DatasetNameValidator::validate(valid_dsn);
        BENCHMARK_USE(result);
    }, 50000);
    
    // Member name validation
    String valid_member = "PAYRPT01";
    suite.run("MemberNameValidator::validate", [&valid_member]() {
        auto result = validation::MemberNameValidator::validate(valid_member);
        BENCHMARK_USE(result);
    }, 100000);
    
    // Volume serial validation
    String valid_volser = "VOL001";
    suite.run("VolumeSerialValidator::validate", [&valid_volser]() {
        auto result = validation::VolumeSerialValidator::validate(valid_volser);
        BENCHMARK_USE(result);
    }, 100000);
}

// =============================================================================
// Cache Benchmarks
// =============================================================================

void benchmark_cache(BenchmarkSuite& suite) {
    std::cout << "Running cache benchmarks...\n";
    
    LruCache<String, String> cache(10000);
    
    // Pre-populate cache
    for (int i = 0; i < 5000; ++i) {
        cache.put("KEY" + std::to_string(i), "VALUE" + std::to_string(i));
    }
    
    // Cache hit benchmark
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 4999);
    
    suite.run("LruCache::get (hit)", [&cache, &rng, &dist]() {
        String key = "KEY" + std::to_string(dist(rng));
        auto result = cache.get(key);
        BENCHMARK_USE(result);
    }, 100000);
    
    // Cache miss benchmark
    suite.run("LruCache::get (miss)", [&cache]() {
        auto result = cache.get("NONEXISTENT");
        BENCHMARK_USE(result);
    }, 100000);
    
    // Cache put benchmark
    int counter = 0;
    suite.run("LruCache::put", [&cache, &counter]() {
        cache.put("NEW" + std::to_string(counter++), "VALUE");
    }, 50000);
}

// =============================================================================
// Time Utilities Benchmarks
// =============================================================================

void benchmark_time_utils(BenchmarkSuite& suite) {
    std::cout << "Running time utility benchmarks...\n";
    
    // STCK creation
    suite.run("Stck::now", []() {
        auto stck = time::Stck::now();
        BENCHMARK_USE(stck);
    }, 100000);
    
    // Julian date creation
    suite.run("JulianDate::today", []() {
        auto julian = time::JulianDate::today();
        BENCHMARK_USE(julian);
    }, 100000);
    
    // Timestamp formatting
    auto now = SystemClock::now();
    suite.run("TimestampFormatter::format_iso8601", [&now]() {
        auto result = time::TimestampFormatter::format_iso8601(now);
        BENCHMARK_USE(result);
    }, 50000);
    
    suite.run("TimestampFormatter::format_mainframe", [&now]() {
        auto result = time::TimestampFormatter::format_mainframe(now);
        BENCHMARK_USE(result);
    }, 50000);
}

// =============================================================================
// Memory Allocation Benchmarks
// =============================================================================

void benchmark_memory(BenchmarkSuite& suite) {
    std::cout << "Running memory benchmarks...\n";
    
    // Small buffer allocation
    suite.run("ByteBuffer(100) allocation", []() {
        ByteBuffer buffer(100);
        auto size = buffer.size();
        BENCHMARK_USE(size);
    }, 100000);
    
    // Medium buffer allocation
    suite.run("ByteBuffer(4096) allocation", []() {
        ByteBuffer buffer(4096);
        auto size = buffer.size();
        BENCHMARK_USE(size);
    }, 50000);
    
    // Large buffer allocation
    suite.run("ByteBuffer(65536) allocation", []() {
        ByteBuffer buffer(65536);
        auto size = buffer.size();
        BENCHMARK_USE(size);
    }, 10000);
    
    // Record creation
    suite.run("DataRecord creation", []() {
        DataRecord record(1000);
        auto size = record.size();
        BENCHMARK_USE(size);
    }, 50000);
}

// =============================================================================
// Hex Conversion Benchmarks
// =============================================================================

void benchmark_hex_conversion(BenchmarkSuite& suite) {
    std::cout << "Running hex conversion benchmarks...\n";
    
    // Create test data
    ByteBuffer test_data(256);
    for (Size i = 0; i < 256; ++i) {
        test_data[i] = static_cast<Byte>(i);
    }
    
    suite.run("bytes_to_hex (256 bytes)", [&test_data]() {
        auto result = bytes_to_hex(test_data);
        BENCHMARK_USE(result);
    }, 50000);
    
    String hex_str = bytes_to_hex(test_data);
    suite.run("hex_to_bytes (512 chars)", [&hex_str]() {
        auto result = hex_to_bytes(hex_str);
        BENCHMARK_USE(result);
    }, 50000);
}

// =============================================================================
// Packed Decimal Benchmarks
// =============================================================================

void benchmark_packed_decimal(BenchmarkSuite& suite) {
    std::cout << "Running packed decimal benchmarks...\n";
    
    suite.run("format_packed (8 bytes)", []() {
        auto result = strings::format_packed(123456789012345LL, 8);
        BENCHMARK_USE(result);
    }, 100000);
    
    ByteBuffer packed = strings::format_packed(123456789012345LL, 8);
    suite.run("parse_packed (8 bytes)", [&packed]() {
        auto result = strings::parse_packed(packed);
        BENCHMARK_USE(result);
    }, 100000);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "IBM IMS Emulation Enterprise v3.6.2\n";
    std::cout << "Performance Benchmark Suite\n";
    std::cout << "========================================\n\n";
    
    BenchmarkSuite suite("IMS Core Operations");
    
    benchmark_string_operations(suite);
    benchmark_validation(suite);
    benchmark_cache(suite);
    benchmark_time_utils(suite);
    benchmark_memory(suite);
    benchmark_hex_conversion(suite);
    benchmark_packed_decimal(suite);
    
    std::cout << "\n";
    std::cout << suite.to_string();
    
    // Save CSV report
    std::cout << "\n========================================\n";
    std::cout << "CSV Report:\n";
    std::cout << "========================================\n";
    std::cout << suite.to_csv();
    
    return 0;
}
