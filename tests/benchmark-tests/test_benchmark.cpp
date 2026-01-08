// =============================================================================
// IBM IMS Emulation Enterprise - Benchmark Tests
// Version: 3.6.2
// =============================================================================

#include "test_framework.hpp"
#include "ims/common/types.hpp"
#include "ims/common/benchmark.hpp"
#include "ims/common/cache.hpp"
#include "ims/common/string_utils.hpp"
#include "ims/catalog/catalog_types.hpp"

#include <iostream>
#include <random>

using namespace ims;
using namespace ims::test;
using namespace ims::benchmark;

// =============================================================================
// Benchmark Tests
// =============================================================================

bool test_simple_benchmark() {
    int counter = 0;
    auto result = Benchmark("Counter Increment")
        .warmup(10)
        .iterations(1000)
        .run([&]() {
            counter++;
        });
    
    TEST_ASSERT_EQ(static_cast<UInt64>(1000), result.iterations);
    TEST_ASSERT(result.ops_per_second() > 0);
    TEST_ASSERT(result.avg_time_ns() > 0);
    
    return true;
}

bool test_benchmark_statistics() {
    auto result = Benchmark("Statistics Test")
        .warmup(10)
        .iterations(100)
        .samples(true)
        .run([]() {
            volatile int x = 0;
            for (int i = 0; i < 100; ++i) {
                x += i;
            }
        });
    
    TEST_ASSERT_EQ(static_cast<Size>(100), result.samples.size());
    TEST_ASSERT(result.median_time().count() > 0);
    TEST_ASSERT(result.percentile(95).count() >= result.median_time().count());
    TEST_ASSERT(result.percentile(99).count() >= result.percentile(95).count());
    
    return true;
}

bool test_benchmark_suite() {
    BenchmarkSuite suite("Basic Operations");
    
    suite.run("Addition", []() {
        volatile int x = 1 + 1;
        (void)x;
    }, 100);
    
    suite.run("Multiplication", []() {
        volatile int x = 2 * 2;
        (void)x;
    }, 100);
    
    TEST_ASSERT_EQ(static_cast<Size>(2), suite.results().size());
    
    auto csv = suite.to_csv();
    TEST_ASSERT(csv.find("Addition") != String::npos);
    TEST_ASSERT(csv.find("Multiplication") != String::npos);
    
    return true;
}

bool test_throughput_benchmark() {
    ByteBuffer buffer(1024, 0x42);
    
    auto result = throughput_bench("Buffer Copy", 1024, 1000, [&]() {
        ByteBuffer copy = buffer;
        (void)copy;
    });
    
    TEST_ASSERT_EQ(static_cast<UInt64>(1024 * 1000), result.total_bytes);
    TEST_ASSERT(result.mb_per_second() > 0);
    
    return true;
}

bool test_cache_benchmark() {
    LruCache<String, int> cache(1000);
    
    // Pre-fill cache
    for (int i = 0; i < 500; ++i) {
        cache.put("key" + std::to_string(i), i);
    }
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 499);
    
    auto result = Benchmark("LRU Cache Get")
        .warmup(100)
        .iterations(10000)
        .run([&]() {
            auto key = "key" + std::to_string(dist(rng));
            auto value = cache.get(key);
            (void)value;
        });
    
    TEST_ASSERT(result.ops_per_second() > 1000);  // Should be very fast
    
    const auto& stats = cache.statistics();
    TEST_ASSERT(stats.hits.load() > 0);
    
    return true;
}

bool test_string_utils_benchmark() {
    String test_string = "  Hello World  ";
    
    auto result = Benchmark("String Trim")
        .warmup(100)
        .iterations(10000)
        .run([&]() {
            auto trimmed = strings::trim(test_string);
            (void)trimmed;
        });
    
    TEST_ASSERT(result.ops_per_second() > 10000);
    
    return true;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    TestRunner runner("Benchmark Tests");
    
    runner.add_test("Simple Benchmark", test_simple_benchmark);
    runner.add_test("Benchmark Statistics", test_benchmark_statistics);
    runner.add_test("Benchmark Suite", test_benchmark_suite);
    runner.add_test("Throughput Benchmark", test_throughput_benchmark);
    runner.add_test("Cache Benchmark", test_cache_benchmark);
    runner.add_test("String Utils Benchmark", test_string_utils_benchmark);
    
    return runner.run();
}
