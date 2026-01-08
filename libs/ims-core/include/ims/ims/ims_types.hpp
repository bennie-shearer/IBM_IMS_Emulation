#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - IMS Database Types
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/vsam/vsam_types.hpp"

namespace ims::imsdb {

// =============================================================================
// IMS Database Types
// =============================================================================

enum class DatabaseType : UInt8 {
    HIDAM = 1,     // Hierarchical Indexed Direct Access Method
    HDAM = 2,      // Hierarchical Direct Access Method
    HISAM = 3,     // Hierarchical Indexed Sequential Access Method
    HSAM = 4,      // Hierarchical Sequential Access Method
    SHISAM = 5,    // Simple Hierarchical Indexed Sequential Access Method
    SHSAM = 6,     // Simple Hierarchical Sequential Access Method
    INDEX = 7,     // Secondary Index
    HALDB = 8,     // High Availability Large Database
    GSAM = 9,      // Generalized Sequential Access Method
    DEDB = 10      // Data Entry Database (Fast Path)
};

enum class SegmentType : UInt8 {
    ROOT = 1,      // Root segment
    CHILD = 2,     // Child segment
    LOGICAL_CHILD = 3  // Logical child segment
};

enum class AccessMethod : UInt8 {
    SEQUENTIAL = 1,
    RANDOM = 2,
    DYNAMIC = 3
};

// =============================================================================
// DL/I Call Types and Status Codes
// =============================================================================

enum class DliCall : UInt8 {
    // Get calls
    GU = 1,        // Get Unique
    GN = 2,        // Get Next
    GNP = 3,       // Get Next within Parent
    GHU = 4,       // Get Hold Unique (NEW)
    GHN = 5,       // Get Hold Next (NEW)
    GHNP = 6,      // Get Hold Next within Parent (NEW)
    
    // Update calls
    ISRT = 10,     // Insert
    DLET = 11,     // Delete
    REPL = 12,     // Replace
    
    // System calls
    CHKP = 20,     // Checkpoint
    XRST = 21,     // Restart
    ROLB = 22,     // Rollback
    ROLL = 23,     // Roll (forward recovery)
    SYNC = 24,     // Sync Point (NEW)
    
    // Positioning calls
    SETS = 30,     // Set parentage
    SETO = 31,     // Set options
    POS = 32,      // Position (NEW)
    
    // Fast Path calls (NEW)
    FLD = 40,      // Field call
    
    // GSAM calls (NEW)
    OPEN = 50,
    CLSE = 51
};

enum class DliStatusCode : UInt16 {
    // Normal completion
    NORMAL = 0x0000,              // Normal completion (blank status)
    
    // Informational (GB, GE, etc.)
    END_OF_DATABASE = 0x4742,     // GB - End of database
    SEGMENT_NOT_FOUND = 0x4745,   // GE - Segment not found
    NO_MORE_SEGMENTS = 0x474B,    // GK - No more segments at this level (NEW)
    
    // Warning conditions  
    DUPLICATE_INSERT = 0x4949,    // II - Duplicate insert attempt
    INVALID_SSA = 0x4149,         // AI - Invalid SSA
    
    // Error conditions
    NOT_AUTHORIZED = 0x414A,      // AJ - Not authorized for database
    DATABASE_NOT_AVAILABLE = 0x4241,  // BA - Database not available
    DEADLOCK_OCCURRED = 0x4645,   // FE - Deadlock occurred
    INVALID_FUNCTION = 0x4146,    // AF - Invalid function code
    NO_HOLD = 0x444A,             // DJ - Delete/Replace without prior Get Hold (NEW)
    SEGMENT_CHANGED = 0x4441,     // DA - Segment changed since Get Hold (NEW)
    
    // I/O errors
    IO_ERROR = 0x4158,            // AX - I/O error (NEW)
    
    // System errors
    SYSTEM_ERROR = 0x5345,        // SE - System error
    ABEND_OCCURRED = 0x4142       // AB - Program abend
};

// =============================================================================
// Database Definitions
// =============================================================================

struct SegmentDefinition {
    String name;
    SegmentType type{SegmentType::ROOT};
    UInt32 length{0};
    String parent_name;
    Vector<String> child_names;
    UInt32 key_offset{0};
    UInt32 key_length{0};
    bool has_logical_relationship{false};
    
    SegmentDefinition() = default;
};

struct DatabaseDefinition {
    String name;
    DatabaseType type{DatabaseType::HIDAM};
    AccessMethod access_method{AccessMethod::RANDOM};
    Vector<SegmentDefinition> segments;
    String index_name;
    String vsam_dataset;
    UInt32 max_segments{1000};
    bool is_partitioned{false};
    
    DatabaseDefinition() = default;
};

// =============================================================================
// DL/I Request and Response
// =============================================================================

struct DliRequest {
    DliCall function_code{DliCall::GU};
    String pcb_name;
    Vector<String> ssa;          // Segment Search Arguments
    ByteBuffer io_area;
    UInt32 io_area_length{0};
    
    DliRequest() = default;
};

struct DliResponse {
    DliStatusCode status{DliStatusCode::NORMAL};
    ByteBuffer segment_data;
    String segment_name;
    UInt32 segment_level{0};
    UInt64 processing_time_ns{0};
    
    bool is_success() const { return status == DliStatusCode::NORMAL; }
};

// =============================================================================
// IMS Statistics
// =============================================================================

struct ImsStatistics {
    UInt64 total_transactions{0};
    UInt64 successful_transactions{0};
    UInt64 failed_transactions{0};
    UInt64 get_calls{0};
    UInt64 insert_calls{0};
    UInt64 delete_calls{0};
    UInt64 replace_calls{0};
    UInt64 checkpoint_calls{0};
    SystemTimePoint start_time;
    SystemTimePoint last_activity;
    
    ImsStatistics() 
        : start_time(SystemClock::now())
        , last_activity(SystemClock::now()) {}
    
    double success_rate() const {
        return total_transactions > 0 ?
               static_cast<double>(successful_transactions) / static_cast<double>(total_transactions) * 100.0 : 100.0;
    }
};

// =============================================================================
// Utility Functions
// =============================================================================

inline String database_type_to_string(DatabaseType type) {
    switch (type) {
        case DatabaseType::HIDAM: return "HIDAM";
        case DatabaseType::HDAM: return "HDAM";
        case DatabaseType::HISAM: return "HISAM";
        case DatabaseType::HSAM: return "HSAM";
        case DatabaseType::SHISAM: return "SHISAM";
        case DatabaseType::SHSAM: return "SHSAM";
        case DatabaseType::INDEX: return "INDEX";
        case DatabaseType::HALDB: return "HALDB";
        case DatabaseType::GSAM: return "GSAM";
        case DatabaseType::DEDB: return "DEDB";
        default: return "UNKNOWN";
    }
}

inline String access_method_to_string(AccessMethod method) {
    switch (method) {
        case AccessMethod::SEQUENTIAL: return "SEQUENTIAL";
        case AccessMethod::RANDOM: return "RANDOM";
        case AccessMethod::DYNAMIC: return "DYNAMIC";
        default: return "UNKNOWN";
    }
}

inline String dli_call_to_string(DliCall call) {
    switch (call) {
        // Get calls
        case DliCall::GU: return "GU";
        case DliCall::GN: return "GN";
        case DliCall::GNP: return "GNP";
        case DliCall::GHU: return "GHU";
        case DliCall::GHN: return "GHN";
        case DliCall::GHNP: return "GHNP";
        // Update calls
        case DliCall::ISRT: return "ISRT";
        case DliCall::DLET: return "DLET";
        case DliCall::REPL: return "REPL";
        // System calls
        case DliCall::CHKP: return "CHKP";
        case DliCall::XRST: return "XRST";
        case DliCall::ROLB: return "ROLB";
        case DliCall::ROLL: return "ROLL";
        case DliCall::SYNC: return "SYNC";
        // Positioning calls
        case DliCall::SETS: return "SETS";
        case DliCall::SETO: return "SETO";
        case DliCall::POS: return "POS";
        // Fast Path
        case DliCall::FLD: return "FLD";
        // GSAM
        case DliCall::OPEN: return "OPEN";
        case DliCall::CLSE: return "CLSE";
        default: return "UNKNOWN";
    }
}

inline String dli_status_to_string(DliStatusCode status) {
    switch (status) {
        case DliStatusCode::NORMAL: return "  ";
        case DliStatusCode::END_OF_DATABASE: return "GB";
        case DliStatusCode::SEGMENT_NOT_FOUND: return "GE";
        case DliStatusCode::NO_MORE_SEGMENTS: return "GK";
        case DliStatusCode::DUPLICATE_INSERT: return "II";
        case DliStatusCode::INVALID_SSA: return "AI";
        case DliStatusCode::NOT_AUTHORIZED: return "AJ";
        case DliStatusCode::DATABASE_NOT_AVAILABLE: return "BA";
        case DliStatusCode::DEADLOCK_OCCURRED: return "FE";
        case DliStatusCode::INVALID_FUNCTION: return "AF";
        case DliStatusCode::NO_HOLD: return "DJ";
        case DliStatusCode::SEGMENT_CHANGED: return "DA";
        case DliStatusCode::IO_ERROR: return "AX";
        case DliStatusCode::SYSTEM_ERROR: return "SE";
        case DliStatusCode::ABEND_OCCURRED: return "AB";
        default: return "??";
    }
}

/// Check if DL/I call is a Get Hold variant
inline bool is_get_hold_call(DliCall call) {
    return call == DliCall::GHU || call == DliCall::GHN || call == DliCall::GHNP;
}

/// Check if DL/I call requires prior Get Hold
inline bool requires_prior_hold(DliCall call) {
    return call == DliCall::DLET || call == DliCall::REPL;
}

} // namespace ims::imsdb
