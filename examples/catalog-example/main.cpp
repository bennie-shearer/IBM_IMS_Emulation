// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Catalog Example
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/catalog/master_catalog.hpp"

#include <iostream>
#include <iomanip>

using namespace ims;
using namespace ims::catalog;

/**
 * @brief Demonstrates Master Catalog operations
 * 
 * This example shows:
 * - Creating and configuring a master catalog
 * - Defining volumes
 * - Creating dataset entries
 * - Listing and searching entries
 * - Working with VSAM datasets
 * - Catalog statistics
 */
int main() {
    std::cout << "=== IMS Catalog Example ===" << std::endl << std::endl;
    
    // -------------------------------------------------------------------------
    // Create Master Catalog
    // -------------------------------------------------------------------------
    std::cout << "1. Creating Master Catalog..." << std::endl;
    
    MasterCatalog catalog("PROD.MASTER.CATALOG", "./catalog_data");
    std::cout << "   Catalog: " << catalog.get_catalog_name() << std::endl;
    std::cout << "   Path: " << catalog.get_catalog_path().string() << std::endl;
    
    // -------------------------------------------------------------------------
    // Define Volumes
    // -------------------------------------------------------------------------
    std::cout << std::endl << "2. Defining Volumes..." << std::endl;
    
    VolumeInfo vol1;
    vol1.volser = "VOL001";
    vol1.type = VolumeType::DASD;
    vol1.status = VolumeStatus::ONLINE;
    vol1.total_space = 10ULL * 1024 * 1024 * 1024;
    vol1.used_space = 2ULL * 1024 * 1024 * 1024;
    vol1.free_space = vol1.total_space - vol1.used_space;
    vol1.is_mounted = true;
    
    VolumeInfo vol2;
    vol2.volser = "VOL002";
    vol2.type = VolumeType::DASD;
    vol2.status = VolumeStatus::ONLINE;
    vol2.total_space = 20ULL * 1024 * 1024 * 1024;
    vol2.used_space = 5ULL * 1024 * 1024 * 1024;
    vol2.free_space = vol2.total_space - vol2.used_space;
    vol2.is_mounted = true;
    
    VolumeInfo vol3;
    vol3.volser = "TAPE01";
    vol3.type = VolumeType::TAPE;
    vol3.status = VolumeStatus::ONLINE;
    vol3.total_space = 100ULL * 1024 * 1024 * 1024;
    vol3.used_space = 0;
    vol3.free_space = vol3.total_space;
    vol3.is_mounted = true;
    
    Vector<VolumeInfo> volumes = {vol1, vol2, vol3};
    
    for (auto& vol : volumes) {
        auto result = catalog.define_volume(vol);
        if (result) {
            std::cout << "   Defined volume: " << vol.volser 
                      << " (" << (vol.type == VolumeType::DASD ? "DASD" : "TAPE") 
                      << ", " << vol.total_space / (1024*1024*1024) << " GB)" << std::endl;
        }
    }
    
    // -------------------------------------------------------------------------
    // Create Dataset Entries
    // -------------------------------------------------------------------------
    std::cout << std::endl << "3. Creating Dataset Entries..." << std::endl;
    
    // Sequential dataset
    CatalogEntry seq_ds;
    seq_ds.name = "PROD.PAYROLL.MASTER";
    seq_ds.dsorg = DatasetOrganization::PS;
    seq_ds.recfm = RecordFormat::FIXED_BLOCKED;
    seq_ds.lrecl = 200;
    seq_ds.blksize = 27800;
    seq_ds.volser = "VOL001";
    seq_ds.allocated_space = 50 * 1024 * 1024;  // 50 MB
    seq_ds.owner = "PAYROLL";
    
    auto result = catalog.define_entry(seq_ds);
    if (result) {
        std::cout << "   Created: " << seq_ds.name << " (PS)" << std::endl;
    }
    
    // Partitioned dataset (PDS)
    CatalogEntry pds_ds;
    pds_ds.name = "PROD.SOURCE.COBOL";
    pds_ds.dsorg = DatasetOrganization::PO;
    pds_ds.recfm = RecordFormat::FIXED_BLOCKED;
    pds_ds.lrecl = 80;
    pds_ds.blksize = 27920;
    pds_ds.volser = "VOL001";
    pds_ds.allocated_space = 100 * 1024 * 1024;  // 100 MB
    pds_ds.owner = "DEVTEAM";
    
    result = catalog.define_entry(pds_ds);
    if (result) {
        std::cout << "   Created: " << pds_ds.name << " (PO)" << std::endl;
    }
    
    // VSAM KSDS dataset
    CatalogEntry vsam_ksds;
    vsam_ksds.name = "PROD.CUSTOMER.KSDS";
    vsam_ksds.dsorg = DatasetOrganization::VSAM;
    vsam_ksds.vsam_type = VsamType::KSDS;
    vsam_ksds.recfm = RecordFormat::VARIABLE;
    vsam_ksds.lrecl = 4096;
    vsam_ksds.keylen = 16;
    vsam_ksds.keyoff = 0;
    vsam_ksds.volser = "VOL002";
    vsam_ksds.allocated_space = 200 * 1024 * 1024;  // 200 MB
    vsam_ksds.owner = "CUSTMGMT";
    
    result = catalog.define_entry(vsam_ksds);
    if (result) {
        std::cout << "   Created: " << vsam_ksds.name << " (VSAM KSDS)" << std::endl;
    }
    
    // VSAM ESDS dataset
    CatalogEntry vsam_esds;
    vsam_esds.name = "PROD.JOURNAL.ESDS";
    vsam_esds.dsorg = DatasetOrganization::VSAM;
    vsam_esds.vsam_type = VsamType::ESDS;
    vsam_esds.recfm = RecordFormat::VARIABLE;
    vsam_esds.lrecl = 32760;
    vsam_esds.volser = "VOL002";
    vsam_esds.allocated_space = 500 * 1024 * 1024;  // 500 MB
    vsam_esds.owner = "JOURNAL";
    
    result = catalog.define_entry(vsam_esds);
    if (result) {
        std::cout << "   Created: " << vsam_esds.name << " (VSAM ESDS)" << std::endl;
    }
    
    // More datasets for demonstration
    Vector<String> additional_datasets = {
        "PROD.INVENTORY.DATA",
        "PROD.ORDERS.HISTORY",
        "PROD.REPORTS.DAILY",
        "TEST.PAYROLL.BACKUP",
        "TEST.CUSTOMER.COPY"
    };
    
    for (const auto& name : additional_datasets) {
        CatalogEntry entry;
        entry.name = name;
        entry.dsorg = DatasetOrganization::PS;
        entry.recfm = RecordFormat::FIXED_BLOCKED;
        entry.lrecl = 256;
        entry.blksize = 27904;
        entry.volser = (name.find("TEST") == 0) ? "VOL002" : "VOL001";
        entry.allocated_space = 10 * 1024 * 1024;
        
        catalog.define_entry(entry);
        std::cout << "   Created: " << name << std::endl;
    }
    
    // -------------------------------------------------------------------------
    // List and Search Entries
    // -------------------------------------------------------------------------
    std::cout << std::endl << "4. Listing and Searching..." << std::endl;
    
    // List all entries
    auto all_entries = catalog.list_entries("*");
    std::cout << "   Total entries: " << all_entries.size() << std::endl;
    
    // List PROD entries
    auto prod_entries = catalog.list_entries("PROD.*");
    std::cout << "   PROD.* entries: " << prod_entries.size() << std::endl;
    for (const auto& name : prod_entries) {
        std::cout << "      " << name << std::endl;
    }
    
    // List TEST entries
    auto test_entries = catalog.list_entries("TEST.*");
    std::cout << "   TEST.* entries: " << test_entries.size() << std::endl;
    
    // -------------------------------------------------------------------------
    // Get VSAM Entries
    // -------------------------------------------------------------------------
    std::cout << std::endl << "5. VSAM Datasets..." << std::endl;
    
    auto vsam_entries = catalog.get_vsam_entries();
    std::cout << "   Total VSAM entries: " << vsam_entries.size() << std::endl;
    
    for (const auto& entry : vsam_entries) {
        std::cout << "   " << entry.name 
                  << " - Type: " << vsam_type_to_string(entry.vsam_type)
                  << ", LRECL: " << entry.lrecl;
        if (entry.vsam_type == VsamType::KSDS) {
            std::cout << ", Key: " << entry.keylen << " bytes";
        }
        std::cout << std::endl;
    }
    
    // Get specific VSAM type
    auto ksds_entries = catalog.get_vsam_entries(VsamType::KSDS);
    std::cout << "   KSDS entries: " << ksds_entries.size() << std::endl;
    
    // -------------------------------------------------------------------------
    // Retrieve Specific Entry
    // -------------------------------------------------------------------------
    std::cout << std::endl << "6. Entry Details..." << std::endl;
    
    auto entry_result = catalog.get_entry("PROD.CUSTOMER.KSDS");
    if (entry_result) {
        const auto& entry = entry_result.value();
        std::cout << "   Dataset: " << entry.name << std::endl;
        std::cout << "   Organization: " << dsorg_to_string(entry.dsorg) << std::endl;
        std::cout << "   VSAM Type: " << vsam_type_to_string(entry.vsam_type) << std::endl;
        std::cout << "   Record Format: " << recfm_to_string(entry.recfm) << std::endl;
        std::cout << "   LRECL: " << entry.lrecl << std::endl;
        std::cout << "   Key Length: " << entry.keylen << std::endl;
        std::cout << "   Volume: " << entry.volser << std::endl;
        std::cout << "   Allocated: " << entry.allocated_space / (1024*1024) << " MB" << std::endl;
        std::cout << "   Owner: " << entry.owner << std::endl;
    }
    
    // -------------------------------------------------------------------------
    // Update Entry
    // -------------------------------------------------------------------------
    std::cout << std::endl << "7. Updating Entry..." << std::endl;
    
    if (entry_result) {
        auto entry = entry_result.value();
        entry.used_space = 150 * 1024 * 1024;  // 150 MB used
        
        auto update_result = catalog.update_entry(entry);
        if (update_result) {
            std::cout << "   Updated " << entry.name << " used space to 150 MB" << std::endl;
        }
    }
    
    // -------------------------------------------------------------------------
    // Catalog Statistics
    // -------------------------------------------------------------------------
    std::cout << std::endl << "8. Catalog Statistics..." << std::endl;
    
    auto stats = catalog.get_statistics();
    std::cout << "   Total Entries: " << stats.total_entries << std::endl;
    std::cout << "   Dataset Entries: " << stats.dataset_entries << std::endl;
    std::cout << "   VSAM Entries: " << stats.vsam_entries << std::endl;
    std::cout << "   Total Space Allocated: " << stats.total_space_allocated / (1024*1024) << " MB" << std::endl;
    std::cout << "   Total Space Used: " << stats.total_space_used / (1024*1024) << " MB" << std::endl;
    std::cout << "   Space Utilization: " << std::fixed << std::setprecision(1) 
              << stats.space_utilization() << "%" << std::endl;
    std::cout << "   Operations: " << stats.operations_count << std::endl;
    
    // -------------------------------------------------------------------------
    // Volume Information
    // -------------------------------------------------------------------------
    std::cout << std::endl << "9. Volume Information..." << std::endl;
    
    auto vol_list = catalog.list_volumes();
    for (const auto& volser : vol_list) {
        auto vol_result = catalog.get_volume(volser);
        if (vol_result) {
            const auto& vol = vol_result.value();
            std::cout << "   " << vol.volser << ": "
                      << vol.used_space / (1024*1024*1024) << " / "
                      << vol.total_space / (1024*1024*1024) << " GB used ("
                      << std::fixed << std::setprecision(1) 
                      << vol.utilization_percent() << "%)" << std::endl;
        }
    }
    
    // -------------------------------------------------------------------------
    // Delete Entry
    // -------------------------------------------------------------------------
    std::cout << std::endl << "10. Deleting Entry..." << std::endl;
    
    auto delete_result = catalog.delete_entry("TEST.CUSTOMER.COPY");
    if (delete_result) {
        std::cout << "   Deleted: TEST.CUSTOMER.COPY" << std::endl;
    }
    
    std::cout << "   Remaining entries: " << catalog.entry_count() << std::endl;
    
    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------
    std::cout << std::endl << "11. Dataset Name Validation..." << std::endl;
    
    Vector<String> test_names = {
        "VALID.DATASET.NAME",
        "A",
        "THIS.NAME.IS.WAY.TOO.LONG.FOR.A.DATASET.NAME.IN.MVS",
        "INVALID/NAME",
        "GOOD.NAME.HERE"
    };
    
    for (const auto& name : test_names) {
        bool valid = MasterCatalog::is_valid_dataset_name(name);
        std::cout << "   '" << name << "': " << (valid ? "VALID" : "INVALID") << std::endl;
    }
    
    std::cout << std::endl << "=== Catalog Example Complete ===" << std::endl;
    
    return 0;
}
