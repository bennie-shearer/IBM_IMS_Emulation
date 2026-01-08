// =============================================================================
// IBM IMS Emulation Enterprise - Security Example
// Version: 3.6.2
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/security/security_context.hpp"

#include <iostream>
#include <iomanip>

using namespace ims;
using namespace ims::security;

/**
 * @brief Demonstrates security context and authorization
 * 
 * This example shows:
 * - User and session management
 * - Security levels
 * - Permission grants and checks
 * - RACF-compatible authorization model
 */

void print_separator(const char* title) {
    std::cout << std::endl << "--- " << title << " ---" << std::endl;
}

int main() {
    std::cout << "=== IMS Security Example ===" << std::endl;
    
    // =========================================================================
    // Security Levels
    // =========================================================================
    print_separator("Security Levels");
    
    std::cout << "Available Security Levels:" << std::endl;
    std::cout << "  0 - NONE        (No security)" << std::endl;
    std::cout << "  1 - PUBLIC      (Public access)" << std::endl;
    std::cout << "  2 - INTERNAL    (Internal users)" << std::endl;
    std::cout << "  3 - CONFIDENTIAL (Confidential data)" << std::endl;
    std::cout << "  4 - SECRET      (Secret data)" << std::endl;
    std::cout << "  5 - TOP_SECRET  (Top secret data)" << std::endl;
    
    // =========================================================================
    // Access Actions
    // =========================================================================
    print_separator("Access Actions");
    
    std::cout << "Available Access Actions:" << std::endl;
    std::cout << "  READ    (1)   - Read access" << std::endl;
    std::cout << "  WRITE   (2)   - Write access" << std::endl;
    std::cout << "  EXECUTE (4)   - Execute access" << std::endl;
    std::cout << "  DELETE  (8)   - Delete access" << std::endl;
    std::cout << "  CREATE  (16)  - Create access" << std::endl;
    std::cout << "  MODIFY  (32)  - Modify access" << std::endl;
    std::cout << "  ADMIN   (64)  - Administrative access" << std::endl;
    std::cout << "  ALL     (255) - All access" << std::endl;
    
    std::cout << std::endl << "Combining actions with bitwise OR:" << std::endl;
    AccessAction rw = AccessAction::READ | AccessAction::WRITE;
    std::cout << "  READ | WRITE = " << static_cast<int>(rw) << std::endl;
    
    AccessAction crud = AccessAction::CREATE | AccessAction::READ | 
                        AccessAction::MODIFY | AccessAction::DELETE;
    std::cout << "  CREATE | READ | MODIFY | DELETE = " << static_cast<int>(crud) << std::endl;
    
    // =========================================================================
    // Create Users
    // =========================================================================
    print_separator("User Management");
    
    // Admin user
    auto admin_user = std::make_shared<UserInfo>();
    admin_user->user_id = "ADMIN001";
    admin_user->display_name = "System Administrator";
    admin_user->email = "admin@company.com";
    admin_user->security_level = SecurityLevel::TOP_SECRET;
    admin_user->roles = {"ADMIN", "SYSADMIN", "OPERATOR"};
    admin_user->groups = {"ADMINS", "IT_STAFF", "ALL_USERS"};
    admin_user->is_active = true;
    admin_user->is_locked = false;
    
    std::cout << "Admin User:" << std::endl;
    std::cout << "  User ID: " << admin_user->user_id << std::endl;
    std::cout << "  Name: " << admin_user->display_name << std::endl;
    std::cout << "  Security Level: " << static_cast<int>(admin_user->security_level) 
              << " (TOP_SECRET)" << std::endl;
    std::cout << "  Roles: ";
    for (size_t i = 0; i < admin_user->roles.size(); ++i) {
        std::cout << admin_user->roles[i];
        if (i < admin_user->roles.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Regular user
    auto regular_user = std::make_shared<UserInfo>();
    regular_user->user_id = "USER001";
    regular_user->display_name = "John Developer";
    regular_user->email = "john@company.com";
    regular_user->security_level = SecurityLevel::INTERNAL;
    regular_user->roles = {"USER", "DEVELOPER"};
    regular_user->groups = {"DEVELOPERS", "ALL_USERS"};
    regular_user->is_active = true;
    regular_user->is_locked = false;
    
    std::cout << std::endl << "Regular User:" << std::endl;
    std::cout << "  User ID: " << regular_user->user_id << std::endl;
    std::cout << "  Name: " << regular_user->display_name << std::endl;
    std::cout << "  Security Level: " << static_cast<int>(regular_user->security_level) 
              << " (INTERNAL)" << std::endl;
    std::cout << "  Roles: ";
    for (size_t i = 0; i < regular_user->roles.size(); ++i) {
        std::cout << regular_user->roles[i];
        if (i < regular_user->roles.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // =========================================================================
    // Session Management
    // =========================================================================
    print_separator("Session Management");
    
    auto session = std::make_shared<SessionInfo>();
    session->session_id = "SES000000001";
    session->user_id = "USER001";
    session->client_ip = "192.168.1.100";
    session->client_agent = "IMS-Client/3.6.2";
    session->is_valid = true;
    
    std::cout << "Session Information:" << std::endl;
    std::cout << "  Session ID: " << session->session_id << std::endl;
    std::cout << "  User ID: " << session->user_id << std::endl;
    std::cout << "  Client IP: " << session->client_ip << std::endl;
    std::cout << "  Client Agent: " << session->client_agent << std::endl;
    std::cout << "  Is Valid: " << (session->is_valid ? "Yes" : "No") << std::endl;
    std::cout << "  Is Expired: " << (session->is_expired() ? "Yes" : "No") << std::endl;
    
    // =========================================================================
    // Security Context
    // =========================================================================
    print_separator("Security Context");
    
    // Get thread-local security context
    auto& context = current_security_context();
    
    // Set up context with regular user
    context.set_user(regular_user);
    context.set_session(session);
    
    std::cout << "Context Setup (Regular User):" << std::endl;
    std::cout << "  Is Authenticated: " << (context.is_authenticated() ? "Yes" : "No") << std::endl;
    std::cout << "  Is Session Valid: " << (context.is_session_valid() ? "Yes" : "No") << std::endl;
    std::cout << "  Security Level: " << static_cast<int>(context.get_security_level()) << std::endl;
    
    // =========================================================================
    // Permission Grants
    // =========================================================================
    print_separator("Permission Grants");
    
    // Grant permissions to resources
    context.grant_permission("PROD.DATA.*", AccessAction::READ);
    context.grant_permission("DEV.DATA.*", AccessAction::READ | AccessAction::WRITE);
    context.grant_permission("DEV.SOURCE.*", AccessAction::ALL);
    context.grant_permission("USER001.PERSONAL.*", AccessAction::ALL);
    
    std::cout << "Granted Permissions:" << std::endl;
    std::cout << "  PROD.DATA.*         -> READ" << std::endl;
    std::cout << "  DEV.DATA.*          -> READ, WRITE" << std::endl;
    std::cout << "  DEV.SOURCE.*        -> ALL" << std::endl;
    std::cout << "  USER001.PERSONAL.*  -> ALL" << std::endl;
    
    // =========================================================================
    // Authorization Checks
    // =========================================================================
    print_separator("Authorization Checks (Regular User)");
    
    struct AuthTest {
        String resource;
        AccessAction action;
        const char* action_name;
    };
    
    Vector<AuthTest> tests = {
        {"PROD.DATA.CUSTOMER", AccessAction::READ, "READ"},
        {"PROD.DATA.CUSTOMER", AccessAction::WRITE, "WRITE"},
        {"DEV.DATA.TEST", AccessAction::READ, "READ"},
        {"DEV.DATA.TEST", AccessAction::WRITE, "WRITE"},
        {"DEV.SOURCE.COBOL", AccessAction::MODIFY, "MODIFY"},
        {"USER001.PERSONAL.NOTES", AccessAction::ALL, "ALL"},
        {"SYSTEM.CONFIG", AccessAction::ADMIN, "ADMIN"},
        {"PROD.PAYROLL.DATA", AccessAction::READ, "READ"}
    };
    
    std::cout << std::setw(30) << std::left << "Resource" 
              << std::setw(10) << "Action"
              << "Result" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    for (const auto& test : tests) {
        bool authorized = context.is_authorized(test.resource, test.action);
        std::cout << std::setw(30) << std::left << test.resource 
                  << std::setw(10) << test.action_name
                  << (authorized ? "GRANTED" : "DENIED") << std::endl;
    }
    
    // =========================================================================
    // Admin Context
    // =========================================================================
    print_separator("Authorization Checks (Admin User)");
    
    // Switch to admin user
    context.set_user(admin_user);
    
    std::cout << "Switched to admin user: " << admin_user->display_name << std::endl;
    std::cout << std::endl;
    
    // Admin should have access to everything
    std::cout << std::setw(30) << std::left << "Resource" 
              << std::setw(10) << "Action"
              << "Result" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    for (const auto& test : tests) {
        bool authorized = context.is_authorized(test.resource, test.action);
        std::cout << std::setw(30) << std::left << test.resource 
                  << std::setw(10) << test.action_name
                  << (authorized ? "GRANTED" : "DENIED") << std::endl;
    }
    
    // =========================================================================
    // Security Manager
    // =========================================================================
    print_separator("Security Manager");
    
    auto& security_manager = global_security_manager();
    
    // Authenticate user
    std::cout << "Authenticating USER..." << std::endl;
    auto auth_result = security_manager->authenticate("USER", "password123");
    if (auth_result) {
        std::cout << "  Authentication successful!" << std::endl;
        std::cout << "  User: " << auth_result.value()->display_name << std::endl;
    } else {
        std::cout << "  Authentication failed: " << auth_result.error() << std::endl;
    }
    
    // Create session
    std::cout << std::endl << "Creating session for USER..." << std::endl;
    auto session_result = security_manager->create_session("USER");
    if (session_result) {
        std::cout << "  Session created: " << session_result.value()->session_id << std::endl;
        
        // Validate session
        auto validate_result = security_manager->validate_session(
            session_result.value()->session_id);
        std::cout << "  Session valid: " << (validate_result ? "Yes" : "No") << std::endl;
        
        // Invalidate session
        std::cout << std::endl << "Invalidating session..." << std::endl;
        security_manager->invalidate_session(session_result.value()->session_id);
        
        // Validate again
        validate_result = security_manager->validate_session(
            session_result.value()->session_id);
        std::cout << "  Session valid after invalidation: " 
                  << (validate_result ? "Yes" : "No") << std::endl;
    }
    
    // =========================================================================
    // Locked User Scenario
    // =========================================================================
    print_separator("Locked User Scenario");
    
    auto locked_user = std::make_shared<UserInfo>();
    locked_user->user_id = "LOCKED01";
    locked_user->display_name = "Locked User";
    locked_user->is_active = true;
    locked_user->is_locked = true;  // Account is locked
    
    context.set_user(locked_user);
    
    std::cout << "User: " << locked_user->display_name << std::endl;
    std::cout << "  Is Active: " << (locked_user->is_active ? "Yes" : "No") << std::endl;
    std::cout << "  Is Locked: " << (locked_user->is_locked ? "Yes" : "No") << std::endl;
    std::cout << "  Is Authenticated: " << (context.is_authenticated() ? "Yes" : "No") << std::endl;
    
    bool auth_check = context.is_authorized("ANY.RESOURCE", AccessAction::READ);
    std::cout << "  Authorization for ANY.RESOURCE: " 
              << (auth_check ? "GRANTED" : "DENIED") << std::endl;
    
    // =========================================================================
    // Permission Revocation
    // =========================================================================
    print_separator("Permission Revocation");
    
    context.set_user(regular_user);
    
    std::cout << "Before revocation:" << std::endl;
    std::cout << "  DEV.DATA.* READ: " 
              << (context.is_authorized("DEV.DATA.TEST", AccessAction::READ) ? "GRANTED" : "DENIED") 
              << std::endl;
    
    context.revoke_permission("DEV.DATA.*");
    
    std::cout << std::endl << "After revoking DEV.DATA.* permission:" << std::endl;
    std::cout << "  DEV.DATA.* READ: " 
              << (context.is_authorized("DEV.DATA.TEST", AccessAction::READ) ? "GRANTED" : "DENIED") 
              << std::endl;
    
    std::cout << std::endl << "=== Security Example Complete ===" << std::endl;
    
    return 0;
}
