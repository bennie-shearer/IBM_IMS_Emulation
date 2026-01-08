#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Master Catalog
// Version: 3.6.2
// =============================================================================

#include "catalog_types.hpp"
#include "ims/common/logger.hpp"
#include <regex>

namespace ims::catalog {

// =============================================================================
// Master Catalog Class
// =============================================================================

class MasterCatalog {
private:
    String catalog_name_;
    Path catalog_path_;
    HashMap<String, CatalogEntry> entries_;
    HashMap<String, VolumeInfo> volumes_;
    CatalogStatistics statistics_;
    mutable SharedMutex mutex_;
    UInt64 operation_counter_{0};
    SharedPtr<ILogger> logger_;
    
public:
    // -------------------------------------------------------------------------
    // Constructors
    // -------------------------------------------------------------------------
    
    MasterCatalog() 
        : catalog_name_("MASTER")
        , catalog_path_("./catalog")
        , logger_(global_logger()) {}
    
    explicit MasterCatalog(const String& name, const Path& path = "./catalog")
        : catalog_name_(name)
        , catalog_path_(path)
        , logger_(global_logger()) {}
    
    ~MasterCatalog() = default;
    
    // Disable copy
    MasterCatalog(const MasterCatalog&) = delete;
    MasterCatalog& operator=(const MasterCatalog&) = delete;
    
    // Enable move
    MasterCatalog(MasterCatalog&&) noexcept = default;
    MasterCatalog& operator=(MasterCatalog&&) noexcept = default;
    
    // -------------------------------------------------------------------------
    // Catalog Entry Operations
    // -------------------------------------------------------------------------
    
    ErrorResult<void> define_entry(const CatalogEntry& entry) {
        UniqueLock<SharedMutex> lock(mutex_);
        
        if (entry.name.empty()) {
            return "Dataset name cannot be empty";
        }
        
        if (entry.name.length() > 44) {
            return "Dataset name exceeds 44 character limit";
        }
        
        if (entries_.contains(entry.name)) {
            return "Entry already exists: " + entry.name;
        }
        
        entries_[entry.name] = entry;
        ++operation_counter_;
        ++statistics_.total_entries;
        ++statistics_.dataset_entries;
        
        if (entry.is_vsam()) {
            ++statistics_.vsam_entries;
        }
        
        statistics_.total_space_allocated += entry.allocated_space;
        statistics_.last_updated = SystemClock::now();
        
        if (logger_) {
            logger_->info(std::format("Defined catalog entry: {}", entry.name));
        }
        
        return make_success();
    }
    
    ErrorResult<void> delete_entry(const String& name) {
        UniqueLock<SharedMutex> lock(mutex_);
        
        auto it = entries_.find(name);
        if (it == entries_.end()) {
            return "Entry not found: " + name;
        }
        
        statistics_.total_space_allocated -= it->second.allocated_space;
        statistics_.total_space_used -= it->second.used_space;
        
        if (it->second.is_vsam()) {
            --statistics_.vsam_entries;
        }
        
        entries_.erase(it);
        --statistics_.total_entries;
        --statistics_.dataset_entries;
        ++operation_counter_;
        statistics_.last_updated = SystemClock::now();
        
        if (logger_) {
            logger_->info(std::format("Deleted catalog entry: {}", name));
        }
        
        return make_success();
    }
    
    ErrorResult<CatalogEntry> get_entry(const String& name) const {
        SharedLock<SharedMutex> lock(mutex_);
        
        auto it = entries_.find(name);
        if (it == entries_.end()) {
            return ErrorResult<CatalogEntry>::make_error("Entry not found: " + name);
        }
        
        return it->second;
    }
    
    ErrorResult<void> update_entry(const CatalogEntry& entry) {
        UniqueLock<SharedMutex> lock(mutex_);
        
        auto it = entries_.find(entry.name);
        if (it == entries_.end()) {
            return "Entry not found: " + entry.name;
        }
        
        // Update space statistics
        statistics_.total_space_allocated -= it->second.allocated_space;
        statistics_.total_space_used -= it->second.used_space;
        
        it->second = entry;
        it->second.last_modified = SystemClock::now();
        
        statistics_.total_space_allocated += entry.allocated_space;
        statistics_.total_space_used += entry.used_space;
        ++operation_counter_;
        statistics_.last_updated = SystemClock::now();
        
        return make_success();
    }
    
    bool entry_exists(const String& name) const {
        SharedLock<SharedMutex> lock(mutex_);
        return entries_.contains(name);
    }
    
    // -------------------------------------------------------------------------
    // Listing and Search
    // -------------------------------------------------------------------------
    
    Vector<String> list_entries(const String& pattern = "*") const {
        SharedLock<SharedMutex> lock(mutex_);
        
        Vector<String> result;
        result.reserve(entries_.size());
        
        // Convert pattern to regex
        String regex_pattern = pattern;
        for (auto& c : regex_pattern) {
            if (c == '*') c = '.';
        }
        regex_pattern = "^" + regex_pattern + "$";
        
        try {
            std::regex filter(regex_pattern, std::regex::icase);
            
            for (const auto& [name, entry] : entries_) {
                if (pattern == "*" || std::regex_match(name, filter)) {
                    result.push_back(name);
                }
            }
        } catch (const std::regex_error&) {
            // If regex fails, do simple prefix match
            for (const auto& [name, entry] : entries_) {
                if (pattern == "*" || name.find(pattern) == 0) {
                    result.push_back(name);
                }
            }
        }
        
        std::sort(result.begin(), result.end());
        return result;
    }
    
    Vector<CatalogEntry> get_entries_by_organization(DatasetOrganization dsorg) const {
        SharedLock<SharedMutex> lock(mutex_);
        
        Vector<CatalogEntry> result;
        for (const auto& [name, entry] : entries_) {
            if (entry.dsorg == dsorg) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
    Vector<CatalogEntry> get_vsam_entries(VsamType type = VsamType::NONE) const {
        SharedLock<SharedMutex> lock(mutex_);
        
        Vector<CatalogEntry> result;
        for (const auto& [name, entry] : entries_) {
            if (entry.is_vsam()) {
                if (type == VsamType::NONE || entry.vsam_type == type) {
                    result.push_back(entry);
                }
            }
        }
        return result;
    }
    
    // -------------------------------------------------------------------------
    // Volume Operations
    // -------------------------------------------------------------------------
    
    ErrorResult<void> define_volume(const VolumeInfo& volume) {
        UniqueLock<SharedMutex> lock(mutex_);
        
        if (volume.volser.empty()) {
            return "Volume serial cannot be empty";
        }
        
        if (volumes_.contains(volume.volser)) {
            return "Volume already defined: " + volume.volser;
        }
        
        volumes_[volume.volser] = volume;
        ++operation_counter_;
        
        return make_success();
    }
    
    ErrorResult<VolumeInfo> get_volume(const String& volser) const {
        SharedLock<SharedMutex> lock(mutex_);
        
        auto it = volumes_.find(volser);
        if (it == volumes_.end()) {
            return ErrorResult<VolumeInfo>::make_error("Volume not found: " + volser);
        }
        
        return it->second;
    }
    
    Vector<String> list_volumes() const {
        SharedLock<SharedMutex> lock(mutex_);
        
        Vector<String> result;
        result.reserve(volumes_.size());
        for (const auto& [volser, info] : volumes_) {
            result.push_back(volser);
        }
        std::sort(result.begin(), result.end());
        return result;
    }
    
    // -------------------------------------------------------------------------
    // Statistics and Information
    // -------------------------------------------------------------------------
    
    CatalogStatistics get_statistics() const {
        SharedLock<SharedMutex> lock(mutex_);
        CatalogStatistics stats = statistics_;
        stats.operations_count = operation_counter_;
        return stats;
    }
    
    Size entry_count() const {
        SharedLock<SharedMutex> lock(mutex_);
        return entries_.size();
    }
    
    Size volume_count() const {
        SharedLock<SharedMutex> lock(mutex_);
        return volumes_.size();
    }
    
    // Inline getters (no separate .cpp implementation needed)
    String get_catalog_name() const { return catalog_name_; }
    Path get_catalog_path() const { return catalog_path_; }
    UInt64 get_operation_count() const { return operation_counter_; }
    
    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------
    
    static bool is_valid_dataset_name(const String& name) {
        if (name.empty() || name.length() > 44) {
            return false;
        }
        
        // Check for valid characters (simplified)
        for (char c : name) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && 
                c != '.' && c != '@' && c != '#' && c != '$') {
                return false;
            }
        }
        
        return true;
    }
    
    String generate_unique_name(const String& base_name) const {
        return base_name + ".G" + std::to_string(operation_counter_);
    }
    
    void set_logger(SharedPtr<ILogger> logger) {
        logger_ = std::move(logger);
    }
};

} // namespace ims::catalog
