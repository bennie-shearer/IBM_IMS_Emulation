#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Security Context
// Version: 3.6.3
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"

namespace ims::security {

// =============================================================================
// Security Enums
// =============================================================================

enum class SecurityLevel : UInt8 {
    NONE = 0,
    PUBLIC = 1,
    INTERNAL = 2,
    CONFIDENTIAL = 3,
    SECRET = 4,
    TOP_SECRET = 5
};

enum class AuthenticationType : UInt8 {
    NONE = 0,
    PASSWORD = 1,
    CERTIFICATE = 2,
    TOKEN = 3,
    MFA = 4,
    KERBEROS = 5
};

enum class HashAlgorithm : UInt8 {
    SHA256 = 1,
    SHA384 = 2,
    SHA512 = 3,
    BLAKE3 = 4
};

enum class AccessAction : UInt8 {
    READ = 1,
    WRITE = 2,
    EXECUTE = 4,
    DELETE = 8,
    CREATE = 16,
    MODIFY = 32,
    ADMIN = 64,
    ALL = 255
};

// Bitwise operations for AccessAction
constexpr AccessAction operator|(AccessAction lhs, AccessAction rhs) {
    return static_cast<AccessAction>(static_cast<UInt8>(lhs) | static_cast<UInt8>(rhs));
}

constexpr AccessAction operator&(AccessAction lhs, AccessAction rhs) {
    return static_cast<AccessAction>(static_cast<UInt8>(lhs) & static_cast<UInt8>(rhs));
}

constexpr bool has_action(AccessAction permissions, AccessAction action) {
    return (static_cast<UInt8>(permissions) & static_cast<UInt8>(action)) != 0;
}

// =============================================================================
// User Information
// =============================================================================

struct UserInfo {
    String user_id;
    String display_name;
    String email;
    Vector<String> groups;
    Vector<String> roles;
    SecurityLevel security_level{SecurityLevel::INTERNAL};
    SystemTimePoint created_at;
    SystemTimePoint last_login;
    bool is_active{true};
    bool is_locked{false};
    
    UserInfo() : created_at(SystemClock::now()), last_login(SystemClock::now()) {}
};

// =============================================================================
// Session Information
// =============================================================================

struct SessionInfo {
    String session_id;
    String user_id;
    SystemTimePoint created_at;
    SystemTimePoint expires_at;
    SystemTimePoint last_activity;
    String client_ip;
    String client_agent;
    bool is_valid{true};
    
    SessionInfo() 
        : created_at(SystemClock::now())
        , expires_at(SystemClock::now() + Hours(8))
        , last_activity(SystemClock::now()) {}
    
    bool is_expired() const {
        return SystemClock::now() > expires_at;
    }
};

// =============================================================================
// Security Context
// =============================================================================

class SecurityContext {
private:
    SharedPtr<UserInfo> user_;
    SharedPtr<SessionInfo> session_;
    SecurityLevel effective_level_{SecurityLevel::NONE};
    HashMap<String, AccessAction> permissions_;
    mutable Mutex mutex_;
    
public:
    SecurityContext() = default;
    
    SecurityContext(SharedPtr<UserInfo> user, SharedPtr<SessionInfo> session)
        : user_(std::move(user))
        , session_(std::move(session)) {
        if (user_) {
            effective_level_ = user_->security_level;
        }
    }
    
    // User access
    SharedPtr<UserInfo> get_user() const {
        LockGuard<Mutex> lock(mutex_);
        return user_;
    }
    
    void set_user(SharedPtr<UserInfo> user) {
        LockGuard<Mutex> lock(mutex_);
        user_ = std::move(user);
        if (user_) {
            effective_level_ = user_->security_level;
        }
    }
    
    // Session access
    SharedPtr<SessionInfo> get_session() const {
        LockGuard<Mutex> lock(mutex_);
        return session_;
    }
    
    void set_session(SharedPtr<SessionInfo> session) {
        LockGuard<Mutex> lock(mutex_);
        session_ = std::move(session);
    }
    
    String get_session_id() const {
        LockGuard<Mutex> lock(mutex_);
        return session_ ? session_->session_id : "";
    }
    
    // Security level
    SecurityLevel get_security_level() const {
        LockGuard<Mutex> lock(mutex_);
        return effective_level_;
    }
    
    // Authorization
    void grant_permission(const String& resource, AccessAction actions) {
        LockGuard<Mutex> lock(mutex_);
        auto it = permissions_.find(resource);
        if (it != permissions_.end()) {
            it->second = it->second | actions;
        } else {
            permissions_[resource] = actions;
        }
    }
    
    void revoke_permission(const String& resource) {
        LockGuard<Mutex> lock(mutex_);
        permissions_.erase(resource);
    }
    
    bool is_authorized(const String& resource, AccessAction action) const {
        LockGuard<Mutex> lock(mutex_);
        
        // Check user is valid
        if (!user_ || !user_->is_active || user_->is_locked) {
            return false;
        }
        
        // Check session is valid
        if (session_ && (session_->is_expired() || !session_->is_valid)) {
            return false;
        }
        
        // Check permissions
        auto it = permissions_.find(resource);
        if (it != permissions_.end()) {
            return has_action(it->second, action);
        }
        
        // Default: check for admin role
        for (const auto& role : user_->roles) {
            if (role == "ADMIN" || role == "SYSADMIN") {
                return true;
            }
        }
        
        return false;
    }
    
    bool is_authenticated() const {
        LockGuard<Mutex> lock(mutex_);
        return user_ != nullptr && user_->is_active && !user_->is_locked;
    }
    
    bool is_session_valid() const {
        LockGuard<Mutex> lock(mutex_);
        return session_ != nullptr && !session_->is_expired() && session_->is_valid;
    }
};

// =============================================================================
// Global Security Context
// =============================================================================

inline SecurityContext& current_security_context() {
    static thread_local SecurityContext context;
    return context;
}

// =============================================================================
// Security Manager Interface
// =============================================================================

class ISecurityManager {
public:
    virtual ~ISecurityManager() = default;
    
    virtual ErrorResult<SharedPtr<UserInfo>> authenticate(
        const String& user_id, const String& credentials) = 0;
    
    virtual ErrorResult<SharedPtr<SessionInfo>> create_session(
        const String& user_id) = 0;
    
    virtual ErrorResult<void> validate_session(const String& session_id) = 0;
    
    virtual ErrorResult<void> invalidate_session(const String& session_id) = 0;
    
    virtual bool is_authorized(const String& user_id, 
                               const String& resource, 
                               AccessAction action) = 0;
};

// =============================================================================
// Default Security Manager
// =============================================================================

class DefaultSecurityManager : public ISecurityManager {
private:
    HashMap<String, SharedPtr<UserInfo>> users_;
    HashMap<String, SharedPtr<SessionInfo>> sessions_;
    mutable SharedMutex mutex_;
    UInt64 session_counter_{0};
    
public:
    DefaultSecurityManager() {
        // Create default admin user
        auto admin = std::make_shared<UserInfo>();
        admin->user_id = "ADMIN";
        admin->display_name = "System Administrator";
        admin->security_level = SecurityLevel::TOP_SECRET;
        admin->roles = {"ADMIN", "SYSADMIN"};
        admin->groups = {"ADMINS", "OPERATORS"};
        users_["ADMIN"] = admin;
        
        // Create default user
        auto user = std::make_shared<UserInfo>();
        user->user_id = "USER";
        user->display_name = "Default User";
        user->security_level = SecurityLevel::INTERNAL;
        user->roles = {"USER"};
        user->groups = {"USERS"};
        users_["USER"] = user;
    }
    
    ErrorResult<SharedPtr<UserInfo>> authenticate(
        const String& user_id, const String& credentials) override {
        SharedLock<SharedMutex> lock(mutex_);
        
        auto it = users_.find(user_id);
        if (it == users_.end()) {
            return "User not found: " + user_id;
        }
        
        auto& user = it->second;
        if (!user->is_active) {
            return "User account is disabled";
        }
        if (user->is_locked) {
            return "User account is locked";
        }
        
        // Simple credential check (in production, use proper hashing)
        // For demo purposes, accept any non-empty credential
        if (credentials.empty()) {
            return "Invalid credentials";
        }
        
        user->last_login = SystemClock::now();
        return user;
    }
    
    ErrorResult<SharedPtr<SessionInfo>> create_session(const String& user_id) override {
        UniqueLock<SharedMutex> lock(mutex_);
        
        auto session = std::make_shared<SessionInfo>();
        session->session_id = std::format("SES{:012}", ++session_counter_);
        session->user_id = user_id;
        session->created_at = SystemClock::now();
        session->expires_at = SystemClock::now() + Hours(8);
        session->last_activity = SystemClock::now();
        
        sessions_[session->session_id] = session;
        return session;
    }
    
    ErrorResult<void> validate_session(const String& session_id) override {
        SharedLock<SharedMutex> lock(mutex_);
        
        auto it = sessions_.find(session_id);
        if (it == sessions_.end()) {
            return "Session not found";
        }
        
        if (it->second->is_expired()) {
            return "Session expired";
        }
        
        if (!it->second->is_valid) {
            return "Session invalidated";
        }
        
        return make_success();
    }
    
    ErrorResult<void> invalidate_session(const String& session_id) override {
        UniqueLock<SharedMutex> lock(mutex_);
        
        auto it = sessions_.find(session_id);
        if (it != sessions_.end()) {
            it->second->is_valid = false;
        }
        
        return make_success();
    }
    
    bool is_authorized(const String& user_id, 
                       [[maybe_unused]] const String& resource, 
                       AccessAction action) override {
        SharedLock<SharedMutex> lock(mutex_);
        
        auto it = users_.find(user_id);
        if (it == users_.end()) {
            return false;
        }
        
        // Check for admin role
        for (const auto& role : it->second->roles) {
            if (role == "ADMIN" || role == "SYSADMIN") {
                return true;
            }
        }
        
        // Default authorization based on action
        return action == AccessAction::READ;
    }
    
    void add_user(SharedPtr<UserInfo> user) {
        UniqueLock<SharedMutex> lock(mutex_);
        users_[user->user_id] = std::move(user);
    }
};

// Global security manager
inline SharedPtr<ISecurityManager>& global_security_manager() {
    static SharedPtr<ISecurityManager> manager = std::make_shared<DefaultSecurityManager>();
    return manager;
}

} // namespace ims::security
