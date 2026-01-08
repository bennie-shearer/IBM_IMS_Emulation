// =============================================================================
// IBM IMS Emulation Enterprise - VSAM Library Tests
// Version: 3.6.2
// =============================================================================

#include "../test_framework.hpp"
#include "ims/vsam/vsam_types.hpp"

using namespace ims;
using namespace ims::vsam;
using namespace ims::test;

// =============================================================================
// VSAM Configuration Tests
// =============================================================================

bool test_vsam_dataset_config_default() {
    VsamDatasetConfig config;
    
    TEST_ASSERT_TRUE(config.name.empty());
    TEST_ASSERT_EQ(static_cast<int>(VsamType::KSDS), static_cast<int>(config.type));
    TEST_ASSERT_EQ(static_cast<int>(catalog::RecordFormat::VARIABLE), static_cast<int>(config.recfm));
    TEST_ASSERT_EQ(32760u, config.max_record_size);
    
    return true;
}

bool test_vsam_dataset_config_ksds() {
    VsamDatasetConfig config("TEST.KSDS", VsamType::KSDS);
    
    TEST_ASSERT_EQ("TEST.KSDS", config.name);
    TEST_ASSERT_EQ(static_cast<int>(VsamType::KSDS), static_cast<int>(config.type));
    TEST_ASSERT_EQ(8u, config.key_length);  // Default key length for KSDS
    
    return true;
}

bool test_vsam_dataset_config_esds() {
    VsamDatasetConfig config("TEST.ESDS", VsamType::ESDS);
    
    TEST_ASSERT_EQ("TEST.ESDS", config.name);
    TEST_ASSERT_EQ(static_cast<int>(VsamType::ESDS), static_cast<int>(config.type));
    TEST_ASSERT_EQ(0u, config.key_length);  // ESDS has no key
    
    return true;
}

// =============================================================================
// VSAM Key Tests
// =============================================================================

bool test_vsam_key_default() {
    VsamKey key;
    
    TEST_ASSERT_TRUE(key.empty());
    TEST_ASSERT_EQ(0u, key.length);
    
    return true;
}

bool test_vsam_key_from_size() {
    VsamKey key(16);
    
    TEST_ASSERT_FALSE(key.empty());
    TEST_ASSERT_EQ(16u, key.length);
    TEST_ASSERT_EQ(16u, key.data.size());
    
    return true;
}

bool test_vsam_key_from_string() {
    VsamKey key("CUSTOMER001");
    
    TEST_ASSERT_FALSE(key.empty());
    TEST_ASSERT_EQ(11u, key.length);
    TEST_ASSERT_EQ("CUSTOMER001", key.to_string());
    
    return true;
}

bool test_vsam_key_from_bytes() {
    Byte bytes[] = {0x41, 0x42, 0x43, 0x44};  // "ABCD"
    VsamKey key(bytes, 4);
    
    TEST_ASSERT_EQ(4u, key.length);
    TEST_ASSERT_EQ("ABCD", key.to_string());
    
    return true;
}

bool test_vsam_key_comparison() {
    VsamKey key1("AAA");
    VsamKey key2("AAA");
    VsamKey key3("BBB");
    VsamKey key4("AA");
    
    // Equality
    TEST_ASSERT_TRUE(key1 == key2);
    TEST_ASSERT_FALSE(key1 == key3);
    
    // Less than
    TEST_ASSERT_TRUE(key1 < key3);
    TEST_ASSERT_FALSE(key3 < key1);
    TEST_ASSERT_TRUE(key4 < key1);  // Shorter key is less
    
    return true;
}

// =============================================================================
// VSAM Record Tests
// =============================================================================

bool test_vsam_record_default() {
    VsamRecord record;
    
    TEST_ASSERT_TRUE(record.empty());
    TEST_ASSERT_EQ(0u, record.length);
    TEST_ASSERT_EQ(0u, record.rba);
    TEST_ASSERT_EQ(0u, record.slot);
    
    return true;
}

bool test_vsam_record_from_size() {
    VsamRecord record(256);
    
    TEST_ASSERT_FALSE(record.empty());
    TEST_ASSERT_EQ(256u, record.data.size());
    
    return true;
}

bool test_vsam_record_from_bytes() {
    Byte data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    VsamRecord record(data, 5);
    
    TEST_ASSERT_EQ(5u, record.length);
    TEST_ASSERT_EQ(5u, record.data.size());
    
    for (size_t i = 0; i < 5; ++i) {
        TEST_ASSERT_EQ(data[i], record.data[i]);
    }
    
    return true;
}

bool test_vsam_record_with_key() {
    VsamRecord record(100);
    record.key = VsamKey("KEY001");
    record.length = 100;
    
    TEST_ASSERT_EQ("KEY001", record.key.to_string());
    TEST_ASSERT_EQ(100u, record.length);
    
    return true;
}

bool test_vsam_record_esds_rba() {
    VsamRecord record(256);
    record.rba = 0x00001000;  // 4096
    record.length = 256;
    
    TEST_ASSERT_EQ(0x00001000u, record.rba);
    
    return true;
}

bool test_vsam_record_rrds_slot() {
    VsamRecord record(512);
    record.slot = 42;
    record.length = 512;
    
    TEST_ASSERT_EQ(42u, record.slot);
    
    return true;
}

// =============================================================================
// VSAM Statistics Tests
// =============================================================================

bool test_vsam_statistics_default() {
    VsamStatistics stats;
    
    TEST_ASSERT_EQ(0u, stats.record_count);
    TEST_ASSERT_EQ(0u, stats.read_operations);
    TEST_ASSERT_EQ(0u, stats.total_operations());
    TEST_ASSERT_EQ(0.0, stats.space_utilization());
    
    return true;
}

bool test_vsam_statistics_space_utilization() {
    VsamStatistics stats;
    stats.allocated_space = 1000000;
    stats.used_space = 750000;
    
    double util = stats.space_utilization();
    TEST_ASSERT_TRUE(util > 74.0 && util < 76.0);  // ~75%
    
    return true;
}

bool test_vsam_statistics_total_operations() {
    VsamStatistics stats;
    stats.read_operations = 100;
    stats.write_operations = 50;
    stats.update_operations = 25;
    stats.delete_operations = 10;
    
    TEST_ASSERT_EQ(185u, stats.total_operations());
    
    return true;
}

// =============================================================================
// VSAM Return Code Tests
// =============================================================================

bool test_vsam_return_code_to_string() {
    TEST_ASSERT_EQ("SUCCESS", vsam_return_code_to_string(VsamReturnCode::SUCCESS));
    TEST_ASSERT_EQ("DUPLICATE_KEY", vsam_return_code_to_string(VsamReturnCode::DUPLICATE_KEY));
    TEST_ASSERT_EQ("RECORD_NOT_FOUND", vsam_return_code_to_string(VsamReturnCode::RECORD_NOT_FOUND));
    TEST_ASSERT_EQ("END_OF_FILE", vsam_return_code_to_string(VsamReturnCode::END_OF_FILE));
    TEST_ASSERT_EQ("INVALID_KEY", vsam_return_code_to_string(VsamReturnCode::INVALID_KEY));
    TEST_ASSERT_EQ("RBA_NOT_FOUND", vsam_return_code_to_string(VsamReturnCode::RBA_NOT_FOUND));
    TEST_ASSERT_EQ("SLOT_NOT_FOUND", vsam_return_code_to_string(VsamReturnCode::SLOT_NOT_FOUND));
    TEST_ASSERT_EQ("DATASET_FULL", vsam_return_code_to_string(VsamReturnCode::DATASET_FULL));
    
    return true;
}

bool test_vsam_return_code_values() {
    TEST_ASSERT_EQ(0, static_cast<int>(VsamReturnCode::SUCCESS));
    TEST_ASSERT_EQ(4, static_cast<int>(VsamReturnCode::END_OF_FILE));
    TEST_ASSERT_EQ(8, static_cast<int>(VsamReturnCode::DUPLICATE_KEY));
    TEST_ASSERT_EQ(16, static_cast<int>(VsamReturnCode::RECORD_NOT_FOUND));
    
    return true;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    TestRunner runner("VSAM Library Tests");
    
    // Configuration tests
    runner.add_test("VsamDatasetConfig default", test_vsam_dataset_config_default);
    runner.add_test("VsamDatasetConfig KSDS", test_vsam_dataset_config_ksds);
    runner.add_test("VsamDatasetConfig ESDS", test_vsam_dataset_config_esds);
    
    // Key tests
    runner.add_test("VsamKey default", test_vsam_key_default);
    runner.add_test("VsamKey from size", test_vsam_key_from_size);
    runner.add_test("VsamKey from string", test_vsam_key_from_string);
    runner.add_test("VsamKey from bytes", test_vsam_key_from_bytes);
    runner.add_test("VsamKey comparison", test_vsam_key_comparison);
    
    // Record tests
    runner.add_test("VsamRecord default", test_vsam_record_default);
    runner.add_test("VsamRecord from size", test_vsam_record_from_size);
    runner.add_test("VsamRecord from bytes", test_vsam_record_from_bytes);
    runner.add_test("VsamRecord with key", test_vsam_record_with_key);
    runner.add_test("VsamRecord ESDS RBA", test_vsam_record_esds_rba);
    runner.add_test("VsamRecord RRDS slot", test_vsam_record_rrds_slot);
    
    // Statistics tests
    runner.add_test("VsamStatistics default", test_vsam_statistics_default);
    runner.add_test("VsamStatistics space utilization", test_vsam_statistics_space_utilization);
    runner.add_test("VsamStatistics total operations", test_vsam_statistics_total_operations);
    
    // Return code tests
    runner.add_test("vsam_return_code_to_string", test_vsam_return_code_to_string);
    runner.add_test("VsamReturnCode values", test_vsam_return_code_values);
    
    return runner.run();
}
