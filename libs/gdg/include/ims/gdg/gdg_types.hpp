#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - GDG (Generation Data Group) Types
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/catalog/catalog_types.hpp"

namespace ims::gdg {

// =============================================================================
// GDG Enums
// =============================================================================

enum class GdgModel : UInt8 {
    FIFO = 1,      // First In First Out (oldest deleted first)
    LIFO = 2,      // Last In First Out (newest deleted first)
    EMPTY = 3,     // Delete all when limit exceeded
    NOEMPTY = 4    // Don't delete, just mark inactive
};

enum class GdgStatus : UInt8 {
    ACTIVE = 1,
    INACTIVE = 2,
    ROLLED_OFF = 3,
    DELETED = 4,
    PENDING = 5
};

// =============================================================================
// GDG Structures
// =============================================================================

struct GenerationDataset {
    String base_name;
    Int16 generation_number{0};   // Relative generation number
    UInt16 version_number{0};     // Version within generation
    String absolute_name;          // Full dataset name with Gnnnn
    GdgStatus status{GdgStatus::ACTIVE};
    UInt64 size_bytes{0};
    SystemTimePoint created;
    SystemTimePoint last_accessed;
    String volume;
    catalog::DatasetOrganization dsorg{catalog::DatasetOrganization::PS};
    
    GenerationDataset() 
        : created(SystemClock::now())
        , last_accessed(SystemClock::now()) {}
    
    String get_relative_name() const {
        if (generation_number >= 0) {
            return base_name + "(+" + std::to_string(generation_number) + ")";
        } else {
            return base_name + "(" + std::to_string(generation_number) + ")";
        }
    }
};

struct GdgBase {
    String name;
    UInt16 limit{255};            // Maximum generations (1-255)
    GdgModel model{GdgModel::FIFO};
    bool scratch{true};           // Scratch dataset when rolled off
    bool empty{false};            // Empty all when limit exceeded
    bool extended{false};         // Extended format (>255 generations)
    bool purge{false};            // Purge immediately vs. expire
    SystemTimePoint created;
    SystemTimePoint last_modified;
    Int16 current_generation{0};
    UInt16 active_generations{0};
    
    GdgBase() 
        : created(SystemClock::now())
        , last_modified(SystemClock::now()) {}
};

struct GdgCatalogEntry {
    GdgBase base;
    Vector<GenerationDataset> generations;
    Int16 current_generation{0};
    
    Optional<GenerationDataset> get_generation(Int16 relative_number) const {
        Int16 target_gen = static_cast<Int16>(current_generation + relative_number);
        for (const auto& gen : generations) {
            if (gen.generation_number == target_gen && gen.status == GdgStatus::ACTIVE) {
                return gen;
            }
        }
        return std::nullopt;
    }
    
    Optional<GenerationDataset> get_current() const {
        return get_generation(0);
    }
};

// =============================================================================
// GDG Statistics
// =============================================================================

struct GdgStatistics {
    UInt64 total_gdg_bases{0};
    UInt64 total_generations{0};
    UInt64 active_generations{0};
    UInt64 rolled_off_count{0};
    UInt64 total_space_used{0};
    UInt64 space_by_active_generations{0};
    SystemTimePoint last_updated;
    
    GdgStatistics() : last_updated(SystemClock::now()) {}
    
    double space_utilization() const {
        return total_space_used > 0 ?
               static_cast<double>(space_by_active_generations) / static_cast<double>(total_space_used) * 100.0 : 0.0;
    }
};

// =============================================================================
// Utility Functions
// =============================================================================

inline String gdg_model_to_string(GdgModel model) {
    switch (model) {
        case GdgModel::FIFO: return "FIFO";
        case GdgModel::LIFO: return "LIFO";
        case GdgModel::EMPTY: return "EMPTY";
        case GdgModel::NOEMPTY: return "NOEMPTY";
        default: return "UNKNOWN";
    }
}

inline String gdg_status_to_string(GdgStatus status) {
    switch (status) {
        case GdgStatus::ACTIVE: return "ACTIVE";
        case GdgStatus::INACTIVE: return "INACTIVE";
        case GdgStatus::ROLLED_OFF: return "ROLLED_OFF";
        case GdgStatus::DELETED: return "DELETED";
        case GdgStatus::PENDING: return "PENDING";
        default: return "UNKNOWN";
    }
}

} // namespace ims::gdg
