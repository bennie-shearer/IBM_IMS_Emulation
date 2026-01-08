// =============================================================================
// IBM IMS Emulation Enterprise - GDG Example
// Version: 3.6.2
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/gdg/gdg_types.hpp"

#include <iostream>
#include <iomanip>

using namespace ims;
using namespace ims::gdg;

/**
 * @brief Demonstrates Generation Data Group (GDG) operations
 * 
 * This example shows:
 * - GDG base definition
 * - Generation creation and management
 * - FIFO/LIFO rolloff models
 * - Relative generation numbering
 */

void print_separator(const char* title) {
    std::cout << std::endl << "--- " << title << " ---" << std::endl;
}

int main() {
    std::cout << "=== IMS GDG Example ===" << std::endl;
    
    // =========================================================================
    // GDG Model Types
    // =========================================================================
    print_separator("GDG Model Types");
    
    Vector<GdgModel> models = {
        GdgModel::FIFO,
        GdgModel::LIFO,
        GdgModel::EMPTY,
        GdgModel::NOEMPTY
    };
    
    for (auto model : models) {
        std::cout << "  " << std::setw(8) << gdg_model_to_string(model) << " - ";
        switch (model) {
            case GdgModel::FIFO:
                std::cout << "First In First Out (oldest deleted when limit reached)";
                break;
            case GdgModel::LIFO:
                std::cout << "Last In First Out (newest deleted when limit reached)";
                break;
            case GdgModel::EMPTY:
                std::cout << "Delete all generations when limit exceeded";
                break;
            case GdgModel::NOEMPTY:
                std::cout << "Mark inactive but don't delete";
                break;
        }
        std::cout << std::endl;
    }
    
    // =========================================================================
    // GDG Base Definition
    // =========================================================================
    print_separator("GDG Base Definition");
    
    GdgBase daily_backup;
    daily_backup.name = "PROD.BACKUP.DAILY";
    daily_backup.limit = 7;  // Keep 7 generations (one week)
    daily_backup.model = GdgModel::FIFO;
    daily_backup.scratch = true;
    daily_backup.empty = false;
    daily_backup.extended = false;
    daily_backup.purge = false;
    daily_backup.current_generation = 5;
    daily_backup.active_generations = 5;
    
    std::cout << "Daily Backup GDG:" << std::endl;
    std::cout << "  Name: " << daily_backup.name << std::endl;
    std::cout << "  Limit: " << daily_backup.limit << " generations" << std::endl;
    std::cout << "  Model: " << gdg_model_to_string(daily_backup.model) << std::endl;
    std::cout << "  Scratch: " << (daily_backup.scratch ? "Yes" : "No") << std::endl;
    std::cout << "  Empty: " << (daily_backup.empty ? "Yes" : "No") << std::endl;
    std::cout << "  Extended: " << (daily_backup.extended ? "Yes" : "No") << std::endl;
    std::cout << "  Current Generation: G" << std::setw(4) << std::setfill('0') 
              << daily_backup.current_generation << "V00" << std::setfill(' ') << std::endl;
    std::cout << "  Active Generations: " << daily_backup.active_generations << std::endl;
    
    // Monthly archive with higher limit
    GdgBase monthly_archive;
    monthly_archive.name = "PROD.ARCHIVE.MONTHLY";
    monthly_archive.limit = 24;  // Keep 24 months (2 years)
    monthly_archive.model = GdgModel::FIFO;
    monthly_archive.scratch = false;  // Don't automatically delete
    monthly_archive.extended = true;  // Allow >255 generations
    monthly_archive.current_generation = 18;
    monthly_archive.active_generations = 18;
    
    std::cout << std::endl << "Monthly Archive GDG:" << std::endl;
    std::cout << "  Name: " << monthly_archive.name << std::endl;
    std::cout << "  Limit: " << monthly_archive.limit << " generations" << std::endl;
    std::cout << "  Model: " << gdg_model_to_string(monthly_archive.model) << std::endl;
    std::cout << "  Extended: " << (monthly_archive.extended ? "Yes" : "No") << std::endl;
    
    // =========================================================================
    // Generation Datasets
    // =========================================================================
    print_separator("Generation Datasets");
    
    // Create generations for daily backup
    GdgCatalogEntry daily_catalog;
    daily_catalog.base = daily_backup;
    daily_catalog.current_generation = 5;
    
    for (int i = 1; i <= 5; ++i) {
        GenerationDataset gen;
        gen.base_name = daily_backup.name;
        gen.generation_number = static_cast<Int16>(i);
        gen.version_number = 0;
        gen.absolute_name = daily_backup.name + ".G" + 
                           std::to_string(1000 + i) + "V00";
        gen.status = GdgStatus::ACTIVE;
        gen.size_bytes = static_cast<UInt64>(100 + i * 10) * 1024 * 1024;  // 110-150 MB
        gen.volume = "VOL001";
        gen.dsorg = catalog::DatasetOrganization::PS;
        
        daily_catalog.generations.push_back(gen);
    }
    
    std::cout << "Daily Backup Generations:" << std::endl;
    std::cout << "  " << std::setw(40) << std::left << "Absolute Name" 
              << std::setw(15) << "Relative" 
              << std::setw(10) << "Status"
              << std::setw(12) << "Size" << std::endl;
    std::cout << "  " << std::string(77, '-') << std::endl;
    
    for (const auto& gen : daily_catalog.generations) {
        std::cout << "  " << std::setw(40) << std::left << gen.absolute_name
                  << std::setw(15) << gen.get_relative_name()
                  << std::setw(10) << gdg_status_to_string(gen.status)
                  << std::setw(12) << std::to_string(gen.size_bytes / (1024*1024)) + " MB"
                  << std::endl;
    }
    
    // =========================================================================
    // Relative Generation Numbering
    // =========================================================================
    print_separator("Relative Generation Numbering");
    
    std::cout << "Given current generation = G0005V00:" << std::endl;
    std::cout << std::endl;
    std::cout << "  Reference      Resolves To     Description" << std::endl;
    std::cout << "  " << std::string(55, '-') << std::endl;
    std::cout << "  (+0)           G0005V00        Current generation" << std::endl;
    std::cout << "  (-1)           G0004V00        Previous generation" << std::endl;
    std::cout << "  (-2)           G0003V00        Two generations back" << std::endl;
    std::cout << "  (+1)           G0006V00        Next (new) generation" << std::endl;
    
    // Demonstrate get_generation
    std::cout << std::endl << "Using GdgCatalogEntry::get_generation():" << std::endl;
    
    auto current = daily_catalog.get_current();
    if (current) {
        std::cout << "  get_current() -> " << current->absolute_name << std::endl;
    }
    
    auto prev = daily_catalog.get_generation(-1);
    if (prev) {
        std::cout << "  get_generation(-1) -> " << prev->absolute_name << std::endl;
    }
    
    auto oldest = daily_catalog.get_generation(-4);
    if (oldest) {
        std::cout << "  get_generation(-4) -> " << oldest->absolute_name << std::endl;
    }
    
    // =========================================================================
    // GDG Status Types
    // =========================================================================
    print_separator("GDG Status Types");
    
    Vector<GdgStatus> statuses = {
        GdgStatus::ACTIVE,
        GdgStatus::INACTIVE,
        GdgStatus::ROLLED_OFF,
        GdgStatus::DELETED,
        GdgStatus::PENDING
    };
    
    for (auto status : statuses) {
        std::cout << "  " << std::setw(12) << gdg_status_to_string(status) << " - ";
        switch (status) {
            case GdgStatus::ACTIVE:
                std::cout << "Generation is current and accessible";
                break;
            case GdgStatus::INACTIVE:
                std::cout << "Generation exists but not in active set";
                break;
            case GdgStatus::ROLLED_OFF:
                std::cout << "Generation exceeded limit and was removed from catalog";
                break;
            case GdgStatus::DELETED:
                std::cout << "Generation has been explicitly deleted";
                break;
            case GdgStatus::PENDING:
                std::cout << "Generation is being created";
                break;
        }
        std::cout << std::endl;
    }
    
    // =========================================================================
    // GDG Statistics
    // =========================================================================
    print_separator("GDG Statistics");
    
    GdgStatistics stats;
    stats.total_gdg_bases = 15;
    stats.total_generations = 127;
    stats.active_generations = 105;
    stats.rolled_off_count = 22;
    stats.total_space_used = 50ULL * 1024 * 1024 * 1024;  // 50 GB
    stats.space_by_active_generations = 45ULL * 1024 * 1024 * 1024;  // 45 GB
    
    std::cout << "System GDG Statistics:" << std::endl;
    std::cout << "  Total GDG Bases: " << stats.total_gdg_bases << std::endl;
    std::cout << "  Total Generations: " << stats.total_generations << std::endl;
    std::cout << "  Active Generations: " << stats.active_generations << std::endl;
    std::cout << "  Rolled Off: " << stats.rolled_off_count << std::endl;
    std::cout << "  Total Space: " << stats.total_space_used / (1024*1024*1024) << " GB" << std::endl;
    std::cout << "  Active Space: " << stats.space_by_active_generations / (1024*1024*1024) << " GB" << std::endl;
    std::cout << "  Space Utilization: " << std::fixed << std::setprecision(1) 
              << stats.space_utilization() << "%" << std::endl;
    
    // =========================================================================
    // Simulated GDG Operations
    // =========================================================================
    print_separator("Simulated GDG Operations");
    
    std::cout << "Scenario: Daily backup process" << std::endl;
    std::cout << std::endl;
    
    std::cout << "1. Check GDG base PROD.BACKUP.DAILY" << std::endl;
    std::cout << "   Current: G0005V00, Limit: 7, Active: 5" << std::endl;
    
    std::cout << std::endl << "2. Create new generation (+1)" << std::endl;
    std::cout << "   -> Allocating PROD.BACKUP.DAILY.G0006V00" << std::endl;
    std::cout << "   -> Status: SUCCESS" << std::endl;
    
    std::cout << std::endl << "3. Write backup data to (+1)" << std::endl;
    std::cout << "   -> Writing 125 MB to generation" << std::endl;
    std::cout << "   -> Status: SUCCESS" << std::endl;
    
    std::cout << std::endl << "4. Close generation (+1)" << std::endl;
    std::cout << "   -> Updating catalog" << std::endl;
    std::cout << "   -> Current generation now G0006V00" << std::endl;
    std::cout << "   -> Active generations: 6" << std::endl;
    
    std::cout << std::endl << "5. Next day: Create generation (+1)" << std::endl;
    std::cout << "   -> Allocating PROD.BACKUP.DAILY.G0007V00" << std::endl;
    std::cout << "   -> Current generation now G0007V00" << std::endl;
    std::cout << "   -> Active generations: 7 (at limit)" << std::endl;
    
    std::cout << std::endl << "6. Next day: Create generation (+1)" << std::endl;
    std::cout << "   -> Allocating PROD.BACKUP.DAILY.G0008V00" << std::endl;
    std::cout << "   -> FIFO rolloff: G0001V00 expired" << std::endl;
    std::cout << "   -> Scratch: Deleting G0001V00 from disk" << std::endl;
    std::cout << "   -> Active generations: 7 (maintained at limit)" << std::endl;
    
    // =========================================================================
    // GDG JCL Examples
    // =========================================================================
    print_separator("GDG JCL Reference");
    
    std::cout << "Common GDG JCL patterns:" << std::endl;
    std::cout << std::endl;
    
    std::cout << "// Reference current generation:" << std::endl;
    std::cout << "//INPUT    DD DSN=PROD.BACKUP.DAILY(0),DISP=SHR" << std::endl;
    
    std::cout << std::endl << "// Reference previous generation:" << std::endl;
    std::cout << "//PREV     DD DSN=PROD.BACKUP.DAILY(-1),DISP=SHR" << std::endl;
    
    std::cout << std::endl << "// Create new generation:" << std::endl;
    std::cout << "//OUTPUT   DD DSN=PROD.BACKUP.DAILY(+1)," << std::endl;
    std::cout << "//            DISP=(NEW,CATLG,DELETE)," << std::endl;
    std::cout << "//            SPACE=(CYL,(100,50),RLSE)" << std::endl;
    
    std::cout << std::endl << "// Reference by absolute name:" << std::endl;
    std::cout << "//SPECIFIC DD DSN=PROD.BACKUP.DAILY.G0005V00,DISP=SHR" << std::endl;
    
    std::cout << std::endl << "=== GDG Example Complete ===" << std::endl;
    
    return 0;
}
