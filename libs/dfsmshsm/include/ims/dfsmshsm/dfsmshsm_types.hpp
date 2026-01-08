#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - DFSMShsm Types
// Version: 3.6.2
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/catalog/catalog_types.hpp"

namespace ims::dfsmshsm {

// Import types from catalog namespace
using catalog::RecordFormat;
using catalog::SpaceUnit;

// =============================================================================
// Storage Management Types
// =============================================================================

enum class StorageLevel : UInt8 {
    LEVEL0 = 0,    // Primary storage (DASD)
    LEVEL1 = 1,    // Secondary storage (faster tape/optical)
    LEVEL2 = 2,    // Tertiary storage (slower tape/optical)
    MIGRATED = 3,  // Migrated/archived
    BACKUP = 4,    // Backup storage
    DELETED = 5    // Logically deleted
};

enum class MigrationStatus : UInt8 {
    RESIDENT = 1,      // On primary storage
    MIGRATED = 2,      // Migrated to HSM
    PARTIALLY_MIGRATED = 3,  // Some extents migrated
    RECALLED = 4,      // Recently recalled
    BACKUP_ONLY = 5    // Only backup exists
};

enum class CompressionType : UInt8 {
    NONE = 0,
    BASIC = 1,
    EXTENDED = 2,
    COMPACTION = 3,
    HARDWARE = 4
};

// =============================================================================
// Storage Classes and Policies
// =============================================================================

struct StorageClass {
    String name;
    String description;
    StorageLevel default_level{StorageLevel::LEVEL0};
    bool auto_migration_enabled{true};
    Duration migration_threshold{Hours(24 * 30)};  // 30 days
    UInt32 allocation_unit_size{4096};
    CompressionType compression{CompressionType::BASIC};
    Vector<String> storage_groups;
    
    StorageClass() = default;
};

struct ManagementClass {
    String name;
    String description;
    Duration primary_days{Hours(24 * 30)};      // 30 days
    Duration migration_days{Hours(24 * 365)};   // 1 year
    Duration backup_frequency{Hours(24 * 7)};   // Weekly
    UInt32 backup_versions{7};
    bool auto_backup_enabled{true};
    bool auto_migrate_enabled{true};
    bool auto_delete_enabled{false};
    
    ManagementClass() = default;
};

struct DataClass {
    String name;
    String description;
    RecordFormat record_format{RecordFormat::VARIABLE};
    UInt32 record_size{4096};
    UInt32 key_length{0};
    UInt32 key_offset{0};
    SpaceUnit space_unit{SpaceUnit::TRACKS};
    UInt32 primary_space{10};
    UInt32 secondary_space{5};
    
    DataClass() = default;
};

// =============================================================================
// Storage Statistics
// =============================================================================

struct StorageStatistics {
    UInt64 total_datasets{0};
    UInt64 resident_datasets{0};
    UInt64 migrated_datasets{0};
    UInt64 backup_datasets{0};
    
    UInt64 total_space_bytes{0};
    UInt64 used_space_bytes{0};
    UInt64 migrated_space_bytes{0};
    UInt64 compressed_space_bytes{0};
    
    UInt64 migration_operations{0};
    UInt64 recall_operations{0};
    UInt64 backup_operations{0};
    
    double space_saved_percent{0.0};
    double migration_success_rate{100.0};
    SystemTimePoint last_updated;
    
    StorageStatistics() : last_updated(SystemClock::now()) {}
    
    double utilization_percent() const {
        return total_space_bytes > 0 ? 
               static_cast<double>(used_space_bytes) / static_cast<double>(total_space_bytes) * 100.0 : 0.0;
    }
    
    void update_utilization() {
        if (total_space_bytes > 0) {
            space_saved_percent = static_cast<double>(total_space_bytes - used_space_bytes) / 
                                  static_cast<double>(total_space_bytes) * 100.0;
        }
        last_updated = SystemClock::now();
    }
};

// =============================================================================
// Utility Functions
// =============================================================================

inline String storage_level_to_string(StorageLevel level) {
    switch (level) {
        case StorageLevel::LEVEL0: return "LEVEL0";
        case StorageLevel::LEVEL1: return "LEVEL1";
        case StorageLevel::LEVEL2: return "LEVEL2";
        case StorageLevel::MIGRATED: return "MIGRATED";
        case StorageLevel::BACKUP: return "BACKUP";
        case StorageLevel::DELETED: return "DELETED";
        default: return "UNKNOWN";
    }
}

inline String migration_status_to_string(MigrationStatus status) {
    switch (status) {
        case MigrationStatus::RESIDENT: return "RESIDENT";
        case MigrationStatus::MIGRATED: return "MIGRATED";
        case MigrationStatus::PARTIALLY_MIGRATED: return "PARTIAL";
        case MigrationStatus::RECALLED: return "RECALLED";
        case MigrationStatus::BACKUP_ONLY: return "BACKUP_ONLY";
        default: return "UNKNOWN";
    }
}

} // namespace ims::dfsmshsm
