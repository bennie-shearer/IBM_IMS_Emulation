#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Catalog Types
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"

namespace ims::catalog {

// =============================================================================
// Dataset Organization Types
// =============================================================================

enum class DatasetOrganization : UInt8 {
    UNKNOWN = 0,
    PS = 1,        // Physical Sequential
    PO = 2,        // Partitioned Organization
    DA = 3,        // Direct Access
    IS = 4,        // Indexed Sequential
    VS = 5,        // Virtual Storage
    VSAM = 6,      // Virtual Storage Access Method
    HFS = 7,       // Hierarchical File System
    PDSE = 8,      // Partitioned Dataset Extended
    ZFS = 9        // z/OS File System
};

enum class VsamType : UInt8 {
    NONE = 0,
    KSDS = 1,      // Key Sequenced Data Set
    ESDS = 2,      // Entry Sequenced Data Set
    RRDS = 3,      // Relative Record Data Set
    LDS = 4,       // Linear Data Set
    VRRDS = 5      // Variable Relative Record Data Set
};

enum class RecordFormat : UInt8 {
    FIXED = 1,     // F - Fixed
    VARIABLE = 2,  // V - Variable
    UNDEFINED = 3, // U - Undefined
    FIXED_BLOCKED = 4,    // FB - Fixed Blocked
    VARIABLE_BLOCKED = 5, // VB - Variable Blocked
    VARIABLE_SPANNED = 6  // VS - Variable Spanned
};

enum class SpaceUnit : UInt8 {
    TRACKS = 1,
    CYLINDERS = 2,
    BLOCKS = 3,
    KILOBYTES = 4,
    MEGABYTES = 5,
    GIGABYTES = 6
};

// =============================================================================
// Volume Types
// =============================================================================

enum class VolumeType : UInt8 {
    DASD = 1,      // Direct Access Storage Device
    TAPE = 2,      // Tape Volume
    OPTICAL = 3,   // Optical Media
    VIRTUAL = 4    // Virtual Volume
};

enum class VolumeStatus : UInt8 {
    ONLINE = 1,
    OFFLINE = 2,
    PENDING = 3,
    ERROR = 4,
    MAINTENANCE = 5
};

// =============================================================================
// Volume Information
// =============================================================================

struct VolumeInfo {
    String volser;         // Volume serial number
    VolumeType type{VolumeType::DASD};
    VolumeStatus status{VolumeStatus::ONLINE};
    UInt64 total_space{0};
    UInt64 used_space{0};
    UInt64 free_space{0};
    String device_type;
    String storage_group;
    SystemTimePoint mount_time;
    bool is_mounted{false};
    
    VolumeInfo() : mount_time(SystemClock::now()) {}
    
    double utilization_percent() const {
        return total_space > 0 ? 
               static_cast<double>(used_space) / static_cast<double>(total_space) * 100.0 : 0.0;
    }
};

// =============================================================================
// Catalog Entry
// =============================================================================

struct CatalogEntry {
    String name;                    // Dataset name (up to 44 chars)
    String alias;                   // Alternate name
    DatasetOrganization dsorg{DatasetOrganization::PS};
    VsamType vsam_type{VsamType::NONE};
    RecordFormat recfm{RecordFormat::FIXED_BLOCKED};
    UInt32 lrecl{80};              // Logical record length
    UInt32 blksize{27920};         // Block size
    UInt32 keylen{0};              // Key length for VSAM
    UInt32 keyoff{0};              // Key offset
    String volser;                  // Volume serial
    String device_type;
    String storage_class;
    String management_class;
    String data_class;
    UInt64 allocated_space{0};
    UInt64 used_space{0};
    SystemTimePoint created;
    SystemTimePoint last_referenced;
    SystemTimePoint last_modified;
    SystemTimePoint expiration;
    String owner;
    bool is_temporary{false};
    bool is_migrated{false};
    bool is_archived{false};
    
    CatalogEntry() 
        : created(SystemClock::now())
        , last_referenced(SystemClock::now())
        , last_modified(SystemClock::now())
        , expiration(SystemClock::now() + Hours(24 * 365 * 10)) {} // 10 years
    
    bool is_vsam() const { return dsorg == DatasetOrganization::VSAM; }
    bool is_pds() const { return dsorg == DatasetOrganization::PO || dsorg == DatasetOrganization::PDSE; }
};

// =============================================================================
// Allocation Parameters
// =============================================================================

struct AllocationParams {
    String dataset_name;
    DatasetOrganization dsorg{DatasetOrganization::PS};
    VsamType vsam_type{VsamType::NONE};
    RecordFormat recfm{RecordFormat::FIXED_BLOCKED};
    UInt32 lrecl{80};
    UInt32 blksize{27920};
    UInt32 keylen{0};
    UInt32 keyoff{0};
    SpaceUnit space_unit{SpaceUnit::TRACKS};
    UInt32 primary_space{10};
    UInt32 secondary_space{5};
    UInt32 directory_blocks{0};     // For PDS
    String volume;
    String storage_class;
    String management_class;
    String data_class;
    bool is_temporary{false};
    Duration expiration{Hours(24 * 365)};  // Default 1 year
};

// =============================================================================
// Catalog Statistics
// =============================================================================

struct CatalogStatistics {
    UInt64 total_entries{0};
    UInt64 dataset_entries{0};
    UInt64 alias_entries{0};
    UInt64 vsam_entries{0};
    UInt64 total_space_allocated{0};
    UInt64 total_space_used{0};
    UInt64 operations_count{0};
    SystemTimePoint last_updated;
    
    CatalogStatistics() : last_updated(SystemClock::now()) {}
    
    double space_utilization() const {
        return total_space_allocated > 0 ?
               static_cast<double>(total_space_used) / static_cast<double>(total_space_allocated) * 100.0 : 0.0;
    }
};

// =============================================================================
// Utility Functions
// =============================================================================

inline String dsorg_to_string(DatasetOrganization dsorg) {
    switch (dsorg) {
        case DatasetOrganization::PS: return "PS";
        case DatasetOrganization::PO: return "PO";
        case DatasetOrganization::DA: return "DA";
        case DatasetOrganization::IS: return "IS";
        case DatasetOrganization::VS: return "VS";
        case DatasetOrganization::VSAM: return "VSAM";
        case DatasetOrganization::HFS: return "HFS";
        case DatasetOrganization::PDSE: return "PDSE";
        case DatasetOrganization::ZFS: return "ZFS";
        default: return "UNKNOWN";
    }
}

inline String vsam_type_to_string(VsamType type) {
    switch (type) {
        case VsamType::KSDS: return "KSDS";
        case VsamType::ESDS: return "ESDS";
        case VsamType::RRDS: return "RRDS";
        case VsamType::LDS: return "LDS";
        case VsamType::VRRDS: return "VRRDS";
        default: return "NONE";
    }
}

inline String recfm_to_string(RecordFormat recfm) {
    switch (recfm) {
        case RecordFormat::FIXED: return "F";
        case RecordFormat::VARIABLE: return "V";
        case RecordFormat::UNDEFINED: return "U";
        case RecordFormat::FIXED_BLOCKED: return "FB";
        case RecordFormat::VARIABLE_BLOCKED: return "VB";
        case RecordFormat::VARIABLE_SPANNED: return "VS";
        default: return "?";
    }
}

} // namespace ims::catalog
