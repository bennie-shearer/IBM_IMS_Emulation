// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Circuit Breaker
// Version: 3.6.3
// =============================================================================
//
// Circuit breaker pattern for fault tolerance and graceful degradation.
// Prevents cascading failures by temporarily blocking failing operations.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#pragma once

#include "types.hpp"
#include "error.hpp"
#include <functional>
#include <chrono>
#include <atomic>

namespace ims {

// =============================================================================
// Circuit Breaker State
// =============================================================================

enum class CircuitState {
    CLOSED,      // Normal operation - requests flow through
    OPEN,        // Failure threshold reached - requests blocked
    HALF_OPEN    // Testing recovery - limited requests allowed
};

inline String circuit_state_to_string(CircuitState state) {
    switch (state) {
        case CircuitState::CLOSED:    return "CLOSED";
        case CircuitState::OPEN:      return "OPEN";
        case CircuitState::HALF_OPEN: return "HALF_OPEN";
        default: return "UNKNOWN";
    }
}

// =============================================================================
// Circuit Breaker Configuration
// =============================================================================

struct CircuitBreakerConfig {
    Size failure_threshold{5};           // Failures before opening
    Size success_threshold{3};           // Successes to close from half-open
    Milliseconds reset_timeout{30000};   // Time before attempting half-open
    Milliseconds half_open_timeout{5000}; // Max time in half-open state
    Size half_open_max_requests{1};      // Requests allowed in half-open
};

// =============================================================================
// Circuit Breaker Statistics
// =============================================================================

struct CircuitBreakerStats {
    std::atomic<Size> total_requests{0};
    std::atomic<Size> successful_requests{0};
    std::atomic<Size> failed_requests{0};
    std::atomic<Size> rejected_requests{0};
    std::atomic<Size> state_transitions{0};
    TimePoint last_failure{};
    TimePoint last_success{};
    TimePoint last_state_change{};
    
    double success_rate() const {
        Size total = total_requests.load();
        if (total == 0) return 100.0;
        return (static_cast<double>(successful_requests.load()) / 
                static_cast<double>(total)) * 100.0;
    }
    
    double failure_rate() const {
        return 100.0 - success_rate();
    }
    
    void reset() {
        total_requests = 0;
        successful_requests = 0;
        failed_requests = 0;
        rejected_requests = 0;
        state_transitions = 0;
    }
};

// =============================================================================
// Circuit Breaker
// =============================================================================

class CircuitBreaker {
public:
    using StateChangeCallback = std::function<void(CircuitState, CircuitState)>;
    
private:
    String name_;
    CircuitBreakerConfig config_;
    CircuitBreakerStats stats_;
    StateChangeCallback state_callback_;
    
    mutable Mutex mutex_;
    CircuitState state_{CircuitState::CLOSED};
    Size consecutive_failures_{0};
    Size consecutive_successes_{0};
    Size half_open_requests_{0};
    TimePoint opened_at_{};
    
    void transition_to(CircuitState new_state) {
        if (state_ == new_state) return;
        
        CircuitState old_state = state_;
        state_ = new_state;
        stats_.last_state_change = Clock::now();
        ++stats_.state_transitions;
        
        // Reset counters on state change
        if (new_state == CircuitState::CLOSED) {
            consecutive_failures_ = 0;
            consecutive_successes_ = 0;
        } else if (new_state == CircuitState::OPEN) {
            opened_at_ = Clock::now();
            consecutive_successes_ = 0;
        } else if (new_state == CircuitState::HALF_OPEN) {
            half_open_requests_ = 0;
            consecutive_successes_ = 0;
        }
        
        // Notify callback outside lock
        if (state_callback_) {
            state_callback_(old_state, new_state);
        }
    }
    
    bool should_attempt_reset() const {
        if (state_ != CircuitState::OPEN) return false;
        auto elapsed = Clock::now() - opened_at_;
        return elapsed >= config_.reset_timeout;
    }
    
public:
    explicit CircuitBreaker(String name, 
                           const CircuitBreakerConfig& config = CircuitBreakerConfig{})
        : name_(std::move(name))
        , config_(config) {
        stats_.last_state_change = Clock::now();
    }
    
    void set_state_callback(StateChangeCallback callback) {
        LockGuard<Mutex> lock(mutex_);
        state_callback_ = std::move(callback);
    }
    
    // Check if request is allowed
    bool allow_request() {
        LockGuard<Mutex> lock(mutex_);
        ++stats_.total_requests;
        
        switch (state_) {
            case CircuitState::CLOSED:
                return true;
                
            case CircuitState::OPEN:
                if (should_attempt_reset()) {
                    transition_to(CircuitState::HALF_OPEN);
                    ++half_open_requests_;
                    return true;
                }
                ++stats_.rejected_requests;
                return false;
                
            case CircuitState::HALF_OPEN:
                if (half_open_requests_ < config_.half_open_max_requests) {
                    ++half_open_requests_;
                    return true;
                }
                ++stats_.rejected_requests;
                return false;
        }
        return false;
    }
    
    // Record success
    void record_success() {
        LockGuard<Mutex> lock(mutex_);
        ++stats_.successful_requests;
        stats_.last_success = Clock::now();
        consecutive_failures_ = 0;
        ++consecutive_successes_;
        
        if (state_ == CircuitState::HALF_OPEN) {
            if (consecutive_successes_ >= config_.success_threshold) {
                transition_to(CircuitState::CLOSED);
            }
        }
    }
    
    // Record failure
    void record_failure() {
        LockGuard<Mutex> lock(mutex_);
        ++stats_.failed_requests;
        stats_.last_failure = Clock::now();
        consecutive_successes_ = 0;
        ++consecutive_failures_;
        
        switch (state_) {
            case CircuitState::CLOSED:
                if (consecutive_failures_ >= config_.failure_threshold) {
                    transition_to(CircuitState::OPEN);
                }
                break;
                
            case CircuitState::HALF_OPEN:
                // Any failure in half-open immediately opens
                transition_to(CircuitState::OPEN);
                break;
                
            case CircuitState::OPEN:
                // Already open, just update opened_at
                opened_at_ = Clock::now();
                break;
        }
    }
    
    // Execute with circuit breaker protection
    template<typename F>
    ErrorResult<typename std::invoke_result_t<F>> execute(F&& func) {
        using ResultType = typename std::invoke_result_t<F>;
        
        if (!allow_request()) {
            return make_error<ResultType>("Circuit breaker is open: " + name_);
        }
        
        try {
            auto result = func();
            record_success();
            return result;
        } catch (const std::exception& e) {
            record_failure();
            return make_error<ResultType>(String("Operation failed: ") + e.what());
        } catch (...) {
            record_failure();
            return make_error<ResultType>("Operation failed with unknown error");
        }
    }
    
    // Execute void function
    ErrorResult<void> execute_void(std::function<void()> func) {
        if (!allow_request()) {
            return make_error("Circuit breaker is open: " + name_);
        }
        
        try {
            func();
            record_success();
            return make_success();
        } catch (const std::exception& e) {
            record_failure();
            return make_error(String("Operation failed: ") + e.what());
        } catch (...) {
            record_failure();
            return make_error("Operation failed with unknown error");
        }
    }
    
    // Getters
    const String& name() const { return name_; }
    CircuitState state() const {
        LockGuard<Mutex> lock(mutex_);
        return state_;
    }
    String state_string() const {
        return circuit_state_to_string(state());
    }
    const CircuitBreakerStats& statistics() const { return stats_; }
    const CircuitBreakerConfig& config() const { return config_; }
    
    bool is_closed() const { return state() == CircuitState::CLOSED; }
    bool is_open() const { return state() == CircuitState::OPEN; }
    bool is_half_open() const { return state() == CircuitState::HALF_OPEN; }
    
    // Manual controls
    void force_open() {
        LockGuard<Mutex> lock(mutex_);
        transition_to(CircuitState::OPEN);
    }
    
    void force_close() {
        LockGuard<Mutex> lock(mutex_);
        transition_to(CircuitState::CLOSED);
    }
    
    void reset() {
        LockGuard<Mutex> lock(mutex_);
        transition_to(CircuitState::CLOSED);
        stats_.reset();
        consecutive_failures_ = 0;
        consecutive_successes_ = 0;
        half_open_requests_ = 0;
    }
};

// =============================================================================
// Circuit Breaker Registry
// =============================================================================

class CircuitBreakerRegistry {
private:
    HashMap<String, std::unique_ptr<CircuitBreaker>> breakers_;
    mutable SharedMutex mutex_;
    
public:
    CircuitBreaker& get_or_create(const String& name, 
                                  const CircuitBreakerConfig& config = CircuitBreakerConfig{}) {
        {
            SharedLock<SharedMutex> lock(mutex_);
            auto it = breakers_.find(name);
            if (it != breakers_.end()) {
                return *it->second;
            }
        }
        
        LockGuard<SharedMutex> lock(mutex_);
        // Double-check after acquiring exclusive lock
        auto it = breakers_.find(name);
        if (it != breakers_.end()) {
            return *it->second;
        }
        
        auto breaker = std::make_unique<CircuitBreaker>(name, config);
        auto& ref = *breaker;
        breakers_[name] = std::move(breaker);
        return ref;
    }
    
    CircuitBreaker* get(const String& name) {
        SharedLock<SharedMutex> lock(mutex_);
        auto it = breakers_.find(name);
        return it != breakers_.end() ? it->second.get() : nullptr;
    }
    
    bool remove(const String& name) {
        LockGuard<SharedMutex> lock(mutex_);
        return breakers_.erase(name) > 0;
    }
    
    Vector<String> names() const {
        SharedLock<SharedMutex> lock(mutex_);
        Vector<String> result;
        result.reserve(breakers_.size());
        for (const auto& [name, _] : breakers_) {
            result.push_back(name);
        }
        return result;
    }
    
    Size size() const {
        SharedLock<SharedMutex> lock(mutex_);
        return breakers_.size();
    }
    
    void reset_all() {
        SharedLock<SharedMutex> lock(mutex_);
        for (auto& [_, breaker] : breakers_) {
            breaker->reset();
        }
    }
};

// =============================================================================
// Global Circuit Breaker Registry
// =============================================================================

inline CircuitBreakerRegistry& global_circuit_breakers() {
    static CircuitBreakerRegistry registry;
    return registry;
}

} // namespace ims
