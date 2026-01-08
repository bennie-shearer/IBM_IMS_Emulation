// =============================================================================
// IBM IMS Emulation Enterprise - Configuration Example
// Version: 3.6.2
// NEW in v3.6.2: Demonstrates runtime configuration management
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/config.hpp"

#include <iostream>
#include <iomanip>

using namespace ims;

void print_separator(const char* title) {
    std::cout << std::endl << "--- " << title << " ---" << std::endl;
}

int main() {
    std::cout << "=== IMS Configuration Example (v3.6.2) ===" << std::endl;
    
    // =========================================================================
    // Create and populate configuration
    // =========================================================================
    print_separator("Creating Configuration");
    
    ConfigManager config;
    
    // Set values in default section
    config.set("app_name", "IBM IMS Emulation Enterprise");
    config.set("version", "3.6.2");
    config.set("debug", true);
    config.set("max_connections", 100);
    config.set("timeout_seconds", 30.5);
    
    std::cout << "Default section values set:" << std::endl;
    std::cout << "  app_name: " << config.get_string("app_name") << std::endl;
    std::cout << "  version: " << config.get_string("version") << std::endl;
    std::cout << "  debug: " << (config.get_bool("debug") ? "true" : "false") << std::endl;
    std::cout << "  max_connections: " << config.get_int("max_connections") << std::endl;
    std::cout << "  timeout_seconds: " << std::fixed << std::setprecision(1) 
              << config.get_float("timeout_seconds") << std::endl;
    
    // =========================================================================
    // Create sections
    // =========================================================================
    print_separator("Creating Sections");
    
    // Database section
    auto& db_section = config.section("database");
    db_section.set("host", "localhost");
    db_section.set("port", 5432);
    db_section.set("name", "imsdb");
    db_section.set("pool_size", 10);
    db_section.set("ssl_enabled", true);
    
    std::cout << "Database section:" << std::endl;
    std::cout << "  host: " << db_section.get_string("host") << std::endl;
    std::cout << "  port: " << db_section.get_int("port") << std::endl;
    std::cout << "  name: " << db_section.get_string("name") << std::endl;
    std::cout << "  pool_size: " << db_section.get_int("pool_size") << std::endl;
    std::cout << "  ssl_enabled: " << (db_section.get_bool("ssl_enabled") ? "true" : "false") << std::endl;
    
    // Logging section
    auto& log_section = config.section("logging");
    log_section.set("level", "INFO");
    log_section.set("file", "/var/log/ims/app.log");
    log_section.set("max_size_mb", 100);
    log_section.set("rotate_count", 5);
    log_section.set("console", true);
    
    std::cout << std::endl << "Logging section:" << std::endl;
    std::cout << "  level: " << log_section.get_string("level") << std::endl;
    std::cout << "  file: " << log_section.get_string("file") << std::endl;
    std::cout << "  max_size_mb: " << log_section.get_int("max_size_mb") << std::endl;
    std::cout << "  rotate_count: " << log_section.get_int("rotate_count") << std::endl;
    std::cout << "  console: " << (log_section.get_bool("console") ? "true" : "false") << std::endl;
    
    // Security section
    auto& sec_section = config.section("security");
    sec_section.set("auth_type", "LDAP");
    sec_section.set("session_timeout", 3600);
    sec_section.set("max_login_attempts", 3);
    sec_section.set("lockout_duration", 300);
    sec_section.set("audit_enabled", true);
    
    std::cout << std::endl << "Security section:" << std::endl;
    std::cout << "  auth_type: " << sec_section.get_string("auth_type") << std::endl;
    std::cout << "  session_timeout: " << sec_section.get_int("session_timeout") << std::endl;
    std::cout << "  max_login_attempts: " << sec_section.get_int("max_login_attempts") << std::endl;
    std::cout << "  lockout_duration: " << sec_section.get_int("lockout_duration") << std::endl;
    std::cout << "  audit_enabled: " << (sec_section.get_bool("audit_enabled") ? "true" : "false") << std::endl;
    
    // =========================================================================
    // Save configuration to file
    // =========================================================================
    print_separator("Saving Configuration");
    
    auto save_result = config.save("ims_config.ini");
    if (save_result.has_value()) {
        std::cout << "Configuration saved to: ims_config.ini" << std::endl;
    } else {
        std::cout << "Failed to save: " << save_result.error() << std::endl;
    }
    
    // =========================================================================
    // Load configuration from file
    // =========================================================================
    print_separator("Loading Configuration");
    
    ConfigManager loaded_config;
    auto load_result = loaded_config.load("ims_config.ini");
    
    if (load_result.has_value()) {
        std::cout << "Configuration loaded successfully!" << std::endl;
        
        std::cout << std::endl << "Sections found:" << std::endl;
        for (const auto& name : loaded_config.section_names()) {
            std::cout << "  [" << name << "]" << std::endl;
        }
        
        std::cout << std::endl << "Loaded values:" << std::endl;
        std::cout << "  app_name: " << loaded_config.get_string("app_name") << std::endl;
        
        if (auto* db = loaded_config.section_ptr("database")) {
            std::cout << "  database.host: " << db->get_string("host") << std::endl;
            std::cout << "  database.port: " << db->get_int("port") << std::endl;
        }
        
        if (auto* sec = loaded_config.section_ptr("security")) {
            std::cout << "  security.auth_type: " << sec->get_string("auth_type") << std::endl;
        }
    } else {
        std::cout << "Failed to load: " << load_result.error() << std::endl;
    }
    
    // =========================================================================
    // Qualified key access
    // =========================================================================
    print_separator("Qualified Key Access");
    
    std::cout << "Using dotted notation:" << std::endl;
    std::cout << "  database.host: " << loaded_config.get_qualified("database.host").as_string() << std::endl;
    std::cout << "  database.port: " << loaded_config.get_qualified("database.port").as_int() << std::endl;
    std::cout << "  security.auth_type: " << loaded_config.get_qualified("security.auth_type").as_string() << std::endl;
    std::cout << "  logging.level: " << loaded_config.get_qualified("logging.level").as_string() << std::endl;
    
    // =========================================================================
    // Default values
    // =========================================================================
    print_separator("Default Values");
    
    std::cout << "Non-existent keys with defaults:" << std::endl;
    std::cout << "  missing_string: '" << config.get_string("missing", "DEFAULT") << "'" << std::endl;
    std::cout << "  missing_int: " << config.get_int("missing", 999) << std::endl;
    std::cout << "  missing_bool: " << (config.get_bool("missing", true) ? "true" : "false") << std::endl;
    
    // =========================================================================
    // Global configuration
    // =========================================================================
    print_separator("Global Configuration");
    
    auto& global = global_config();
    global.set("global_setting", "This is global");
    global.section("paths").set("data_dir", "/var/ims/data");
    global.section("paths").set("log_dir", "/var/ims/logs");
    
    std::cout << "Global config values:" << std::endl;
    std::cout << "  global_setting: " << global.get_string("global_setting") << std::endl;
    std::cout << "  paths.data_dir: " << global.section("paths").get_string("data_dir") << std::endl;
    std::cout << "  paths.log_dir: " << global.section("paths").get_string("log_dir") << std::endl;
    
    // =========================================================================
    // Type conversion
    // =========================================================================
    print_separator("Type Conversion");
    
    ConfigValue str_val("42");
    ConfigValue int_val(42);
    ConfigValue float_val(3.14159);
    ConfigValue bool_val(true);
    
    std::cout << "String '42' as int: " << str_val.as_int() << std::endl;
    std::cout << "Int 42 as string: " << int_val.to_string() << std::endl;
    std::cout << "Float 3.14159 as int: " << float_val.as_int() << std::endl;
    std::cout << "Bool true as string: " << bool_val.to_string() << std::endl;
    
    // Boolean conversions
    ConfigValue yes("yes");
    ConfigValue no("no");
    ConfigValue on("on");
    ConfigValue off("off");
    
    std::cout << std::endl << "Boolean string conversions:" << std::endl;
    std::cout << "  'yes' -> " << (yes.as_bool() ? "true" : "false") << std::endl;
    std::cout << "  'no' -> " << (no.as_bool() ? "true" : "false") << std::endl;
    std::cout << "  'on' -> " << (on.as_bool() ? "true" : "false") << std::endl;
    std::cout << "  'off' -> " << (off.as_bool() ? "true" : "false") << std::endl;
    
    // Cleanup
    std::filesystem::remove("ims_config.ini");
    
    std::cout << std::endl << "=== Configuration Example Complete ===" << std::endl;
    
    return 0;
}
