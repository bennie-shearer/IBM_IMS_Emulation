// =============================================================================
// IBM IMS Emulation Enterprise - Common Library Tests
// Version: 3.6.2
// =============================================================================

#include "../test_framework.hpp"
#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/common/platform.hpp"

using namespace ims;
using namespace ims::test;

// =============================================================================
// Type Tests
// =============================================================================

bool test_record_key_construction() {
    // Default construction
    RecordKey key1;
    TEST_ASSERT_TRUE(key1.empty());
    TEST_ASSERT_EQ(0u, key1.length);
    
    // Size construction
    RecordKey key2(16);
    TEST_ASSERT_EQ(16u, key2.size());
    TEST_ASSERT_EQ(16u, key2.length);
    
    // String construction
    RecordKey key3("TEST_KEY_001");
    TEST_ASSERT_EQ(12u, key3.size());
    TEST_ASSERT_EQ("TEST_KEY_001", key3.to_string());
    
    // Byte array construction
    Byte bytes[] = {0x41, 0x42, 0x43};  // "ABC"
    RecordKey key4(bytes, 3);
    TEST_ASSERT_EQ(3u, key4.size());
    TEST_ASSERT_EQ("ABC", key4.to_string());
    
    return true;
}

bool test_record_key_comparison() {
    RecordKey key1("AAA");
    RecordKey key2("AAA");
    RecordKey key3("BBB");
    
    TEST_ASSERT_TRUE(key1 == key2);
    TEST_ASSERT_FALSE(key1 == key3);
    TEST_ASSERT_TRUE(key1 < key3);
    TEST_ASSERT_FALSE(key3 < key1);
    
    return true;
}

bool test_byte_buffer_hex_conversion() {
    ByteBuffer bytes = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
    String hex = bytes_to_hex(bytes);
    TEST_ASSERT_EQ("48656C6C6F", hex);
    
    ByteBuffer converted = hex_to_bytes("48656C6C6F");
    TEST_ASSERT_EQ(bytes.size(), converted.size());
    for (size_t i = 0; i < bytes.size(); ++i) {
        TEST_ASSERT_EQ(bytes[i], converted[i]);
    }
    
    return true;
}

bool test_performance_metrics() {
    PerformanceMetrics metrics;
    
    TEST_ASSERT_EQ(0u, metrics.total_operations.load());
    TEST_ASSERT_EQ(100.0, metrics.get_success_rate());
    
    metrics.record_operation(true, 1000000);   // 1ms success
    metrics.record_operation(true, 2000000);   // 2ms success
    metrics.record_operation(false, 500000);   // 0.5ms failure
    
    TEST_ASSERT_EQ(3u, metrics.total_operations.load());
    TEST_ASSERT_EQ(2u, metrics.successful_operations.load());
    TEST_ASSERT_EQ(1u, metrics.failed_operations.load());
    
    double success_rate = metrics.get_success_rate();
    TEST_ASSERT_TRUE(success_rate > 66.0 && success_rate < 67.0);
    
    return true;
}

// =============================================================================
// Error Handling Tests
// =============================================================================

bool test_error_result_success() {
    ErrorResult<int> result(42);
    
    TEST_ASSERT_TRUE(result.has_value());
    TEST_ASSERT_FALSE(result.is_error());
    TEST_ASSERT_EQ(42, result.value());
    TEST_ASSERT_EQ(42, result.value_or(0));
    
    return true;
}

bool test_error_result_error() {
    ErrorResult<int> result = make_error<int>("Something went wrong");
    
    TEST_ASSERT_FALSE(result.has_value());
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQ("Something went wrong", result.error());
    TEST_ASSERT_EQ(0, result.value_or(0));
    
    return true;
}

bool test_error_result_string_specialization() {
    // Test that ErrorResult<String> works correctly (this was a known bug)
    
    // Success case
    ErrorResult<String> success = ErrorResult<String>::make_success("Success message");
    TEST_ASSERT_TRUE(success.has_value());
    TEST_ASSERT_EQ("Success message", success.value());
    
    // Error case
    ErrorResult<String> error = ErrorResult<String>::make_error("Error message");
    TEST_ASSERT_FALSE(error.has_value());
    TEST_ASSERT_TRUE(error.is_error());
    TEST_ASSERT_EQ("Error message", error.error());
    
    return true;
}

bool test_error_result_void() {
    ErrorResult<void> success;
    TEST_ASSERT_TRUE(success.has_value());
    TEST_ASSERT_FALSE(success.is_error());
    
    ErrorResult<void> error = make_error<void>("Failed");
    TEST_ASSERT_FALSE(error.has_value());
    TEST_ASSERT_TRUE(error.is_error());
    TEST_ASSERT_EQ("Failed", error.error());
    
    return true;
}

bool test_error_result_map() {
    ErrorResult<int> result(10);
    
    auto mapped = result.map([](int x) { return x * 2; });
    TEST_ASSERT_TRUE(mapped.has_value());
    TEST_ASSERT_EQ(20, mapped.value());
    
    ErrorResult<int> error_result = make_error<int>("Error");
    auto mapped_error = error_result.map([](int x) { return x * 2; });
    TEST_ASSERT_TRUE(mapped_error.is_error());
    
    return true;
}

bool test_error_info() {
    ErrorInfo info(ImsErrorCode::RECORD_NOT_FOUND, "Record with key 'TEST' not found");
    
    TEST_ASSERT_TRUE(info.is_error());
    TEST_ASSERT_FALSE(info.is_success());
    TEST_ASSERT_EQ(ImsErrorCode::RECORD_NOT_FOUND, info.code);
    TEST_ASSERT_TRUE(info.message.find("not found") != String::npos);
    
    return true;
}

bool test_error_reporter() {
    ErrorReporter reporter;
    
    TEST_ASSERT_EQ(0u, reporter.error_count());
    
    reporter.report(ImsErrorCode::FILE_NOT_FOUND, "File not found");
    reporter.report(ImsErrorCode::PERMISSION_DENIED, "Access denied");
    
    TEST_ASSERT_EQ(2u, reporter.error_count());
    TEST_ASSERT_EQ(2u, reporter.total_error_count());
    
    auto errors = reporter.get_errors();
    TEST_ASSERT_EQ(2u, errors.size());
    
    reporter.clear();
    TEST_ASSERT_EQ(0u, reporter.error_count());
    
    return true;
}

// =============================================================================
// Platform Tests
// =============================================================================

bool test_platform_info() {
    auto info = PlatformInfo::get_current();
    
    TEST_ASSERT_FALSE(info.os_name.empty());
    TEST_ASSERT_FALSE(info.arch_name.empty());
    TEST_ASSERT_FALSE(info.compiler_name.empty());
    TEST_ASSERT_TRUE(info.cpu_count > 0);
    TEST_ASSERT_TRUE(info.page_size > 0);
    
    return true;
}

bool test_platform_utilities() {
    // Test timestamp
    UInt64 ts1 = platform::get_timestamp_ns();
    UInt64 ts2 = platform::get_timestamp_ns();
    TEST_ASSERT_TRUE(ts2 >= ts1);
    
    // Test process/thread ID
    UInt32 pid = platform::get_process_id();
    TEST_ASSERT_TRUE(pid > 0);
    
    UInt64 tid = platform::get_thread_id();
    TEST_ASSERT_TRUE(tid > 0);
    
    // Test page size
    Size page_size = platform::get_page_size();
    TEST_ASSERT_TRUE(page_size >= 4096);
    
    // Test CPU count
    UInt32 cpu_count = platform::get_cpu_count();
    TEST_ASSERT_TRUE(cpu_count > 0);
    
    // Test directories
    Path home = platform::get_home_directory();
    TEST_ASSERT_FALSE(home.empty());
    
    Path temp = platform::get_temp_directory();
    TEST_ASSERT_FALSE(temp.empty());
    
    Path cwd = platform::get_current_directory();
    TEST_ASSERT_FALSE(cwd.empty());
    
    return true;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    TestRunner runner("Common Library Tests");
    
    // Type tests
    runner.add_test("RecordKey Construction", test_record_key_construction);
    runner.add_test("RecordKey Comparison", test_record_key_comparison);
    runner.add_test("ByteBuffer Hex Conversion", test_byte_buffer_hex_conversion);
    runner.add_test("PerformanceMetrics", test_performance_metrics);
    
    // Error handling tests
    runner.add_test("ErrorResult Success", test_error_result_success);
    runner.add_test("ErrorResult Error", test_error_result_error);
    runner.add_test("ErrorResult<String> Specialization", test_error_result_string_specialization);
    runner.add_test("ErrorResult<void>", test_error_result_void);
    runner.add_test("ErrorResult Map", test_error_result_map);
    runner.add_test("ErrorInfo", test_error_info);
    runner.add_test("ErrorReporter", test_error_reporter);
    
    // Platform tests
    runner.add_test("PlatformInfo", test_platform_info);
    runner.add_test("Platform Utilities", test_platform_utilities);
    
    return runner.run();
}
