// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - VSAM Example
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/vsam/vsam_types.hpp"

#include <iostream>
#include <iomanip>
#include <cstring>

using namespace ims;
using namespace ims::vsam;

/**
 * @brief Demonstrates VSAM dataset operations
 * 
 * This example shows:
 * - VSAM dataset configuration for different types
 * - Key and record manipulation
 * - KSDS, ESDS, RRDS concepts
 * - Statistics tracking
 */

// Sample customer record structure
struct CustomerRecord {
    char customer_id[16];
    char name[50];
    char address[100];
    char city[30];
    char state[3];  // 2 chars + null terminator
    char zip[10];
    double balance;
    int32_t transaction_count;
};

void print_separator(const char* title) {
    std::cout << std::endl << "--- " << title << " ---" << std::endl;
}

int main() {
    std::cout << "=== IMS VSAM Example ===" << std::endl;
    
    // =========================================================================
    // KSDS (Key Sequenced Data Set) Configuration
    // =========================================================================
    print_separator("KSDS Configuration");
    
    VsamDatasetConfig ksds_config("CUSTOMER.MASTER.KSDS", VsamType::KSDS);
    ksds_config.key_length = 16;              // Customer ID length
    ksds_config.key_offset = 0;               // Key at start of record
    ksds_config.max_record_size = sizeof(CustomerRecord);
    ksds_config.avg_record_size = sizeof(CustomerRecord);
    ksds_config.initial_size = 10 * 1024 * 1024;  // 10 MB
    ksds_config.allow_duplicates = false;
    ksds_config.compressed = true;
    
    std::cout << "KSDS Dataset Configuration:" << std::endl;
    std::cout << "  Name: " << ksds_config.name << std::endl;
    std::cout << "  Type: " << catalog::vsam_type_to_string(ksds_config.type) << std::endl;
    std::cout << "  Key Length: " << ksds_config.key_length << " bytes" << std::endl;
    std::cout << "  Key Offset: " << ksds_config.key_offset << std::endl;
    std::cout << "  Max Record: " << ksds_config.max_record_size << " bytes" << std::endl;
    std::cout << "  Avg Record: " << ksds_config.avg_record_size << " bytes" << std::endl;
    std::cout << "  Initial Size: " << ksds_config.initial_size / (1024*1024) << " MB" << std::endl;
    std::cout << "  Compressed: " << (ksds_config.compressed ? "Yes" : "No") << std::endl;
    
    // =========================================================================
    // ESDS (Entry Sequenced Data Set) Configuration
    // =========================================================================
    print_separator("ESDS Configuration");
    
    VsamDatasetConfig esds_config("TRANSACTION.LOG.ESDS", VsamType::ESDS);
    esds_config.max_record_size = 32760;
    esds_config.avg_record_size = 500;
    esds_config.initial_size = 100 * 1024 * 1024;  // 100 MB
    esds_config.spanned_records = true;
    
    std::cout << "ESDS Dataset Configuration:" << std::endl;
    std::cout << "  Name: " << esds_config.name << std::endl;
    std::cout << "  Type: " << catalog::vsam_type_to_string(esds_config.type) << std::endl;
    std::cout << "  Max Record: " << esds_config.max_record_size << " bytes" << std::endl;
    std::cout << "  Avg Record: " << esds_config.avg_record_size << " bytes" << std::endl;
    std::cout << "  Spanned: " << (esds_config.spanned_records ? "Yes" : "No") << std::endl;
    
    // =========================================================================
    // RRDS (Relative Record Data Set) Configuration
    // =========================================================================
    print_separator("RRDS Configuration");
    
    VsamDatasetConfig rrds_config("EMPLOYEE.SLOTS.RRDS", VsamType::RRDS);
    rrds_config.max_record_size = 1024;
    rrds_config.avg_record_size = 1024;  // Fixed for RRDS
    rrds_config.initial_size = 50 * 1024 * 1024;
    
    std::cout << "RRDS Dataset Configuration:" << std::endl;
    std::cout << "  Name: " << rrds_config.name << std::endl;
    std::cout << "  Type: " << catalog::vsam_type_to_string(rrds_config.type) << std::endl;
    std::cout << "  Record Size: " << rrds_config.max_record_size << " bytes (fixed)" << std::endl;
    
    // =========================================================================
    // LDS (Linear Data Set) Configuration
    // =========================================================================
    print_separator("LDS Configuration");
    
    VsamDatasetConfig lds_config("DATABASE.TABLESPACE.LDS", VsamType::LDS);
    lds_config.initial_size = 1024 * 1024 * 1024;  // 1 GB
    lds_config.max_size = 10ULL * 1024 * 1024 * 1024;  // 10 GB max
    
    std::cout << "LDS Dataset Configuration:" << std::endl;
    std::cout << "  Name: " << lds_config.name << std::endl;
    std::cout << "  Type: " << catalog::vsam_type_to_string(lds_config.type) << std::endl;
    std::cout << "  Initial Size: " << lds_config.initial_size / (1024*1024) << " MB" << std::endl;
    std::cout << "  Max Size: " << lds_config.max_size / (1024*1024*1024) << " GB" << std::endl;
    
    // =========================================================================
    // Working with VSAM Keys
    // =========================================================================
    print_separator("VSAM Key Operations");
    
    // Create keys from different sources
    VsamKey key1("CUST0000000001");
    VsamKey key2("CUST0000000002");
    VsamKey key3(16);  // Empty key with size
    
    std::cout << "Key 1: '" << key1.to_string() << "' (length: " << key1.length << ")" << std::endl;
    std::cout << "Key 2: '" << key2.to_string() << "' (length: " << key2.length << ")" << std::endl;
    std::cout << "Key 3: (empty, length: " << key3.length << ")" << std::endl;
    
    // Key comparison
    std::cout << "Key1 == Key2: " << (key1 == key2 ? "true" : "false") << std::endl;
    std::cout << "Key1 < Key2: " << (key1 < key2 ? "true" : "false") << std::endl;
    
    // Create key from byte array
    Byte key_bytes[] = {'A', 'C', 'C', 'T', '0', '0', '0', '1'};
    VsamKey key4(key_bytes, sizeof(key_bytes));
    std::cout << "Key 4: '" << key4.to_string() << "'" << std::endl;
    
    // =========================================================================
    // Working with VSAM Records
    // =========================================================================
    print_separator("VSAM Record Operations");
    
    // Create a customer record
    CustomerRecord customer;
    std::strncpy(customer.customer_id, "CUST0000000001", sizeof(customer.customer_id));
    std::strncpy(customer.name, "John Smith", sizeof(customer.name));
    std::strncpy(customer.address, "123 Main Street", sizeof(customer.address));
    std::strncpy(customer.city, "New York", sizeof(customer.city));
    std::strncpy(customer.state, "NY", sizeof(customer.state));
    std::strncpy(customer.zip, "10001", sizeof(customer.zip));
    customer.balance = 15000.50;
    customer.transaction_count = 42;
    
    // Wrap in VsamRecord
    VsamRecord record(sizeof(CustomerRecord));
    std::memcpy(record.data.data(), &customer, sizeof(customer));
    record.length = sizeof(CustomerRecord);
    record.key = VsamKey(reinterpret_cast<const Byte*>(customer.customer_id), 16);
    
    std::cout << "KSDS Record:" << std::endl;
    std::cout << "  Key: " << record.key.to_string() << std::endl;
    std::cout << "  Length: " << record.length << " bytes" << std::endl;
    std::cout << "  Data size: " << record.data.size() << " bytes" << std::endl;
    
    // ESDS record with RBA
    VsamRecord esds_record(256);
    esds_record.length = 256;
    esds_record.rba = 0x00001000;  // RBA position
    
    std::cout << std::endl << "ESDS Record:" << std::endl;
    std::cout << "  RBA: 0x" << std::hex << esds_record.rba << std::dec << std::endl;
    std::cout << "  Length: " << esds_record.length << " bytes" << std::endl;
    
    // RRDS record with slot
    VsamRecord rrds_record(1024);
    rrds_record.length = 1024;
    rrds_record.slot = 42;  // Slot number
    
    std::cout << std::endl << "RRDS Record:" << std::endl;
    std::cout << "  Slot: " << rrds_record.slot << std::endl;
    std::cout << "  Length: " << rrds_record.length << " bytes" << std::endl;
    
    // =========================================================================
    // VSAM Statistics
    // =========================================================================
    print_separator("VSAM Statistics");
    
    VsamStatistics stats;
    stats.record_count = 10000;
    stats.allocated_space = 10 * 1024 * 1024;
    stats.used_space = 7 * 1024 * 1024;
    stats.read_operations = 50000;
    stats.write_operations = 10000;
    stats.update_operations = 5000;
    stats.delete_operations = 500;
    stats.splits = 25;
    stats.ci_splits = 20;
    stats.ca_splits = 5;
    
    std::cout << "Dataset Statistics:" << std::endl;
    std::cout << "  Record Count: " << stats.record_count << std::endl;
    std::cout << "  Space: " << stats.used_space / (1024*1024) << " / " 
              << stats.allocated_space / (1024*1024) << " MB" << std::endl;
    std::cout << "  Utilization: " << std::fixed << std::setprecision(1) 
              << stats.space_utilization() << "%" << std::endl;
    std::cout << std::endl << "  Operations:" << std::endl;
    std::cout << "    Reads: " << stats.read_operations << std::endl;
    std::cout << "    Writes: " << stats.write_operations << std::endl;
    std::cout << "    Updates: " << stats.update_operations << std::endl;
    std::cout << "    Deletes: " << stats.delete_operations << std::endl;
    std::cout << "    Total: " << stats.total_operations() << std::endl;
    std::cout << std::endl << "  Splits:" << std::endl;
    std::cout << "    CI Splits: " << stats.ci_splits << std::endl;
    std::cout << "    CA Splits: " << stats.ca_splits << std::endl;
    std::cout << "    Total Splits: " << stats.splits << std::endl;
    
    // =========================================================================
    // Return Code Demonstration
    // =========================================================================
    print_separator("VSAM Return Codes");
    
    Vector<VsamReturnCode> return_codes = {
        VsamReturnCode::SUCCESS,
        VsamReturnCode::END_OF_FILE,
        VsamReturnCode::DUPLICATE_KEY,
        VsamReturnCode::RECORD_NOT_FOUND,
        VsamReturnCode::INVALID_KEY,
        VsamReturnCode::DATASET_FULL
    };
    
    for (auto rc : return_codes) {
        std::cout << "  RC " << std::setw(2) << static_cast<int>(rc) 
                  << ": " << vsam_return_code_to_string(rc) << std::endl;
    }
    
    // =========================================================================
    // Simulated VSAM Operations
    // =========================================================================
    print_separator("Simulated VSAM Operations");
    
    std::cout << "KSDS Operations Simulation:" << std::endl;
    
    // Simulate GET by key
    std::cout << "  GET key='CUST0000000001' -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SUCCESS) << std::endl;
    
    // Simulate INSERT
    std::cout << "  INSERT key='CUST0000000003' -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SUCCESS) << std::endl;
    
    // Simulate duplicate INSERT
    std::cout << "  INSERT key='CUST0000000001' -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::DUPLICATE_KEY) << std::endl;
    
    // Simulate UPDATE
    std::cout << "  UPDATE key='CUST0000000001' -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SUCCESS) << std::endl;
    
    // Simulate DELETE
    std::cout << "  DELETE key='CUST0000000002' -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SUCCESS) << std::endl;
    
    // Simulate GET not found
    std::cout << "  GET key='CUST9999999999' -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::RECORD_NOT_FOUND) << std::endl;
    
    std::cout << std::endl << "ESDS Operations Simulation:" << std::endl;
    
    // ESDS sequential read
    std::cout << "  GET NEXT RBA=0x0000 -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SUCCESS) << std::endl;
    
    std::cout << "  GET NEXT RBA=0x1000 -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SUCCESS) << std::endl;
    
    std::cout << "  GET NEXT RBA=EOF -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::END_OF_FILE) << std::endl;
    
    std::cout << std::endl << "RRDS Operations Simulation:" << std::endl;
    
    // RRDS by slot
    std::cout << "  GET SLOT=42 -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SUCCESS) << std::endl;
    
    std::cout << "  GET SLOT=999 -> ";
    std::cout << vsam_return_code_to_string(VsamReturnCode::SLOT_NOT_FOUND) << std::endl;
    
    std::cout << std::endl << "=== VSAM Example Complete ===" << std::endl;
    
    return 0;
}
