/**
 * @file test_new_modules.cpp
 * @brief Tests for v3.5.0, v3.6.0, and v3.6.3 modules
 * @version 3.6.3
 *
 * Tests for:
 * - Configuration persistence
 * - Connection pool
 * - Metrics collector
 * - Command processor
 * - State machine
 * - Compression
 * - Event bus
 * - Object pool (v3.6.3)
 * - Async queue (v3.6.3)
 * - Task scheduler (v3.6.3)
 * - Data mapper (v3.6.3)
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#include <ims/common/config_persistence.hpp>
#include <ims/common/connection_pool.hpp>
#include <ims/common/metrics_collector.hpp>
#include <ims/common/command_processor.hpp>
#include <ims/common/state_machine.hpp>
#include <ims/common/compression.hpp>
#include <ims/common/event_bus.hpp>
#include <ims/common/object_pool.hpp>
#include <ims/common/async_queue.hpp>
#include <ims/common/task_scheduler.hpp>
#include <ims/common/data_mapper.hpp>

#include <iostream>
#include <cassert>
#include <sstream>
#include <cmath>

using namespace ims;
using namespace ims::common;

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRunner_##name { \
        TestRunner_##name() { \
            std::cout << "Running: " << #name << "... "; \
            try { \
                test_##name(); \
                std::cout << "PASSED" << std::endl; \
                ++tests_passed; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << std::endl; \
                ++tests_failed; \
            } catch (...) { \
                std::cout << "FAILED: Unknown exception" << std::endl; \
                ++tests_failed; \
            } \
        } \
    } test_runner_##name; \
    void test_##name()

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("Assertion failed: " #expr)

#define ASSERT_FALSE(expr) \
    if (expr) throw std::runtime_error("Assertion failed: !" #expr)

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b)

#define ASSERT_NE(a, b) \
    if ((a) == (b)) throw std::runtime_error("Assertion failed: " #a " != " #b)

#define ASSERT_GT(a, b) \
    if (!((a) > (b))) throw std::runtime_error("Assertion failed: " #a " > " #b)

#define ASSERT_LT(a, b) \
    if (!((a) < (b))) throw std::runtime_error("Assertion failed: " #a " < " #b)

// =============================================================================
// ConfigValue Tests
// =============================================================================

TEST(config_value_types) {
    ConfigValue null_val;
    ASSERT_TRUE(null_val.is_null());
    
    ConfigValue str_val("hello");
    ASSERT_TRUE(str_val.is_string());
    ASSERT_EQ(str_val.as_string(), "hello");
    
    ConfigValue int_val(42);
    ASSERT_TRUE(int_val.is_integer());
    ASSERT_EQ(int_val.as_integer(), 42);
    
    ConfigValue dbl_val(3.14);
    ASSERT_TRUE(dbl_val.is_double());
    ASSERT_TRUE(std::abs(dbl_val.as_double() - 3.14) < 0.001);
    
    ConfigValue bool_val = ConfigValue::from_bool(true);
    ASSERT_TRUE(bool_val.is_boolean());
    ASSERT_TRUE(bool_val.as_boolean());
}

TEST(config_value_conversions) {
    ConfigValue int_val(123);
    ASSERT_EQ(int_val.as_string(), "123");
    ASSERT_TRUE(std::abs(int_val.as_double() - 123.0) < 0.001);
    
    ConfigValue str_val("456");
    ASSERT_EQ(str_val.as_integer(), 456);
    
    ConfigValue bool_str("true");
    ASSERT_TRUE(bool_str.as_boolean());
}

TEST(json_parser) {
    String json = R"({"name": "test", "value": 42, "enabled": true})";
    
    JsonParser parser;
    auto config = parser.parse(json);
    
    ASSERT_TRUE(config.is_object());
    
    auto* name = config.get("name");
    ASSERT_TRUE(name != nullptr);
    ASSERT_EQ(name->as_string(), "test");
    
    auto* value = config.get("value");
    ASSERT_TRUE(value != nullptr);
    ASSERT_EQ(value->as_integer(), 42);
    
    auto* enabled = config.get("enabled");
    ASSERT_TRUE(enabled != nullptr);
    ASSERT_TRUE(enabled->as_boolean());
}

TEST(json_writer) {
    ConfigValue config;
    config.set_property("name", ConfigValue("test"));
    config.set_property("count", ConfigValue(10));
    
    JsonWriter writer(true, 2);
    String json = writer.write(config);
    
    ASSERT_TRUE(json.find("\"name\"") != String::npos);
    ASSERT_TRUE(json.find("\"test\"") != String::npos);
    ASSERT_TRUE(json.find("\"count\"") != String::npos);
}

// =============================================================================
// Metrics Collector Tests
// =============================================================================

TEST(metrics_counter) {
    MetricsRegistry registry("test");
    
    auto& counter = registry.counter("requests", "Total requests");
    counter.increment();
    counter.increment(5);
    
    ASSERT_EQ(static_cast<int>(counter.value()), 6);
}

TEST(metrics_gauge) {
    MetricsRegistry registry("test");
    
    auto& gauge = registry.gauge("temperature");
    gauge.set(25.5);
    ASSERT_TRUE(std::abs(gauge.value() - 25.5) < 0.001);
    
    gauge.add(2.0);
    ASSERT_TRUE(std::abs(gauge.value() - 27.5) < 0.001);
}

TEST(metrics_histogram) {
    Histogram hist;
    
    for (int i = 1; i <= 100; ++i) {
        hist.record(static_cast<double>(i));
    }
    
    auto summary = hist.summary();
    ASSERT_EQ(summary.count, 100u);
    ASSERT_TRUE(std::abs(summary.min - 1.0) < 0.001);
    ASSERT_TRUE(std::abs(summary.max - 100.0) < 0.001);
    ASSERT_TRUE(std::abs(summary.mean - 50.5) < 0.1);
}

// =============================================================================
// Command Processor Tests
// =============================================================================

TEST(command_tokenize) {
    auto tokens = CommandParser::tokenize("hello world");
    ASSERT_EQ(tokens.size(), 2u);
    ASSERT_EQ(tokens[0], "hello");
    ASSERT_EQ(tokens[1], "world");
    
    tokens = CommandParser::tokenize("\"hello world\"");
    ASSERT_EQ(tokens.size(), 1u);
    ASSERT_EQ(tokens[0], "hello world");
}

TEST(command_parse) {
    auto parsed = CommandParser::parse("cmd arg1 arg2 --flag --opt=value");
    
    ASSERT_EQ(parsed.name, "cmd");
    ASSERT_EQ(parsed.args.size(), 2u);
    ASSERT_EQ(parsed.args[0], "arg1");
    ASSERT_EQ(parsed.args[1], "arg2");
    ASSERT_TRUE(parsed.flags.find("flag") != parsed.flags.end());
    ASSERT_EQ(parsed.options["opt"], "value");
}

TEST(command_processor_builtin) {
    std::ostringstream out, err;
    CommandProcessor proc(out, err);
    
    auto result = proc.execute("echo hello world");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(out.str().find("hello world") != String::npos);
}

// =============================================================================
// State Machine Tests
// =============================================================================

enum class TrafficLight { Red, Yellow, Green };

TEST(state_machine_basic) {
    StateMachine<TrafficLight, String> sm;
    
    sm.add_state(TrafficLight::Red, "Red");
    sm.add_state(TrafficLight::Yellow, "Yellow");
    sm.add_state(TrafficLight::Green, "Green");
    
    sm.add_transition(TrafficLight::Red, TrafficLight::Green,
        [](const String& e) { return e == "go"; });
    sm.add_transition(TrafficLight::Green, TrafficLight::Yellow,
        [](const String& e) { return e == "slow"; });
    sm.add_transition(TrafficLight::Yellow, TrafficLight::Red,
        [](const String& e) { return e == "stop"; });
    
    sm.set_initial_state(TrafficLight::Red);
    ASSERT_TRUE(sm.start());
    
    ASSERT_TRUE(sm.is_in_state(TrafficLight::Red));
    
    auto result = sm.process_event("go");
    ASSERT_EQ(result, TransitionResult::Success);
    ASSERT_TRUE(sm.is_in_state(TrafficLight::Green));
    
    result = sm.process_event("slow");
    ASSERT_EQ(result, TransitionResult::Success);
    ASSERT_TRUE(sm.is_in_state(TrafficLight::Yellow));
}

TEST(state_machine_guard) {
    StateMachine<int, int, int> sm;
    
    sm.add_state(0, "Start");
    sm.add_state(1, "End");
    
    // Only allow transition if context > 5
    sm.add_transition(0, 1, nullptr,
        [](const int& ctx, const int&) { return ctx > 5; });
    
    sm.set_initial_state(0);
    sm.start();
    
    sm.context() = 3;
    auto result = sm.process_event(1);
    ASSERT_EQ(result, TransitionResult::GuardFailed);
    
    sm.context() = 10;
    result = sm.process_event(1);
    ASSERT_EQ(result, TransitionResult::Success);
}

// =============================================================================
// Compression Tests
// =============================================================================

TEST(rle_compression) {
    RleCompressor rle;
    
    // Test data with runs
    ByteBuffer data = {'A', 'A', 'A', 'A', 'A', 'B', 'C', 'C', 'C', 'C'};
    
    auto compressed = rle.compress(data);
    ASSERT_TRUE(compressed.success);
    ASSERT_LT(compressed.compressed_size, compressed.original_size);
    
    auto decompressed = rle.decompress(compressed.data);
    ASSERT_TRUE(decompressed.success);
    ASSERT_EQ(decompressed.data.size(), data.size());
    
    for (Size i = 0; i < data.size(); ++i) {
        ASSERT_EQ(decompressed.data[i], data[i]);
    }
}

TEST(dictionary_compression) {
    DictionaryCompressor dict;
    
    // Test data with repeated patterns
    String pattern = "ABCDEFGH";
    ByteBuffer data;
    for (int i = 0; i < 10; ++i) {
        for (char c : pattern) {
            data.push_back(static_cast<Byte>(c));
        }
    }
    
    auto compressed = dict.compress(data);
    ASSERT_TRUE(compressed.success);
    
    auto decompressed = dict.decompress(compressed.data);
    ASSERT_TRUE(decompressed.success);
    ASSERT_EQ(decompressed.data.size(), data.size());
}

TEST(compression_algorithm_recommend) {
    // Small data - no compression
    ByteBuffer small_data(10, 'A');
    ASSERT_EQ(recommend_compression(small_data), CompressionAlgorithm::None);
    
    // Data with runs - RLE
    ByteBuffer run_data(200, 'A');
    ASSERT_EQ(recommend_compression(run_data), CompressionAlgorithm::RLE);
}

// =============================================================================
// Event Bus Tests
// =============================================================================

struct TestEvent {
    String message;
    int value;
};

TEST(event_bus_basic) {
    EventBus bus;
    
    String received_message;
    int received_value = 0;
    
    auto handle = bus.subscribe<TestEvent>([&](Event<TestEvent>& e) {
        received_message = e.data.message;
        received_value = e.data.value;
    });
    
    bus.publish(TestEvent{"hello", 42});
    
    ASSERT_EQ(received_message, "hello");
    ASSERT_EQ(received_value, 42);
}

TEST(event_bus_priority) {
    EventBus bus;
    
    Vector<int> order;
    
    bus.subscribe<int>([&](Event<int>&) { order.push_back(1); }, 1);
    bus.subscribe<int>([&](Event<int>&) { order.push_back(2); }, 2);
    bus.subscribe<int>([&](Event<int>&) { order.push_back(3); }, 3);
    
    bus.publish(0);
    
    // Higher priority should be called first
    ASSERT_EQ(order.size(), 3u);
    ASSERT_EQ(order[0], 3);
    ASSERT_EQ(order[1], 2);
    ASSERT_EQ(order[2], 1);
}

TEST(event_bus_cancel) {
    EventBus bus;
    
    int call_count = 0;
    
    bus.subscribe<int>([&](Event<int>& e) {
        ++call_count;
        e.cancel();
    }, 2);
    
    bus.subscribe<int>([&](Event<int>&) {
        ++call_count;
    }, 1);
    
    bus.publish(0);
    
    // Second handler should not be called due to cancellation
    ASSERT_EQ(call_count, 1);
}

TEST(event_bus_once) {
    EventBus bus;
    
    int call_count = 0;
    
    bus.subscribe_once<int>([&](Event<int>&) {
        ++call_count;
    });
    
    bus.publish(1);
    bus.publish(2);
    bus.publish(3);
    
    // Should only be called once
    ASSERT_EQ(call_count, 1);
}

TEST(topic_pubsub_wildcard) {
    ASSERT_TRUE(TopicPubSub::matches_topic("sensor/#", "sensor/temp/room1"));
    ASSERT_TRUE(TopicPubSub::matches_topic("sensor/*/room1", "sensor/temp/room1"));
    ASSERT_FALSE(TopicPubSub::matches_topic("sensor/temp", "sensor/humidity"));
    ASSERT_TRUE(TopicPubSub::matches_topic("#", "any/topic/here"));
}

// =============================================================================
// Connection Pool Tests (Mock Connection)
// =============================================================================

struct MockConnection {
    int id;
    bool valid{true};
    
    MockConnection() : id(0) {}
    explicit MockConnection(int i) : id(i) {}
};

TEST(connection_pool_basic) {
    int next_id = 0;
    
    ConnectionPool<MockConnection> pool([&]() {
        return std::make_unique<MockConnection>(++next_id);
    });
    
    auto conn = pool.acquire();
    ASSERT_TRUE(conn != nullptr);
    ASSERT_GT(conn->id, 0);
    
    pool.release(conn);
    
    // Acquire another connection
    auto conn2 = pool.acquire();
    ASSERT_TRUE(conn2 != nullptr);
    ASSERT_GT(conn2->id, 0);
    
    pool.release(conn2);
    
    // Verify pool is working
    ASSERT_TRUE(pool.size() > 0);
}

TEST(connection_pool_scoped) {
    int next_id = 0;
    
    ConnectionPool<MockConnection> pool([&]() {
        return std::make_unique<MockConnection>(++next_id);
    });
    
    {
        auto scoped = pool.acquire_scoped();
        ASSERT_TRUE(scoped.get() != nullptr);
        ASSERT_GT(scoped->id, 0);
        // Connection auto-released at end of scope
    }
    
    // Pool should have at least the connection that was released
    ASSERT_TRUE(pool.size() > 0);
}

// =============================================================================
// Object Pool Tests (v3.6.3)
// =============================================================================

TEST(object_pool_basic) {
    ObjectPool<String> pool(5, 10);
    
    auto* str = pool.acquire();
    ASSERT_TRUE(str != nullptr);
    *str = "hello";
    
    pool.release(str);
    
    auto stats = pool.stats();
    ASSERT_EQ(stats.total_acquired, 1u);
    ASSERT_EQ(stats.total_released, 1u);
}

TEST(object_pool_scoped) {
    ObjectPool<String> pool(5, 10);
    
    {
        auto scoped = pool.acquire_scoped();
        ASSERT_TRUE(scoped.get() != nullptr);
        *scoped = "test";
    }
    
    // Object auto-released
    ASSERT_EQ(pool.available(), 5u);  // Back to initial
}

// =============================================================================
// Async Queue Tests (v3.6.3)
// =============================================================================

TEST(async_queue_basic) {
    AsyncQueue<int> queue;
    
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    ASSERT_EQ(queue.size(), 3u);
    
    auto val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(*val, 1);
}

TEST(bounded_queue_basic) {
    BoundedAsyncQueue<int> queue(3);
    
    ASSERT_TRUE(queue.try_push(1));
    ASSERT_TRUE(queue.try_push(2));
    ASSERT_TRUE(queue.try_push(3));
    ASSERT_FALSE(queue.try_push(4));  // Full
    
    ASSERT_EQ(queue.dropped(), 1u);
}

TEST(priority_queue_basic) {
    PriorityAsyncQueue<int, std::greater<int>> queue;  // Min-heap
    
    queue.push(3);
    queue.push(1);
    queue.push(2);
    
    auto val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(*val, 1);  // Smallest first
}

// =============================================================================
// Data Mapper Tests (v3.6.3)
// =============================================================================

TEST(field_value_types) {
    FieldValue str_val("hello");
    ASSERT_TRUE(str_val.is_string());
    ASSERT_EQ(str_val.as_string(), "hello");
    
    FieldValue int_val(static_cast<Int64>(42));
    ASSERT_TRUE(int_val.is_integer());
    ASSERT_EQ(int_val.as_integer(), 42);
    
    FieldValue dec_val(3.14);
    ASSERT_TRUE(dec_val.is_decimal());
    ASSERT_TRUE(std::abs(dec_val.as_decimal() - 3.14) < 0.001);
}

TEST(record_def_basic) {
    RecordDef rec("Customer");
    rec.add_field("ID", FieldType::Integer, 0, 4)
       .add_field("NAME", FieldType::String, 4, 20)
       .add_field("BALANCE", FieldType::Decimal, 24, 8);
    
    ASSERT_EQ(rec.field_count(), 3u);
    ASSERT_EQ(rec.record_length(), 32u);
    
    auto* id_field = rec.field("ID");
    ASSERT_TRUE(id_field != nullptr);
    ASSERT_EQ(id_field->type, FieldType::Integer);
}

TEST(data_mapper_transform) {
    // Test uppercase transform
    FieldValue input("hello");
    auto result = transforms::to_upper(input);
    ASSERT_EQ(result.as_string(), "HELLO");
    
    // Test trim
    FieldValue padded("  test  ");
    auto trimmed = transforms::trim(padded);
    ASSERT_EQ(trimmed.as_string(), "test");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "\n=== IBM IMS (Information Management System) Emulation v3.6.3 - New Modules Test Suite ===\n\n";
    
    // Tests are auto-registered and run
    
    std::cout << "\n=== Test Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    
    if (tests_failed > 0) {
        std::cout << "\nSOME TESTS FAILED!\n";
        return 1;
    }
    
    std::cout << "\nALL TESTS PASSED!\n";
    return 0;
}
