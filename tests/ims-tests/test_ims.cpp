// =============================================================================
// IBM IMS Emulation Enterprise - IMS Library Tests
// Version: 3.6.2
// =============================================================================

#include "../test_framework.hpp"
#include "ims/ims/ims_types.hpp"

using namespace ims;
using namespace ims::imsdb;
using namespace ims::test;

// =============================================================================
// Database Type Tests
// =============================================================================

bool test_database_type_to_string() {
    TEST_ASSERT_EQ("HIDAM", database_type_to_string(DatabaseType::HIDAM));
    TEST_ASSERT_EQ("HDAM", database_type_to_string(DatabaseType::HDAM));
    TEST_ASSERT_EQ("HISAM", database_type_to_string(DatabaseType::HISAM));
    TEST_ASSERT_EQ("HSAM", database_type_to_string(DatabaseType::HSAM));
    TEST_ASSERT_EQ("SHISAM", database_type_to_string(DatabaseType::SHISAM));
    TEST_ASSERT_EQ("HALDB", database_type_to_string(DatabaseType::HALDB));
    
    return true;
}

bool test_access_method_to_string() {
    TEST_ASSERT_EQ("SEQUENTIAL", access_method_to_string(AccessMethod::SEQUENTIAL));
    TEST_ASSERT_EQ("RANDOM", access_method_to_string(AccessMethod::RANDOM));
    TEST_ASSERT_EQ("DYNAMIC", access_method_to_string(AccessMethod::DYNAMIC));
    
    return true;
}

// =============================================================================
// DL/I Call Tests
// =============================================================================

bool test_dli_call_to_string() {
    TEST_ASSERT_EQ("GU", dli_call_to_string(DliCall::GU));
    TEST_ASSERT_EQ("GN", dli_call_to_string(DliCall::GN));
    TEST_ASSERT_EQ("GNP", dli_call_to_string(DliCall::GNP));
    TEST_ASSERT_EQ("ISRT", dli_call_to_string(DliCall::ISRT));
    TEST_ASSERT_EQ("DLET", dli_call_to_string(DliCall::DLET));
    TEST_ASSERT_EQ("REPL", dli_call_to_string(DliCall::REPL));
    TEST_ASSERT_EQ("CHKP", dli_call_to_string(DliCall::CHKP));
    TEST_ASSERT_EQ("ROLB", dli_call_to_string(DliCall::ROLB));
    
    return true;
}

// =============================================================================
// DL/I Status Code Tests
// =============================================================================

bool test_dli_status_to_string() {
    TEST_ASSERT_EQ("  ", dli_status_to_string(DliStatusCode::NORMAL));
    TEST_ASSERT_EQ("GB", dli_status_to_string(DliStatusCode::END_OF_DATABASE));
    TEST_ASSERT_EQ("GE", dli_status_to_string(DliStatusCode::SEGMENT_NOT_FOUND));
    TEST_ASSERT_EQ("II", dli_status_to_string(DliStatusCode::DUPLICATE_INSERT));
    TEST_ASSERT_EQ("AI", dli_status_to_string(DliStatusCode::INVALID_SSA));
    TEST_ASSERT_EQ("AJ", dli_status_to_string(DliStatusCode::NOT_AUTHORIZED));
    TEST_ASSERT_EQ("BA", dli_status_to_string(DliStatusCode::DATABASE_NOT_AVAILABLE));
    TEST_ASSERT_EQ("FE", dli_status_to_string(DliStatusCode::DEADLOCK_OCCURRED));
    
    return true;
}

bool test_dli_status_code_values_unique() {
    // Verify that the enum values are unique (was a known bug)
    TEST_ASSERT_NE(static_cast<UInt16>(DliStatusCode::SEGMENT_NOT_FOUND),
                   static_cast<UInt16>(DliStatusCode::END_OF_DATABASE));
    TEST_ASSERT_NE(static_cast<UInt16>(DliStatusCode::NOT_AUTHORIZED),
                   static_cast<UInt16>(DliStatusCode::INVALID_SSA));
    TEST_ASSERT_NE(static_cast<UInt16>(DliStatusCode::INVALID_FUNCTION),
                   static_cast<UInt16>(DliStatusCode::INVALID_SSA));
    
    return true;
}

// =============================================================================
// Segment Definition Tests
// =============================================================================

bool test_segment_definition_root() {
    SegmentDefinition seg;
    seg.name = "CUSTOMER";
    seg.type = SegmentType::ROOT;
    seg.length = 200;
    seg.key_offset = 0;
    seg.key_length = 10;
    seg.child_names = {"ORDER", "ADDRESS"};
    
    TEST_ASSERT_EQ("CUSTOMER", seg.name);
    TEST_ASSERT_EQ(static_cast<int>(SegmentType::ROOT), static_cast<int>(seg.type));
    TEST_ASSERT_EQ(200u, seg.length);
    TEST_ASSERT_EQ(10u, seg.key_length);
    TEST_ASSERT_EQ(2u, seg.child_names.size());
    
    return true;
}

bool test_segment_definition_child() {
    SegmentDefinition seg;
    seg.name = "ORDER";
    seg.type = SegmentType::CHILD;
    seg.length = 150;
    seg.parent_name = "CUSTOMER";
    seg.key_offset = 0;
    seg.key_length = 12;
    
    TEST_ASSERT_EQ("ORDER", seg.name);
    TEST_ASSERT_EQ(static_cast<int>(SegmentType::CHILD), static_cast<int>(seg.type));
    TEST_ASSERT_EQ("CUSTOMER", seg.parent_name);
    
    return true;
}

// =============================================================================
// Database Definition Tests
// =============================================================================

bool test_database_definition_default() {
    DatabaseDefinition db;
    
    TEST_ASSERT_TRUE(db.name.empty());
    TEST_ASSERT_EQ(static_cast<int>(DatabaseType::HIDAM), static_cast<int>(db.type));
    TEST_ASSERT_EQ(static_cast<int>(AccessMethod::RANDOM), static_cast<int>(db.access_method));
    TEST_ASSERT_TRUE(db.segments.empty());
    TEST_ASSERT_FALSE(db.is_partitioned);
    
    return true;
}

bool test_database_definition_with_segments() {
    DatabaseDefinition db;
    db.name = "CUSTDB";
    db.type = DatabaseType::HIDAM;
    db.access_method = AccessMethod::RANDOM;
    db.vsam_dataset = "PROD.CUSTDB.DATA";
    db.max_segments = 10000;
    
    SegmentDefinition root;
    root.name = "CUSTOMER";
    root.type = SegmentType::ROOT;
    root.length = 200;
    
    SegmentDefinition child;
    child.name = "ORDER";
    child.type = SegmentType::CHILD;
    child.parent_name = "CUSTOMER";
    child.length = 150;
    
    db.segments = {root, child};
    
    TEST_ASSERT_EQ("CUSTDB", db.name);
    TEST_ASSERT_EQ(2u, db.segments.size());
    TEST_ASSERT_EQ("CUSTOMER", db.segments[0].name);
    TEST_ASSERT_EQ("ORDER", db.segments[1].name);
    
    return true;
}

// =============================================================================
// DL/I Request Tests
// =============================================================================

bool test_dli_request_default() {
    DliRequest request;
    
    TEST_ASSERT_EQ(static_cast<int>(DliCall::GU), static_cast<int>(request.function_code));
    TEST_ASSERT_TRUE(request.pcb_name.empty());
    TEST_ASSERT_TRUE(request.ssa.empty());
    TEST_ASSERT_EQ(0u, request.io_area_length);
    
    return true;
}

bool test_dli_request_gu() {
    DliRequest request;
    request.function_code = DliCall::GU;
    request.pcb_name = "CUSTPCB";
    request.ssa = {"CUSTOMER(CUSTKEY='C000000001')"};
    request.io_area.resize(200);
    request.io_area_length = 200;
    
    TEST_ASSERT_EQ(static_cast<int>(DliCall::GU), static_cast<int>(request.function_code));
    TEST_ASSERT_EQ("CUSTPCB", request.pcb_name);
    TEST_ASSERT_EQ(1u, request.ssa.size());
    TEST_ASSERT_EQ(200u, request.io_area_length);
    
    return true;
}

bool test_dli_request_isrt() {
    DliRequest request;
    request.function_code = DliCall::ISRT;
    request.pcb_name = "CUSTPCB";
    request.ssa = {"CUSTOMER(CUSTKEY='C000000001')", "ORDER"};
    request.io_area.resize(150);
    request.io_area_length = 150;
    
    TEST_ASSERT_EQ(static_cast<int>(DliCall::ISRT), static_cast<int>(request.function_code));
    TEST_ASSERT_EQ(2u, request.ssa.size());
    
    return true;
}

// =============================================================================
// DL/I Response Tests
// =============================================================================

bool test_dli_response_default() {
    DliResponse response;
    
    TEST_ASSERT_EQ(static_cast<int>(DliStatusCode::NORMAL), static_cast<int>(response.status));
    TEST_ASSERT_TRUE(response.is_success());
    TEST_ASSERT_EQ(0u, response.segment_level);
    TEST_ASSERT_EQ(0u, response.processing_time_ns);
    
    return true;
}

bool test_dli_response_success() {
    DliResponse response;
    response.status = DliStatusCode::NORMAL;
    response.segment_name = "CUSTOMER";
    response.segment_level = 1;
    response.segment_data.resize(200);
    response.processing_time_ns = 125000;
    
    TEST_ASSERT_TRUE(response.is_success());
    TEST_ASSERT_EQ("CUSTOMER", response.segment_name);
    TEST_ASSERT_EQ(1u, response.segment_level);
    
    return true;
}

bool test_dli_response_error() {
    DliResponse response;
    response.status = DliStatusCode::SEGMENT_NOT_FOUND;
    
    TEST_ASSERT_FALSE(response.is_success());
    TEST_ASSERT_EQ(static_cast<int>(DliStatusCode::SEGMENT_NOT_FOUND), static_cast<int>(response.status));
    
    return true;
}

// =============================================================================
// IMS Statistics Tests
// =============================================================================

bool test_ims_statistics_default() {
    ImsStatistics stats;
    
    TEST_ASSERT_EQ(0u, stats.total_transactions);
    TEST_ASSERT_EQ(0u, stats.successful_transactions);
    TEST_ASSERT_EQ(0u, stats.failed_transactions);
    TEST_ASSERT_EQ(100.0, stats.success_rate());  // 0/0 = 100%
    
    return true;
}

bool test_ims_statistics_success_rate() {
    ImsStatistics stats;
    stats.total_transactions = 100;
    stats.successful_transactions = 95;
    stats.failed_transactions = 5;
    
    double rate = stats.success_rate();
    TEST_ASSERT_TRUE(rate > 94.0 && rate < 96.0);  // ~95%
    
    return true;
}

bool test_ims_statistics_calls() {
    ImsStatistics stats;
    stats.get_calls = 1000;
    stats.insert_calls = 200;
    stats.delete_calls = 50;
    stats.replace_calls = 100;
    stats.checkpoint_calls = 10;
    
    TEST_ASSERT_EQ(1000u, stats.get_calls);
    TEST_ASSERT_EQ(200u, stats.insert_calls);
    TEST_ASSERT_EQ(50u, stats.delete_calls);
    TEST_ASSERT_EQ(100u, stats.replace_calls);
    TEST_ASSERT_EQ(10u, stats.checkpoint_calls);
    
    return true;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    TestRunner runner("IMS Library Tests");
    
    // Database type tests
    runner.add_test("database_type_to_string", test_database_type_to_string);
    runner.add_test("access_method_to_string", test_access_method_to_string);
    
    // DL/I call tests
    runner.add_test("dli_call_to_string", test_dli_call_to_string);
    
    // DL/I status tests
    runner.add_test("dli_status_to_string", test_dli_status_to_string);
    runner.add_test("DliStatusCode unique values", test_dli_status_code_values_unique);
    
    // Segment definition tests
    runner.add_test("SegmentDefinition root", test_segment_definition_root);
    runner.add_test("SegmentDefinition child", test_segment_definition_child);
    
    // Database definition tests
    runner.add_test("DatabaseDefinition default", test_database_definition_default);
    runner.add_test("DatabaseDefinition with segments", test_database_definition_with_segments);
    
    // DL/I request tests
    runner.add_test("DliRequest default", test_dli_request_default);
    runner.add_test("DliRequest GU", test_dli_request_gu);
    runner.add_test("DliRequest ISRT", test_dli_request_isrt);
    
    // DL/I response tests
    runner.add_test("DliResponse default", test_dli_response_default);
    runner.add_test("DliResponse success", test_dli_response_success);
    runner.add_test("DliResponse error", test_dli_response_error);
    
    // IMS statistics tests
    runner.add_test("ImsStatistics default", test_ims_statistics_default);
    runner.add_test("ImsStatistics success rate", test_ims_statistics_success_rate);
    runner.add_test("ImsStatistics calls", test_ims_statistics_calls);
    
    return runner.run();
}
