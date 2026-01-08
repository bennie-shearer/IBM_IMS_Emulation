// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - IMS/DL-I Example
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/ims/ims_types.hpp"

#include <iostream>
#include <iomanip>

using namespace ims;
using namespace ims::imsdb;

/**
 * @brief Demonstrates IMS/DL-I database operations
 * 
 * This example shows:
 * - Database and segment definitions
 * - DL/I call construction
 * - Status code handling
 * - Hierarchical data modeling
 */

void print_separator(const char* title) {
    std::cout << std::endl << "--- " << title << " ---" << std::endl;
}

int main() {
    std::cout << "=== IMS/DL-I Example ===" << std::endl;
    
    // =========================================================================
    // Database Type Overview
    // =========================================================================
    print_separator("IMS Database Types");
    
    Vector<DatabaseType> db_types = {
        DatabaseType::HIDAM,
        DatabaseType::HDAM,
        DatabaseType::HISAM,
        DatabaseType::HSAM,
        DatabaseType::SHISAM,
        DatabaseType::HALDB
    };
    
    for (auto type : db_types) {
        std::cout << "  " << database_type_to_string(type);
        switch (type) {
            case DatabaseType::HIDAM:
                std::cout << " - Hierarchical Indexed Direct Access Method";
                break;
            case DatabaseType::HDAM:
                std::cout << " - Hierarchical Direct Access Method";
                break;
            case DatabaseType::HISAM:
                std::cout << " - Hierarchical Indexed Sequential Access Method";
                break;
            case DatabaseType::HSAM:
                std::cout << " - Hierarchical Sequential Access Method";
                break;
            case DatabaseType::SHISAM:
                std::cout << " - Simple HISAM";
                break;
            case DatabaseType::HALDB:
                std::cout << " - High Availability Large Database";
                break;
            default:
                break;
        }
        std::cout << std::endl;
    }
    
    // =========================================================================
    // Database Definition
    // =========================================================================
    print_separator("Database Definition");
    
    // Create a customer database definition
    DatabaseDefinition customer_db;
    customer_db.name = "CUSTDB";
    customer_db.type = DatabaseType::HIDAM;
    customer_db.access_method = AccessMethod::RANDOM;
    customer_db.vsam_dataset = "PROD.CUSTOMER.HIDAM";
    customer_db.max_segments = 10000;
    customer_db.is_partitioned = false;
    
    std::cout << "Customer Database:" << std::endl;
    std::cout << "  Name: " << customer_db.name << std::endl;
    std::cout << "  Type: " << database_type_to_string(customer_db.type) << std::endl;
    std::cout << "  Access: " << access_method_to_string(customer_db.access_method) << std::endl;
    std::cout << "  VSAM Dataset: " << customer_db.vsam_dataset << std::endl;
    std::cout << "  Max Segments: " << customer_db.max_segments << std::endl;
    
    // =========================================================================
    // Segment Definitions
    // =========================================================================
    print_separator("Segment Definitions");
    
    // Root segment - CUSTOMER
    SegmentDefinition customer_seg;
    customer_seg.name = "CUSTOMER";
    customer_seg.type = SegmentType::ROOT;
    customer_seg.length = 200;
    customer_seg.key_offset = 0;
    customer_seg.key_length = 10;
    customer_seg.child_names = {"ORDER", "ADDRESS"};
    
    // Child segment - ORDER
    SegmentDefinition order_seg;
    order_seg.name = "ORDER";
    order_seg.type = SegmentType::CHILD;
    order_seg.length = 150;
    order_seg.parent_name = "CUSTOMER";
    order_seg.key_offset = 0;
    order_seg.key_length = 12;
    order_seg.child_names = {"ITEM"};
    
    // Child segment - ADDRESS
    SegmentDefinition address_seg;
    address_seg.name = "ADDRESS";
    address_seg.type = SegmentType::CHILD;
    address_seg.length = 300;
    address_seg.parent_name = "CUSTOMER";
    address_seg.key_offset = 0;
    address_seg.key_length = 4;
    
    // Grandchild segment - ITEM
    SegmentDefinition item_seg;
    item_seg.name = "ITEM";
    item_seg.type = SegmentType::CHILD;
    item_seg.length = 100;
    item_seg.parent_name = "ORDER";
    item_seg.key_offset = 0;
    item_seg.key_length = 8;
    
    customer_db.segments = {customer_seg, order_seg, address_seg, item_seg};
    
    std::cout << "Hierarchical Structure:" << std::endl;
    std::cout << "  CUSTOMER (Root)" << std::endl;
    std::cout << "    |---- ORDER" << std::endl;
    std::cout << "    |   +---- ITEM" << std::endl;
    std::cout << "    +---- ADDRESS" << std::endl;
    
    std::cout << std::endl << "Segment Details:" << std::endl;
    for (const auto& seg : customer_db.segments) {
        std::cout << "  " << seg.name << ":" << std::endl;
        std::cout << "    Type: " << (seg.type == SegmentType::ROOT ? "ROOT" : "CHILD") << std::endl;
        std::cout << "    Length: " << seg.length << " bytes" << std::endl;
        std::cout << "    Key: offset=" << seg.key_offset << ", length=" << seg.key_length << std::endl;
        if (!seg.parent_name.empty()) {
            std::cout << "    Parent: " << seg.parent_name << std::endl;
        }
    }
    
    // =========================================================================
    // DL/I Call Types
    // =========================================================================
    print_separator("DL/I Call Types");
    
    Vector<DliCall> calls = {
        DliCall::GU, DliCall::GN, DliCall::GNP,
        DliCall::ISRT, DliCall::DLET, DliCall::REPL,
        DliCall::CHKP, DliCall::ROLB
    };
    
    for (auto call : calls) {
        std::cout << "  " << std::setw(4) << dli_call_to_string(call) << " - ";
        switch (call) {
            case DliCall::GU:   std::cout << "Get Unique (direct retrieval by key)"; break;
            case DliCall::GN:   std::cout << "Get Next (sequential retrieval)"; break;
            case DliCall::GNP:  std::cout << "Get Next within Parent"; break;
            case DliCall::ISRT: std::cout << "Insert segment"; break;
            case DliCall::DLET: std::cout << "Delete segment"; break;
            case DliCall::REPL: std::cout << "Replace segment"; break;
            case DliCall::CHKP: std::cout << "Checkpoint"; break;
            case DliCall::ROLB: std::cout << "Rollback"; break;
            default: break;
        }
        std::cout << std::endl;
    }
    
    // =========================================================================
    // DL/I Status Codes
    // =========================================================================
    print_separator("DL/I Status Codes");
    
    Vector<DliStatusCode> status_codes = {
        DliStatusCode::NORMAL,
        DliStatusCode::END_OF_DATABASE,
        DliStatusCode::SEGMENT_NOT_FOUND,
        DliStatusCode::DUPLICATE_INSERT,
        DliStatusCode::INVALID_SSA,
        DliStatusCode::NOT_AUTHORIZED,
        DliStatusCode::DATABASE_NOT_AVAILABLE,
        DliStatusCode::DEADLOCK_OCCURRED
    };
    
    for (auto status : status_codes) {
        std::cout << "  '" << dli_status_to_string(status) << "' (0x" 
                  << std::hex << std::setw(4) << std::setfill('0') 
                  << static_cast<int>(status) << std::dec << std::setfill(' ')
                  << ") - ";
        switch (status) {
            case DliStatusCode::NORMAL: 
                std::cout << "Successful completion"; break;
            case DliStatusCode::END_OF_DATABASE: 
                std::cout << "End of database reached"; break;
            case DliStatusCode::SEGMENT_NOT_FOUND: 
                std::cout << "Segment not found"; break;
            case DliStatusCode::DUPLICATE_INSERT: 
                std::cout << "Duplicate segment key"; break;
            case DliStatusCode::INVALID_SSA: 
                std::cout << "Invalid segment search argument"; break;
            case DliStatusCode::NOT_AUTHORIZED: 
                std::cout << "Not authorized for database"; break;
            case DliStatusCode::DATABASE_NOT_AVAILABLE: 
                std::cout << "Database not available"; break;
            case DliStatusCode::DEADLOCK_OCCURRED: 
                std::cout << "Deadlock detected"; break;
            default: break;
        }
        std::cout << std::endl;
    }
    
    // =========================================================================
    // DL/I Request Construction
    // =========================================================================
    print_separator("DL/I Request Examples");
    
    // GU (Get Unique) - Direct retrieval
    DliRequest gu_request;
    gu_request.function_code = DliCall::GU;
    gu_request.pcb_name = "CUSTPCB";
    gu_request.ssa = {"CUSTOMER(CUSTKEY='C000000001')"};
    gu_request.io_area.resize(200);
    gu_request.io_area_length = 200;
    
    std::cout << "GU Request (Get Customer by Key):" << std::endl;
    std::cout << "  Function: " << dli_call_to_string(gu_request.function_code) << std::endl;
    std::cout << "  PCB: " << gu_request.pcb_name << std::endl;
    std::cout << "  SSA[0]: " << gu_request.ssa[0] << std::endl;
    std::cout << "  I/O Area: " << gu_request.io_area_length << " bytes" << std::endl;
    
    // GN (Get Next) - Sequential retrieval
    DliRequest gn_request;
    gn_request.function_code = DliCall::GN;
    gn_request.pcb_name = "CUSTPCB";
    gn_request.ssa = {"CUSTOMER", "ORDER"};
    gn_request.io_area.resize(150);
    gn_request.io_area_length = 150;
    
    std::cout << std::endl << "GN Request (Get Next Order):" << std::endl;
    std::cout << "  Function: " << dli_call_to_string(gn_request.function_code) << std::endl;
    std::cout << "  PCB: " << gn_request.pcb_name << std::endl;
    std::cout << "  SSA[0]: " << gn_request.ssa[0] << std::endl;
    std::cout << "  SSA[1]: " << gn_request.ssa[1] << std::endl;
    
    // ISRT (Insert) - Add new segment
    DliRequest isrt_request;
    isrt_request.function_code = DliCall::ISRT;
    isrt_request.pcb_name = "CUSTPCB";
    isrt_request.ssa = {"CUSTOMER(CUSTKEY='C000000001')", "ORDER"};
    isrt_request.io_area.resize(150);
    isrt_request.io_area_length = 150;
    
    std::cout << std::endl << "ISRT Request (Insert New Order):" << std::endl;
    std::cout << "  Function: " << dli_call_to_string(isrt_request.function_code) << std::endl;
    std::cout << "  PCB: " << isrt_request.pcb_name << std::endl;
    std::cout << "  Parent SSA: " << isrt_request.ssa[0] << std::endl;
    std::cout << "  Segment: " << isrt_request.ssa[1] << std::endl;
    
    // =========================================================================
    // DL/I Response Examples
    // =========================================================================
    print_separator("DL/I Response Examples");
    
    // Successful response
    DliResponse success_response;
    success_response.status = DliStatusCode::NORMAL;
    success_response.segment_name = "CUSTOMER";
    success_response.segment_level = 1;
    success_response.segment_data.resize(200);
    success_response.processing_time_ns = 125000;  // 125 microseconds
    
    std::cout << "Successful Response:" << std::endl;
    std::cout << "  Status: '" << dli_status_to_string(success_response.status) << "'" << std::endl;
    std::cout << "  Segment: " << success_response.segment_name << std::endl;
    std::cout << "  Level: " << success_response.segment_level << std::endl;
    std::cout << "  Data Size: " << success_response.segment_data.size() << " bytes" << std::endl;
    std::cout << "  Processing Time: " << static_cast<double>(success_response.processing_time_ns) / 1000.0 << " us" << std::endl;
    std::cout << "  is_success(): " << (success_response.is_success() ? "true" : "false") << std::endl;
    
    // Not found response
    DliResponse notfound_response;
    notfound_response.status = DliStatusCode::SEGMENT_NOT_FOUND;
    notfound_response.processing_time_ns = 50000;
    
    std::cout << std::endl << "Not Found Response:" << std::endl;
    std::cout << "  Status: '" << dli_status_to_string(notfound_response.status) << "'" << std::endl;
    std::cout << "  is_success(): " << (notfound_response.is_success() ? "true" : "false") << std::endl;
    
    // End of database response
    DliResponse eod_response;
    eod_response.status = DliStatusCode::END_OF_DATABASE;
    
    std::cout << std::endl << "End of Database Response:" << std::endl;
    std::cout << "  Status: '" << dli_status_to_string(eod_response.status) << "'" << std::endl;
    
    // =========================================================================
    // IMS Statistics
    // =========================================================================
    print_separator("IMS Statistics");
    
    ImsStatistics stats;
    stats.total_transactions = 100000;
    stats.successful_transactions = 99500;
    stats.failed_transactions = 500;
    stats.get_calls = 75000;
    stats.insert_calls = 15000;
    stats.delete_calls = 2000;
    stats.replace_calls = 8000;
    stats.checkpoint_calls = 100;
    
    std::cout << "Transaction Statistics:" << std::endl;
    std::cout << "  Total: " << stats.total_transactions << std::endl;
    std::cout << "  Successful: " << stats.successful_transactions << std::endl;
    std::cout << "  Failed: " << stats.failed_transactions << std::endl;
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(2) 
              << stats.success_rate() << "%" << std::endl;
    
    std::cout << std::endl << "Call Statistics:" << std::endl;
    std::cout << "  GET calls: " << stats.get_calls << std::endl;
    std::cout << "  INSERT calls: " << stats.insert_calls << std::endl;
    std::cout << "  DELETE calls: " << stats.delete_calls << std::endl;
    std::cout << "  REPLACE calls: " << stats.replace_calls << std::endl;
    std::cout << "  CHECKPOINT calls: " << stats.checkpoint_calls << std::endl;
    
    // =========================================================================
    // Simulated DL/I Session
    // =========================================================================
    print_separator("Simulated DL/I Session");
    
    std::cout << "1. Open database CUSTDB" << std::endl;
    std::cout << "   -> Status: '  ' (Success)" << std::endl;
    
    std::cout << "2. GU CUSTOMER(CUSTKEY='C000000001')" << std::endl;
    std::cout << "   -> Status: '  ' Customer retrieved" << std::endl;
    
    std::cout << "3. GNP ORDER" << std::endl;
    std::cout << "   -> Status: '  ' Order #1 retrieved" << std::endl;
    
    std::cout << "4. GNP ITEM" << std::endl;
    std::cout << "   -> Status: '  ' Item #1 retrieved" << std::endl;
    
    std::cout << "5. GNP ITEM" << std::endl;
    std::cout << "   -> Status: '  ' Item #2 retrieved" << std::endl;
    
    std::cout << "6. GNP ITEM" << std::endl;
    std::cout << "   -> Status: 'GE' No more items" << std::endl;
    
    std::cout << "7. GNP ORDER" << std::endl;
    std::cout << "   -> Status: '  ' Order #2 retrieved" << std::endl;
    
    std::cout << "8. ISRT ORDER (new order data)" << std::endl;
    std::cout << "   -> Status: '  ' Order inserted" << std::endl;
    
    std::cout << "9. CHKP (checkpoint)" << std::endl;
    std::cout << "   -> Status: '  ' Checkpoint taken" << std::endl;
    
    std::cout << "10. Close database" << std::endl;
    std::cout << "    -> Status: '  ' (Success)" << std::endl;
    
    std::cout << std::endl << "=== IMS/DL-I Example Complete ===" << std::endl;
    
    return 0;
}
