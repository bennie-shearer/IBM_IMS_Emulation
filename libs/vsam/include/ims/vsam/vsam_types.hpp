#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - VSAM Types
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/catalog/catalog_types.hpp"

namespace ims::vsam {

using catalog::VsamType;
using catalog::RecordFormat;

// =============================================================================
// VSAM Return Codes
// =============================================================================

enum class VsamReturnCode : UInt8 {
    SUCCESS = 0,
    DUPLICATE_KEY = 8,
    RECORD_NOT_FOUND = 16,
    END_OF_FILE = 4,
    INVALID_KEY = 20,
    RBA_NOT_FOUND = 24,
    SLOT_NOT_FOUND = 28,
    DATASET_FULL = 12,
    INVALID_REQUEST = 32,
    IO_ERROR = 36,
    LOGICAL_ERROR = 40
};

// =============================================================================
// VSAM Dataset Configuration
// =============================================================================

struct VsamDatasetConfig {
    String name;
    VsamType type{VsamType::KSDS};
    RecordFormat recfm{RecordFormat::VARIABLE};
    UInt32 max_record_size{32760};
    UInt32 avg_record_size{500};
    UInt32 key_length{0};
    UInt32 key_offset{0};
    UInt64 initial_size{1024 * 1024};  // 1 MB default
    UInt64 max_size{0};                 // 0 = unlimited
    bool allow_duplicates{false};       // For KSDS alternate indexes
    bool spanned_records{false};
    bool compressed{false};
    String storage_class;
    String data_class;
    
    VsamDatasetConfig() = default;
    
    explicit VsamDatasetConfig(const String& ds_name, VsamType ds_type = VsamType::KSDS)
        : name(ds_name), type(ds_type) {
        if (type == VsamType::KSDS) {
            key_length = 8;  // Default key length
        }
    }
};

// =============================================================================
// VSAM Statistics
// =============================================================================

struct VsamStatistics {
    UInt64 record_count{0};
    UInt64 allocated_space{0};
    UInt64 used_space{0};
    UInt64 read_operations{0};
    UInt64 write_operations{0};
    UInt64 update_operations{0};
    UInt64 delete_operations{0};
    UInt64 splits{0};
    UInt64 ci_splits{0};
    UInt64 ca_splits{0};
    SystemTimePoint created;
    SystemTimePoint last_accessed;
    
    VsamStatistics() 
        : created(SystemClock::now())
        , last_accessed(SystemClock::now()) {}
    
    double space_utilization() const {
        return allocated_space > 0 ? 
               static_cast<double>(used_space) / static_cast<double>(allocated_space) * 100.0 : 0.0;
    }
    
    UInt64 total_operations() const {
        return read_operations + write_operations + update_operations + delete_operations;
    }
    
    double operations_per_second() const {
        auto seconds = std::chrono::duration_cast<Seconds>(
            last_accessed - created).count();
        return seconds > 0 ? 
               static_cast<double>(total_operations()) / static_cast<double>(seconds) : 0.0;
    }
};

// =============================================================================
// VSAM Key
// =============================================================================

struct VsamKey {
    ByteBuffer data;
    UInt32 length{0};
    
    VsamKey() = default;
    explicit VsamKey(Size size) : data(size), length(static_cast<UInt32>(size)) {}
    VsamKey(const Byte* ptr, Size size) : data(ptr, ptr + size), length(static_cast<UInt32>(size)) {}
    VsamKey(const String& str) : data(str.begin(), str.end()), length(static_cast<UInt32>(str.size())) {}
    
    bool operator==(const VsamKey& other) const { return data == other.data; }
    bool operator<(const VsamKey& other) const { return data < other.data; }
    
    String to_string() const { return String(data.begin(), data.end()); }
    bool empty() const { return data.empty(); }
};

// =============================================================================
// VSAM Record
// =============================================================================

struct VsamRecord {
    ByteBuffer data;
    UInt32 length{0};
    VsamKey key;
    UInt64 rba{0};      // Relative Byte Address (for ESDS)
    UInt32 slot{0};     // Slot number (for RRDS)
    
    VsamRecord() = default;
    explicit VsamRecord(Size size) : data(size), length(static_cast<UInt32>(size)) {}
    VsamRecord(const Byte* ptr, Size size) 
        : data(ptr, ptr + size), length(static_cast<UInt32>(size)) {}
    
    bool empty() const { return data.empty(); }
};

// =============================================================================
// Utility Functions
// =============================================================================

inline String vsam_return_code_to_string(VsamReturnCode rc) {
    switch (rc) {
        case VsamReturnCode::SUCCESS: return "SUCCESS";
        case VsamReturnCode::DUPLICATE_KEY: return "DUPLICATE_KEY";
        case VsamReturnCode::RECORD_NOT_FOUND: return "RECORD_NOT_FOUND";
        case VsamReturnCode::END_OF_FILE: return "END_OF_FILE";
        case VsamReturnCode::INVALID_KEY: return "INVALID_KEY";
        case VsamReturnCode::RBA_NOT_FOUND: return "RBA_NOT_FOUND";
        case VsamReturnCode::SLOT_NOT_FOUND: return "SLOT_NOT_FOUND";
        case VsamReturnCode::DATASET_FULL: return "DATASET_FULL";
        case VsamReturnCode::INVALID_REQUEST: return "INVALID_REQUEST";
        case VsamReturnCode::IO_ERROR: return "IO_ERROR";
        case VsamReturnCode::LOGICAL_ERROR: return "LOGICAL_ERROR";
        default: return "UNKNOWN";
    }
}

} // namespace ims::vsam
