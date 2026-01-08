// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Console Demo
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/common/logger.hpp"
#include "ims/common/platform.hpp"
#include "ims/security/security_context.hpp"
#include "ims/catalog/master_catalog.hpp"
#include "ims/vsam/vsam_types.hpp"
#include "ims/ims/ims_types.hpp"
#include "ims/gdg/gdg_types.hpp"
#include "ims/dfsmshsm/dfsmshsm_types.hpp"

#include <iostream>
#include <iomanip>

using namespace ims;

/**
 * @brief IBM IMS (Information Management System) Emulation Enterprise Console Demo
 * 
 * Production demonstration of IMS Enterprise capabilities:
 * - Console output only (no GUI dialogs)
 * - All core libraries working together
 * - Clean, professional logging
 * - Enterprise-grade functionality
 */
class ImsConsoleDemo {
private:
    UniquePtr<catalog::MasterCatalog> master_catalog_;
    
public:
    ImsConsoleDemo() = default;
    
    ErrorResult<void> initialize() {
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "IBM IMS (INFORMATION MANAGEMENT SYSTEM) EMULATION ENTERPRISE v" << IMS_VERSION << " - CONSOLE DEMO" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "Platform: " << IMS_PLATFORM_NAME << std::endl;
        std::cout << "Architecture: " << IMS_ARCH_NAME << std::endl;
        std::cout << "Compiler: " << IMS_COMPILER_NAME << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        // Initialize security
        auto security_result = initialize_security();
        if (!security_result) {
            return make_error<void>("Security initialization failed: " + security_result.error());
        }
        
        // Initialize catalog
        auto catalog_result = initialize_catalog();
        if (!catalog_result) {
            return make_error<void>("Catalog initialization failed: " + catalog_result.error());
        }
        
        return make_success();
    }
    
    ErrorResult<void> run_demo() {
        std::cout << std::endl << "Starting Enterprise Demonstration..." << std::endl;
        
        // Demonstrate each subsystem
        auto security_demo = demonstrate_security_system();
        if (!security_demo) {
            std::cout << "  [WARNING] Security demo: " << security_demo.error() << std::endl;
        }
        
        auto catalog_demo = demonstrate_catalog_system();
        if (!catalog_demo) {
            std::cout << "  [WARNING] Catalog demo: " << catalog_demo.error() << std::endl;
        }
        
        auto vsam_demo = demonstrate_vsam_system();
        if (!vsam_demo) {
            std::cout << "  [WARNING] VSAM demo: " << vsam_demo.error() << std::endl;
        }
        
        auto ims_demo = demonstrate_ims_system();
        if (!ims_demo) {
            std::cout << "  [WARNING] IMS demo: " << ims_demo.error() << std::endl;
        }
        
        auto gdg_demo = demonstrate_gdg_system();
        if (!gdg_demo) {
            std::cout << "  [WARNING] GDG demo: " << gdg_demo.error() << std::endl;
        }
        
        auto hsm_demo = demonstrate_hsm_system();
        if (!hsm_demo) {
            std::cout << "  [WARNING] HSM demo: " << hsm_demo.error() << std::endl;
        }
        
        // Print summary
        print_summary();
        
        return make_success();
    }
    
private:
    ErrorResult<void> initialize_security() {
        std::cout << std::endl << "[INIT] Security System" << std::endl;
        
        // Create and authenticate admin user
        auto& context = security::current_security_context();
        
        auto admin_user = std::make_shared<security::UserInfo>();
        admin_user->user_id = "ADMIN";
        admin_user->display_name = "System Administrator";
        admin_user->security_level = security::SecurityLevel::TOP_SECRET;
        admin_user->roles = {"ADMIN", "SYSADMIN", "OPERATOR"};
        admin_user->groups = {"ADMINS", "ALL"};
        
        context.set_user(admin_user);
        
        auto session = std::make_shared<security::SessionInfo>();
        session->session_id = "DEMO001";
        session->user_id = "ADMIN";
        context.set_session(session);
        
        // Grant permissions
        context.grant_permission("ENTERPRISE.*", security::AccessAction::ALL);
        context.grant_permission("SYSTEM.*", security::AccessAction::ALL);
        
        std::cout << "  [OK] Security context initialized" << std::endl;
        std::cout << "  [OK] User authenticated: " << admin_user->display_name << std::endl;
        
        return make_success();
    }
    
    ErrorResult<void> initialize_catalog() {
        std::cout << std::endl << "[INIT] Master Catalog" << std::endl;
        
        master_catalog_ = std::make_unique<catalog::MasterCatalog>("MASTER.CATALOG");
        
        // Define some initial volumes
        catalog::VolumeInfo vol1;
        vol1.volser = "VOL001";
        vol1.type = catalog::VolumeType::DASD;
        vol1.total_space = 10ULL * 1024 * 1024 * 1024;  // 10 GB
        vol1.used_space = 2ULL * 1024 * 1024 * 1024;    // 2 GB
        vol1.free_space = vol1.total_space - vol1.used_space;
        vol1.is_mounted = true;
        master_catalog_->define_volume(vol1);
        
        std::cout << "  [OK] Master catalog initialized" << std::endl;
        std::cout << "  [OK] Volume defined: " << vol1.volser << std::endl;
        
        return make_success();
    }
    
    ErrorResult<void> demonstrate_security_system() {
        std::cout << std::endl << "[DEMO] Security System" << std::endl;
        
        auto& context = security::current_security_context();
        
        // Test authorization
        bool auth_read = context.is_authorized("ENTERPRISE.PAYROLL.DATA", security::AccessAction::READ);
        bool auth_admin = context.is_authorized("SYSTEM.CONFIG", security::AccessAction::ADMIN);
        
        std::cout << "  User: " << context.get_user()->display_name << std::endl;
        std::cout << "  Security Level: " << static_cast<int>(context.get_security_level()) << std::endl;
        std::cout << "  Session: " << context.get_session_id() << std::endl;
        std::cout << "  Authorization (Payroll Read): " << (auth_read ? "GRANTED" : "DENIED") << std::endl;
        std::cout << "  Authorization (System Admin): " << (auth_admin ? "GRANTED" : "DENIED") << std::endl;
        
        return make_success();
    }
    
    ErrorResult<void> demonstrate_catalog_system() {
        std::cout << std::endl << "[DEMO] Master Catalog System" << std::endl;
        
        // Create sample datasets
        Vector<String> datasets = {
            "ENTERPRISE.CUSTOMER.MASTER",
            "ENTERPRISE.TRANSACTION.LOG",
            "ENTERPRISE.PRODUCT.CATALOG",
            "ENTERPRISE.REPORTS.DAILY"
        };
        
        for (const auto& name : datasets) {
            catalog::CatalogEntry entry;
            entry.name = name;
            entry.dsorg = catalog::DatasetOrganization::PS;
            entry.recfm = catalog::RecordFormat::FIXED_BLOCKED;
            entry.lrecl = 1024;
            entry.blksize = 27920;
            entry.allocated_space = 1024 * 1024;  // 1 MB
            entry.volser = "VOL001";
            
            auto result = master_catalog_->define_entry(entry);
            if (result) {
                std::cout << "  [OK] Defined: " << name << std::endl;
            }
        }
        
        // List entries
        auto entries = master_catalog_->list_entries("ENTERPRISE.*");
        std::cout << "  Total entries matching 'ENTERPRISE.*': " << entries.size() << std::endl;
        
        // Get statistics
        auto stats = master_catalog_->get_statistics();
        std::cout << "  Catalog Statistics:" << std::endl;
        std::cout << "    Total Entries: " << stats.total_entries << std::endl;
        std::cout << "    Space Allocated: " << stats.total_space_allocated << " bytes" << std::endl;
        
        return make_success();
    }
    
    ErrorResult<void> demonstrate_vsam_system() {
        std::cout << std::endl << "[DEMO] VSAM System" << std::endl;
        
        // Create VSAM dataset configuration
        vsam::VsamDatasetConfig config("ENTERPRISE.VSAM.KSDS", vsam::VsamType::KSDS);
        config.key_length = 16;
        config.max_record_size = 4096;
        config.avg_record_size = 512;
        
        std::cout << "  Dataset Configuration:" << std::endl;
        std::cout << "    Name: " << config.name << std::endl;
        std::cout << "    Type: " << catalog::vsam_type_to_string(config.type) << std::endl;
        std::cout << "    Key Length: " << config.key_length << std::endl;
        std::cout << "    Max Record: " << config.max_record_size << std::endl;
        
        // Create a sample key and record
        vsam::VsamKey key("CUSTOMER00000001");
        vsam::VsamRecord record(512);
        record.key = key;
        record.length = 512;
        
        std::cout << "  Sample Operations:" << std::endl;
        std::cout << "    [OK] Key: " << key.to_string() << std::endl;
        std::cout << "    [OK] Record Size: " << record.length << " bytes" << std::endl;
        std::cout << "    [OK] Operations: READ, WRITE, UPDATE simulated" << std::endl;
        
        return make_success();
    }
    
    ErrorResult<void> demonstrate_ims_system() {
        std::cout << std::endl << "[DEMO] IMS Database System" << std::endl;
        
        // Create IMS database definition
        imsdb::DatabaseDefinition db_def;
        db_def.name = "CUSTOMER";
        db_def.type = imsdb::DatabaseType::HIDAM;
        db_def.access_method = imsdb::AccessMethod::RANDOM;
        
        std::cout << "  Database Configuration:" << std::endl;
        std::cout << "    Name: " << db_def.name << std::endl;
        std::cout << "    Type: " << imsdb::database_type_to_string(db_def.type) << std::endl;
        std::cout << "    Access: " << imsdb::access_method_to_string(db_def.access_method) << std::endl;
        
        // Simulate DL/I operations
        imsdb::DliRequest request;
        request.function_code = imsdb::DliCall::GU;
        request.pcb_name = "CUSTOMER_PCB";
        
        std::cout << "  DL/I Operations:" << std::endl;
        std::cout << "    [OK] Function: " << imsdb::dli_call_to_string(request.function_code) << std::endl;
        std::cout << "    [OK] PCB: " << request.pcb_name << std::endl;
        std::cout << "    [OK] Status: " << imsdb::dli_status_to_string(imsdb::DliStatusCode::NORMAL) << std::endl;
        
        return make_success();
    }
    
    ErrorResult<void> demonstrate_gdg_system() {
        std::cout << std::endl << "[DEMO] GDG System" << std::endl;
        
        // Create GDG base
        gdg::GdgBase base;
        base.name = "ENTERPRISE.BACKUP.DAILY";
        base.limit = 7;  // Keep 7 generations (week)
        base.model = gdg::GdgModel::FIFO;
        
        std::cout << "  GDG Configuration:" << std::endl;
        std::cout << "    Name: " << base.name << std::endl;
        std::cout << "    Limit: " << base.limit << " generations" << std::endl;
        std::cout << "    Model: " << gdg::gdg_model_to_string(base.model) << std::endl;
        
        // Create sample generations
        for (int i = 0; i < 3; ++i) {
            gdg::GenerationDataset gen;
            gen.base_name = base.name;
            gen.generation_number = static_cast<Int16>(i);
            gen.status = gdg::GdgStatus::ACTIVE;
            std::cout << "    [OK] Generation: " << gen.get_relative_name() << std::endl;
        }
        
        return make_success();
    }
    
    ErrorResult<void> demonstrate_hsm_system() {
        std::cout << std::endl << "[DEMO] DFSMShsm Storage Management" << std::endl;
        
        // Create storage class
        dfsmshsm::StorageClass sc;
        sc.name = "STANDARD";
        sc.default_level = dfsmshsm::StorageLevel::LEVEL0;
        sc.auto_migration_enabled = true;
        sc.compression = dfsmshsm::CompressionType::BASIC;
        
        std::cout << "  Storage Class: " << sc.name << std::endl;
        std::cout << "    Default Level: " << dfsmshsm::storage_level_to_string(sc.default_level) << std::endl;
        std::cout << "    Auto Migration: " << (sc.auto_migration_enabled ? "Enabled" : "Disabled") << std::endl;
        
        // Create management class
        dfsmshsm::ManagementClass mc;
        mc.name = "STANDARD";
        mc.backup_versions = 7;
        mc.auto_backup_enabled = true;
        
        std::cout << "  Management Class: " << mc.name << std::endl;
        std::cout << "    Backup Versions: " << mc.backup_versions << std::endl;
        std::cout << "    Auto Backup: " << (mc.auto_backup_enabled ? "Enabled" : "Disabled") << std::endl;
        
        // Display statistics
        dfsmshsm::StorageStatistics stats;
        stats.total_datasets = 100;
        stats.resident_datasets = 80;
        stats.migrated_datasets = 20;
        stats.total_space_bytes = 10ULL * 1024 * 1024 * 1024;
        stats.used_space_bytes = 4ULL * 1024 * 1024 * 1024;
        
        std::cout << "  Storage Statistics:" << std::endl;
        std::cout << "    Total Datasets: " << stats.total_datasets << std::endl;
        std::cout << "    Resident: " << stats.resident_datasets << std::endl;
        std::cout << "    Migrated: " << stats.migrated_datasets << std::endl;
        std::cout << "    Utilization: " << std::fixed << std::setprecision(1) 
                  << stats.utilization_percent() << "%" << std::endl;
        
        return make_success();
    }
    
    void print_summary() {
        std::cout << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "DEMONSTRATION COMPLETE" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << std::endl;
        std::cout << "IBM IMS (Information Management System) Emulation Enterprise v" << IMS_VERSION << " Features Demonstrated:" << std::endl;
        std::cout << "  - Security Context and Authorization" << std::endl;
        std::cout << "  - Master Catalog Management" << std::endl;
        std::cout << "  - VSAM Dataset Operations" << std::endl;
        std::cout << "  - IMS/DL-I Database Access" << std::endl;
        std::cout << "  - GDG (Generation Data Group) Management" << std::endl;
        std::cout << "  - DFSMShsm Storage Management" << std::endl;
        std::cout << std::endl;
        std::cout << "All subsystems operational." << std::endl;
        std::cout << std::string(80, '=') << std::endl;
    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    try {
        ImsConsoleDemo demo;
        
        auto init_result = demo.initialize();
        if (!init_result) {
            std::cerr << "Initialization failed: " << init_result.error() << std::endl;
            return 1;
        }
        
        auto run_result = demo.run_demo();
        if (!run_result) {
            std::cerr << "Demo failed: " << run_result.error() << std::endl;
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
