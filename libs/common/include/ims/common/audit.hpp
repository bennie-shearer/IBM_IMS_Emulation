#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Audit Logging
// Version: 3.6.3
// NEW in v3.6.3: Security audit trail support
// =============================================================================

#include "types.hpp"
#include "logger.hpp"
#include <iomanip>
#include <sstream>
#include <fstream>

namespace ims {

// =============================================================================
// Audit Event Types
// =============================================================================

enum class AuditEventType {
    // Authentication events
    LOGIN_SUCCESS,
    LOGIN_FAILURE,
    LOGOUT,
    SESSION_CREATED,
    SESSION_EXPIRED,
    SESSION_TERMINATED,
    
    // Authorization events
    ACCESS_GRANTED,
    ACCESS_DENIED,
    PRIVILEGE_ESCALATION,
    
    // Data access events
    DATA_READ,
    DATA_WRITE,
    DATA_DELETE,
    DATA_EXPORT,
    
    // Configuration events
    CONFIG_CHANGED,
    PERMISSION_CHANGED,
    USER_CREATED,
    USER_MODIFIED,
    USER_DELETED,
    
    // System events
    SYSTEM_START,
    SYSTEM_STOP,
    ERROR_CRITICAL,
    SECURITY_ALERT
};

inline String audit_event_type_to_string(AuditEventType type) {
    switch (type) {
        case AuditEventType::LOGIN_SUCCESS: return "LOGIN_SUCCESS";
        case AuditEventType::LOGIN_FAILURE: return "LOGIN_FAILURE";
        case AuditEventType::LOGOUT: return "LOGOUT";
        case AuditEventType::SESSION_CREATED: return "SESSION_CREATED";
        case AuditEventType::SESSION_EXPIRED: return "SESSION_EXPIRED";
        case AuditEventType::SESSION_TERMINATED: return "SESSION_TERMINATED";
        case AuditEventType::ACCESS_GRANTED: return "ACCESS_GRANTED";
        case AuditEventType::ACCESS_DENIED: return "ACCESS_DENIED";
        case AuditEventType::PRIVILEGE_ESCALATION: return "PRIVILEGE_ESCALATION";
        case AuditEventType::DATA_READ: return "DATA_READ";
        case AuditEventType::DATA_WRITE: return "DATA_WRITE";
        case AuditEventType::DATA_DELETE: return "DATA_DELETE";
        case AuditEventType::DATA_EXPORT: return "DATA_EXPORT";
        case AuditEventType::CONFIG_CHANGED: return "CONFIG_CHANGED";
        case AuditEventType::PERMISSION_CHANGED: return "PERMISSION_CHANGED";
        case AuditEventType::USER_CREATED: return "USER_CREATED";
        case AuditEventType::USER_MODIFIED: return "USER_MODIFIED";
        case AuditEventType::USER_DELETED: return "USER_DELETED";
        case AuditEventType::SYSTEM_START: return "SYSTEM_START";
        case AuditEventType::SYSTEM_STOP: return "SYSTEM_STOP";
        case AuditEventType::ERROR_CRITICAL: return "ERROR_CRITICAL";
        case AuditEventType::SECURITY_ALERT: return "SECURITY_ALERT";
        default: return "UNKNOWN";
    }
}

// =============================================================================
// Audit Record
// =============================================================================

struct AuditRecord {
    UInt64 sequence_id{0};
    SystemTimePoint timestamp;
    AuditEventType event_type;
    String user_id;
    String session_id;
    String source_ip;
    String resource;
    String action;
    String outcome;  // "SUCCESS", "FAILURE", "DENIED"
    String details;
    HashMap<String, String> metadata;
    
    AuditRecord() : timestamp(SystemClock::now()) {}
    
    AuditRecord(AuditEventType type, const String& user, const String& res = "")
        : timestamp(SystemClock::now())
        , event_type(type)
        , user_id(user)
        , resource(res) {}
    
    void set_metadata(const String& key, const String& value) {
        metadata[key] = value;
    }
    
    String get_metadata(const String& key, const String& def = "") const {
        auto it = metadata.find(key);
        return it != metadata.end() ? it->second : def;
    }
    
    // Format as log line
    String to_log_line() const {
        std::ostringstream oss;
        
        // Format timestamp
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        
        oss << " | " << std::setw(6) << sequence_id
            << " | " << std::setw(20) << audit_event_type_to_string(event_type)
            << " | " << std::setw(12) << user_id
            << " | " << outcome
            << " | " << resource;
        
        if (!details.empty()) {
            oss << " | " << details;
        }
        
        return oss.str();
    }
    
    // Format as CSV
    String to_csv() const {
        std::ostringstream oss;
        
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        
        oss << "," << sequence_id
            << "," << audit_event_type_to_string(event_type)
            << "," << user_id
            << "," << session_id
            << "," << source_ip
            << "," << resource
            << "," << action
            << "," << outcome
            << ",\"" << details << "\"";
        
        return oss.str();
    }
};

// =============================================================================
// Audit Logger Interface
// =============================================================================

class IAuditLogger {
public:
    virtual ~IAuditLogger() = default;
    virtual void log(const AuditRecord& record) = 0;
    virtual void flush() = 0;
};

using AuditLoggerPtr = SharedPtr<IAuditLogger>;

// =============================================================================
// File Audit Logger
// =============================================================================

class FileAuditLogger : public IAuditLogger {
private:
    Path log_path_;
    std::ofstream file_;
    mutable Mutex mutex_;
    std::atomic<UInt64> sequence_{1};
    bool use_csv_{false};
    
public:
    explicit FileAuditLogger(const Path& path, bool csv_format = false)
        : log_path_(path), use_csv_(csv_format) {
        file_.open(path, std::ios::app);
        
        // Write CSV header if new file
        if (use_csv_ && file_.tellp() == 0) {
            file_ << "timestamp,sequence,event_type,user_id,session_id,source_ip,"
                  << "resource,action,outcome,details\n";
        }
    }
    
    ~FileAuditLogger() override {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void log(const AuditRecord& record) override {
        std::unique_lock lock(mutex_);
        
        if (!file_.is_open()) return;
        
        AuditRecord r = record;
        r.sequence_id = sequence_++;
        
        if (use_csv_) {
            file_ << r.to_csv() << "\n";
        } else {
            file_ << r.to_log_line() << "\n";
        }
    }
    
    void flush() override {
        std::unique_lock lock(mutex_);
        if (file_.is_open()) {
            file_.flush();
        }
    }
};

// =============================================================================
// Console Audit Logger (for debugging)
// =============================================================================

class ConsoleAuditLogger : public IAuditLogger {
private:
    mutable Mutex mutex_;
    std::atomic<UInt64> sequence_{1};
    
public:
    void log(const AuditRecord& record) override {
        std::unique_lock lock(mutex_);
        
        AuditRecord r = record;
        r.sequence_id = sequence_++;
        
        std::cout << "[AUDIT] " << r.to_log_line() << std::endl;
    }
    
    void flush() override {
        std::cout.flush();
    }
};

// =============================================================================
// Composite Audit Logger (multiple destinations)
// =============================================================================

class CompositeAuditLogger : public IAuditLogger {
private:
    Vector<AuditLoggerPtr> loggers_;
    mutable Mutex mutex_;
    
public:
    void add_logger(AuditLoggerPtr logger) {
        std::unique_lock lock(mutex_);
        loggers_.push_back(std::move(logger));
    }
    
    void log(const AuditRecord& record) override {
        std::unique_lock lock(mutex_);
        for (auto& logger : loggers_) {
            logger->log(record);
        }
    }
    
    void flush() override {
        std::unique_lock lock(mutex_);
        for (auto& logger : loggers_) {
            logger->flush();
        }
    }
};

// =============================================================================
// Audit Manager
// =============================================================================

class AuditManager {
private:
    AuditLoggerPtr logger_;
    mutable SharedMutex mutex_;
    std::atomic<bool> enabled_{true};
    String default_user_{"SYSTEM"};
    String default_session_;
    String default_source_ip_;
    
public:
    AuditManager() = default;
    
    void set_logger(AuditLoggerPtr logger) {
        std::unique_lock lock(mutex_);
        logger_ = std::move(logger);
    }
    
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_.load(); }
    
    void set_default_context(const String& user, const String& session = "", 
                            const String& ip = "") {
        std::unique_lock lock(mutex_);
        default_user_ = user;
        default_session_ = session;
        default_source_ip_ = ip;
    }
    
    void log(AuditRecord record) {
        if (!enabled_.load()) return;
        
        std::shared_lock lock(mutex_);
        if (!logger_) return;
        
        // Apply defaults if not set
        if (record.user_id.empty()) record.user_id = default_user_;
        if (record.session_id.empty()) record.session_id = default_session_;
        if (record.source_ip.empty()) record.source_ip = default_source_ip_;
        
        logger_->log(record);
    }
    
    // Convenience methods
    void log_login_success(const String& user, const String& ip = "") {
        AuditRecord r(AuditEventType::LOGIN_SUCCESS, user);
        r.source_ip = ip;
        r.outcome = "SUCCESS";
        log(std::move(r));
    }
    
    void log_login_failure(const String& user, const String& reason, const String& ip = "") {
        AuditRecord r(AuditEventType::LOGIN_FAILURE, user);
        r.source_ip = ip;
        r.outcome = "FAILURE";
        r.details = reason;
        log(std::move(r));
    }
    
    void log_access(AuditEventType type, const String& user, const String& resource,
                   bool granted, const String& action = "") {
        AuditRecord r(type, user, resource);
        r.action = action;
        r.outcome = granted ? "GRANTED" : "DENIED";
        log(std::move(r));
    }
    
    void log_data_access(const String& user, const String& dataset, 
                        const String& operation, bool success = true) {
        AuditEventType type = AuditEventType::DATA_READ;
        if (operation == "WRITE" || operation == "INSERT" || operation == "UPDATE") {
            type = AuditEventType::DATA_WRITE;
        } else if (operation == "DELETE") {
            type = AuditEventType::DATA_DELETE;
        }
        
        AuditRecord r(type, user, dataset);
        r.action = operation;
        r.outcome = success ? "SUCCESS" : "FAILURE";
        log(std::move(r));
    }
    
    void log_security_alert(const String& message, const String& user = "") {
        AuditRecord r(AuditEventType::SECURITY_ALERT, user);
        r.details = message;
        r.outcome = "ALERT";
        log(std::move(r));
    }
    
    void flush() {
        std::shared_lock lock(mutex_);
        if (logger_) {
            logger_->flush();
        }
    }
};

// =============================================================================
// Global Audit Manager
// =============================================================================

inline AuditManager& global_audit_manager() {
    static AuditManager instance;
    return instance;
}

// =============================================================================
// Audit Macros
// =============================================================================

#define IMS_AUDIT_LOG(record) \
    ims::global_audit_manager().log(record)

#define IMS_AUDIT_LOGIN_SUCCESS(user, ip) \
    ims::global_audit_manager().log_login_success(user, ip)

#define IMS_AUDIT_LOGIN_FAILURE(user, reason, ip) \
    ims::global_audit_manager().log_login_failure(user, reason, ip)

#define IMS_AUDIT_DATA_ACCESS(user, dataset, operation) \
    ims::global_audit_manager().log_data_access(user, dataset, operation)

#define IMS_AUDIT_SECURITY_ALERT(message) \
    ims::global_audit_manager().log_security_alert(message)

} // namespace ims
