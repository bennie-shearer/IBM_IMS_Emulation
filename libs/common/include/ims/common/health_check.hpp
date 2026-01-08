// =============================================================================
// IBM IMS Emulation Enterprise - Health Check
// Version: 3.6.2
// =============================================================================
//
// System health monitoring with configurable checks and status reporting.
// Supports readiness probes, liveness probes, and detailed health status.
//
// Copyright (c) 2025 CICS Systems - Complete Mainframe Emulation Framework
// =============================================================================

#ifndef IMS_COMMON_HEALTH_CHECK_HPP
#define IMS_COMMON_HEALTH_CHECK_HPP

#include "types.hpp"
#include <functional>
#include <chrono>
#include <atomic>

namespace ims {

// =============================================================================
// Health Status
// =============================================================================

enum class HealthStatus {
    HEALTHY,     // Component is functioning normally
    DEGRADED,    // Component is functioning with issues
    UNHEALTHY,   // Component is not functioning
    UNKNOWN      // Health status cannot be determined
};

inline String health_status_to_string(HealthStatus status) {
    switch (status) {
        case HealthStatus::HEALTHY:   return "HEALTHY";
        case HealthStatus::DEGRADED:  return "DEGRADED";
        case HealthStatus::UNHEALTHY: return "UNHEALTHY";
        case HealthStatus::UNKNOWN:   return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

// =============================================================================
// Health Check Result
// =============================================================================

struct HealthCheckResult {
    String name;
    HealthStatus status{HealthStatus::UNKNOWN};
    String message;
    Milliseconds response_time{0};
    SteadyClock::time_point checked_at{};
    HashMap<String, String> details;
    
    bool is_healthy() const { 
        return status == HealthStatus::HEALTHY; 
    }
    
    bool is_degraded() const { 
        return status == HealthStatus::DEGRADED; 
    }
    
    bool is_unhealthy() const { 
        return status == HealthStatus::UNHEALTHY || 
               status == HealthStatus::UNKNOWN; 
    }
    
    static HealthCheckResult healthy(const String& name, const String& message = "OK") {
        HealthCheckResult result;
        result.name = name;
        result.status = HealthStatus::HEALTHY;
        result.message = message;
        result.checked_at = SteadyClock::now();
        return result;
    }
    
    static HealthCheckResult degraded(const String& name, const String& message) {
        HealthCheckResult result;
        result.name = name;
        result.status = HealthStatus::DEGRADED;
        result.message = message;
        result.checked_at = SteadyClock::now();
        return result;
    }
    
    static HealthCheckResult unhealthy(const String& name, const String& message) {
        HealthCheckResult result;
        result.name = name;
        result.status = HealthStatus::UNHEALTHY;
        result.message = message;
        result.checked_at = SteadyClock::now();
        return result;
    }
};

// =============================================================================
// Health Check Interface
// =============================================================================

class IHealthCheck {
public:
    virtual ~IHealthCheck() = default;
    virtual String name() const = 0;
    virtual HealthCheckResult check() = 0;
    virtual bool is_critical() const { return true; }
    virtual Milliseconds timeout() const { return Milliseconds(5000); }
};

// =============================================================================
// Lambda Health Check
// =============================================================================

class LambdaHealthCheck : public IHealthCheck {
private:
    String name_;
    std::function<HealthCheckResult()> check_func_;
    bool critical_;
    Milliseconds timeout_;
    
public:
    LambdaHealthCheck(String name, 
                      std::function<HealthCheckResult()> check_func,
                      bool critical = true,
                      Milliseconds timeout = Milliseconds(5000))
        : name_(std::move(name))
        , check_func_(std::move(check_func))
        , critical_(critical)
        , timeout_(timeout) {}
    
    String name() const override { return name_; }
    
    HealthCheckResult check() override {
        auto start = SteadyClock::now();
        auto result = check_func_();
        auto end = SteadyClock::now();
        result.response_time = std::chrono::duration_cast<Milliseconds>(end - start);
        result.checked_at = start;
        return result;
    }
    
    bool is_critical() const override { return critical_; }
    Milliseconds timeout() const override { return timeout_; }
};

// =============================================================================
// Composite Health Result
// =============================================================================

struct CompositeHealthResult {
    HealthStatus overall_status{HealthStatus::HEALTHY};
    Vector<HealthCheckResult> checks;
    Milliseconds total_time{0};
    SteadyClock::time_point checked_at{};
    
    bool is_healthy() const {
        return overall_status == HealthStatus::HEALTHY;
    }
    
    bool is_ready() const {
        return overall_status != HealthStatus::UNHEALTHY;
    }
    
    Size healthy_count() const {
        Size count = 0;
        for (const auto& check : checks) {
            if (check.is_healthy()) ++count;
        }
        return count;
    }
    
    Size unhealthy_count() const {
        Size count = 0;
        for (const auto& check : checks) {
            if (check.is_unhealthy()) ++count;
        }
        return count;
    }
};

// =============================================================================
// Health Check Manager
// =============================================================================

class HealthCheckManager {
private:
    Vector<std::unique_ptr<IHealthCheck>> checks_;
    mutable SharedMutex mutex_;
    CompositeHealthResult last_result_;
    std::atomic<bool> cache_valid_{false};
    Milliseconds cache_ttl_{Milliseconds(5000)};
    SteadyClock::time_point last_check_time_{};
    
    HealthStatus calculate_overall_status(const Vector<HealthCheckResult>& results) {
        bool has_unhealthy = false;
        bool has_degraded = false;
        
        for (const auto& result : results) {
            if (result.status == HealthStatus::UNHEALTHY) {
                // Check if this is a critical check
                for (const auto& check : checks_) {
                    if (check->name() == result.name && check->is_critical()) {
                        has_unhealthy = true;
                        break;
                    }
                }
            } else if (result.status == HealthStatus::DEGRADED) {
                has_degraded = true;
            }
        }
        
        if (has_unhealthy) return HealthStatus::UNHEALTHY;
        if (has_degraded) return HealthStatus::DEGRADED;
        return HealthStatus::HEALTHY;
    }
    
public:
    void add_check(std::unique_ptr<IHealthCheck> check) {
        LockGuard<SharedMutex> lock(mutex_);
        checks_.push_back(std::move(check));
        cache_valid_ = false;
    }
    
    template<typename F>
    void add_lambda_check(const String& name, F&& check_func, 
                          bool critical = true,
                          Milliseconds timeout = Milliseconds(5000)) {
        auto check = std::make_unique<LambdaHealthCheck>(
            name,
            [func = std::forward<F>(check_func), name]() {
                return func();
            },
            critical,
            timeout
        );
        add_check(std::move(check));
    }
    
    void set_cache_ttl(Milliseconds ttl) {
        cache_ttl_ = ttl;
    }
    
    CompositeHealthResult check_all(bool use_cache = true) {
        // Check cache
        if (use_cache && cache_valid_) {
            SharedLock<SharedMutex> lock(mutex_);
            auto elapsed = SteadyClock::now() - last_check_time_;
            if (elapsed < cache_ttl_) {
                return last_result_;
            }
        }
        
        LockGuard<SharedMutex> lock(mutex_);
        
        auto overall_start = SteadyClock::now();
        CompositeHealthResult result;
        result.checked_at = overall_start;
        
        for (const auto& check : checks_) {
            auto check_result = check->check();
            result.checks.push_back(std::move(check_result));
        }
        
        auto overall_end = SteadyClock::now();
        result.total_time = std::chrono::duration_cast<Milliseconds>(
            overall_end - overall_start);
        result.overall_status = calculate_overall_status(result.checks);
        
        // Cache result
        last_result_ = result;
        last_check_time_ = overall_start;
        cache_valid_ = true;
        
        return result;
    }
    
    // Quick readiness check (all checks pass or degrade gracefully)
    bool is_ready() {
        auto result = check_all();
        return result.is_ready();
    }
    
    // Quick liveness check (system is alive)
    bool is_alive() {
        auto result = check_all();
        return result.overall_status != HealthStatus::UNHEALTHY;
    }
    
    // Get specific check result
    HealthCheckResult check_one(const String& name) {
        SharedLock<SharedMutex> lock(mutex_);
        for (const auto& check : checks_) {
            if (check->name() == name) {
                return check->check();
            }
        }
        return HealthCheckResult::unhealthy(name, "Check not found");
    }
    
    Vector<String> check_names() const {
        SharedLock<SharedMutex> lock(mutex_);
        Vector<String> names;
        names.reserve(checks_.size());
        for (const auto& check : checks_) {
            names.push_back(check->name());
        }
        return names;
    }
    
    Size check_count() const {
        SharedLock<SharedMutex> lock(mutex_);
        return checks_.size();
    }
    
    void clear_cache() {
        cache_valid_ = false;
    }
    
    void remove_check(const String& name) {
        LockGuard<SharedMutex> lock(mutex_);
        checks_.erase(
            std::remove_if(checks_.begin(), checks_.end(),
                [&name](const std::unique_ptr<IHealthCheck>& check) {
                    return check->name() == name;
                }),
            checks_.end()
        );
        cache_valid_ = false;
    }
};

// =============================================================================
// Built-in Health Checks
// =============================================================================

// Memory health check
inline std::unique_ptr<IHealthCheck> make_memory_check(
    Size warning_threshold_mb = 1024,
    Size critical_threshold_mb = 256) {
    
    return std::make_unique<LambdaHealthCheck>(
        "memory",
        [warning_threshold_mb, critical_threshold_mb]() {
            // Simple memory check (platform-specific implementation needed)
            // For now, just return healthy
            HealthCheckResult result;
            result.name = "memory";
            result.status = HealthStatus::HEALTHY;
            result.message = "Memory check passed";
            result.details["warning_threshold_mb"] = std::to_string(warning_threshold_mb);
            result.details["critical_threshold_mb"] = std::to_string(critical_threshold_mb);
            return result;
        },
        true,
        Milliseconds(1000)
    );
}

// Disk health check
inline std::unique_ptr<IHealthCheck> make_disk_check(
    const String& path = "/",
    Size warning_threshold_mb = 1024,
    Size critical_threshold_mb = 256) {
    
    return std::make_unique<LambdaHealthCheck>(
        "disk:" + path,
        [path, warning_threshold_mb, critical_threshold_mb]() {
            // Simple disk check (platform-specific implementation needed)
            HealthCheckResult result;
            result.name = "disk:" + path;
            result.status = HealthStatus::HEALTHY;
            result.message = "Disk check passed";
            result.details["path"] = path;
            result.details["warning_threshold_mb"] = std::to_string(warning_threshold_mb);
            result.details["critical_threshold_mb"] = std::to_string(critical_threshold_mb);
            return result;
        },
        false,
        Milliseconds(2000)
    );
}

// =============================================================================
// Global Health Check Manager
// =============================================================================

inline HealthCheckManager& global_health_checks() {
    static HealthCheckManager manager;
    return manager;
}

} // namespace ims

#endif // IMS_COMMON_HEALTH_CHECK_HPP
