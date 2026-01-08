#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Logging System
// Version: 3.6.3
// =============================================================================

#include "types.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <source_location>

namespace ims {

// =============================================================================
// Log Levels
// =============================================================================

enum class LogLevel : UInt8 {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5,
    OFF = 6
};

inline StringView log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:    return "TRACE";
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARN";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        case LogLevel::OFF:      return "OFF";
    }
    return "UNKNOWN";
}

// =============================================================================
// Logger Interface
// =============================================================================

class ILogger {
public:
    virtual ~ILogger() = default;
    
    virtual void log(LogLevel level, StringView message,
                     const std::source_location& loc = std::source_location::current()) = 0;
    
    virtual void set_level(LogLevel level) = 0;
    virtual LogLevel get_level() const = 0;
    virtual void flush() = 0;
    
    // Convenience methods
    void trace(StringView msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::TRACE, msg, loc);
    }
    
    void debug(StringView msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::DEBUG, msg, loc);
    }
    
    void info(StringView msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::INFO, msg, loc);
    }
    
    void warning(StringView msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::WARNING, msg, loc);
    }
    
    void error(StringView msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::ERROR, msg, loc);
    }
    
    void critical(StringView msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::CRITICAL, msg, loc);
    }
};

// =============================================================================
// Console Logger
// =============================================================================

class ConsoleLogger : public ILogger {
private:
    LogLevel level_{LogLevel::INFO};
    mutable Mutex mutex_;
    bool use_colors_{true};
    
    String get_color(LogLevel level) const {
        if (!use_colors_) return "";
        switch (level) {
            case LogLevel::TRACE:    return "\033[90m";  // Gray
            case LogLevel::DEBUG:    return "\033[36m";  // Cyan
            case LogLevel::INFO:     return "\033[32m";  // Green
            case LogLevel::WARNING:  return "\033[33m";  // Yellow
            case LogLevel::ERROR:    return "\033[31m";  // Red
            case LogLevel::CRITICAL: return "\033[35m";  // Magenta
            default: return "";
        }
    }
    
    String get_reset() const {
        return use_colors_ ? "\033[0m" : "";
    }
    
public:
    ConsoleLogger() = default;
    explicit ConsoleLogger(LogLevel level) : level_(level) {}
    
    void set_use_colors(bool use) { use_colors_ = use; }
    
    void log(LogLevel level, StringView message,
             [[maybe_unused]] const std::source_location& loc = std::source_location::current()) override {
        if (level < level_) return;
        
        auto now = SystemClock::now();
        auto time = SystemClock::to_time_t(now);
        
        LockGuard<Mutex> lock(mutex_);
        
        std::cout << get_color(level)
                  << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] "
                  << "[" << log_level_to_string(level) << "] "
                  << message
                  << get_reset()
                  << std::endl;
    }
    
    void set_level(LogLevel level) override { level_ = level; }
    LogLevel get_level() const override { return level_; }
    void flush() override { std::cout.flush(); }
};

// =============================================================================
// File Logger
// =============================================================================

class FileLogger : public ILogger {
private:
    LogLevel level_{LogLevel::INFO};
    mutable Mutex mutex_;
    std::ofstream file_;
    Path file_path_;
    
public:
    explicit FileLogger(const Path& path, LogLevel level = LogLevel::INFO)
        : level_(level), file_path_(path) {
        file_.open(path, std::ios::app);
    }
    
    ~FileLogger() override {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void log(LogLevel level, StringView message,
             const std::source_location& loc = std::source_location::current()) override {
        if (level < level_ || !file_.is_open()) return;
        
        auto now = SystemClock::now();
        auto time = SystemClock::to_time_t(now);
        
        LockGuard<Mutex> lock(mutex_);
        
        file_ << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] "
              << "[" << log_level_to_string(level) << "] "
              << "[" << loc.file_name() << ":" << loc.line() << "] "
              << message << "\n";
    }
    
    void set_level(LogLevel level) override { level_ = level; }
    LogLevel get_level() const override { return level_; }
    void flush() override { 
        LockGuard<Mutex> lock(mutex_);
        if (file_.is_open()) file_.flush(); 
    }
};

// =============================================================================
// Global Logger Access
// =============================================================================

inline SharedPtr<ILogger>& global_logger() {
    static SharedPtr<ILogger> logger = std::make_shared<ConsoleLogger>();
    return logger;
}

inline void set_global_logger(SharedPtr<ILogger> logger) {
    global_logger() = std::move(logger);
}

// Convenience macros
#define IMS_LOG_TRACE(msg)    if (ims::global_logger()) ims::global_logger()->trace(msg)
#define IMS_LOG_DEBUG(msg)    if (ims::global_logger()) ims::global_logger()->debug(msg)
#define IMS_LOG_INFO(msg)     if (ims::global_logger()) ims::global_logger()->info(msg)
#define IMS_LOG_WARNING(msg)  if (ims::global_logger()) ims::global_logger()->warning(msg)
#define IMS_LOG_ERROR(msg)    if (ims::global_logger()) ims::global_logger()->error(msg)
#define IMS_LOG_CRITICAL(msg) if (ims::global_logger()) ims::global_logger()->critical(msg)

} // namespace ims
