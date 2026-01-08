#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Retry Mechanism
// Version: 3.6.2
// =============================================================================
//
// Provides retry functionality with exponential backoff for handling
// transient failures in mainframe operations.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include "error.hpp"
#include <random>

namespace ims {

// =============================================================================
// Retry Policy Configuration
// =============================================================================

struct RetryPolicy {
    UInt32 max_attempts{3};              // Maximum number of attempts
    Milliseconds initial_delay{100};      // Initial delay between retries
    Milliseconds max_delay{10000};        // Maximum delay between retries
    double multiplier{2.0};               // Backoff multiplier
    double jitter_factor{0.1};            // Random jitter factor (0-1)
    bool retry_on_timeout{true};          // Retry on timeout errors
    Set<ImsErrorCode> retryable_errors;   // Additional retryable error codes
    
    RetryPolicy() {
        // Default retryable errors
        retryable_errors.insert(ImsErrorCode::TIMEOUT);
        retryable_errors.insert(ImsErrorCode::DEADLOCK_DETECTED);
        retryable_errors.insert(ImsErrorCode::RESOURCE_EXHAUSTED);
        retryable_errors.insert(ImsErrorCode::DATABASE_NOT_AVAILABLE);
    }
    
    static RetryPolicy none() {
        RetryPolicy policy;
        policy.max_attempts = 1;
        return policy;
    }
    
    static RetryPolicy aggressive() {
        RetryPolicy policy;
        policy.max_attempts = 5;
        policy.initial_delay = Milliseconds(50);
        policy.max_delay = Milliseconds(5000);
        return policy;
    }
    
    static RetryPolicy conservative() {
        RetryPolicy policy;
        policy.max_attempts = 3;
        policy.initial_delay = Milliseconds(500);
        policy.max_delay = Milliseconds(30000);
        policy.multiplier = 3.0;
        return policy;
    }
    
    bool is_retryable(ImsErrorCode code) const {
        return retryable_errors.find(code) != retryable_errors.end();
    }
};

// =============================================================================
// Retry Result
// =============================================================================

template<typename T>
struct RetryResult {
    Optional<T> value;
    UInt32 attempts{0};
    Milliseconds total_delay{0};
    Vector<String> errors;
    bool succeeded{false};
    
    bool has_value() const { return succeeded && value.has_value(); }
    const T& get() const { return value.value(); }
};

template<>
struct RetryResult<void> {
    UInt32 attempts{0};
    Milliseconds total_delay{0};
    Vector<String> errors;
    bool succeeded{false};
    
    bool has_value() const { return succeeded; }
};

// =============================================================================
// Retry Executor
// =============================================================================

class RetryExecutor {
private:
    RetryPolicy policy_;
    mutable std::mt19937 rng_{std::random_device{}()};
    
    Milliseconds calculate_delay(UInt32 attempt) const {
        // Calculate base delay with exponential backoff
        double base_delay = static_cast<double>(policy_.initial_delay.count()) * 
                           std::pow(policy_.multiplier, static_cast<double>(attempt - 1));
        
        // Cap at max delay
        base_delay = std::min(base_delay, static_cast<double>(policy_.max_delay.count()));
        
        // Add jitter
        if (policy_.jitter_factor > 0) {
            std::uniform_real_distribution<double> dist(
                1.0 - policy_.jitter_factor, 
                1.0 + policy_.jitter_factor
            );
            base_delay *= dist(rng_);
        }
        
        return Milliseconds(static_cast<Int64>(base_delay));
    }
    
public:
    explicit RetryExecutor(const RetryPolicy& policy = RetryPolicy{}) 
        : policy_(policy) {}
    
    RetryExecutor(const RetryExecutor&) = delete;
    RetryExecutor& operator=(const RetryExecutor&) = delete;
    
    const RetryPolicy& policy() const { return policy_; }
    void set_policy(const RetryPolicy& policy) { policy_ = policy; }
    
    // Execute with return value
    template<typename Func>
    auto execute(Func&& func) -> RetryResult<decltype(func())> {
        using ResultType = decltype(func());
        RetryResult<ResultType> result;
        
        for (UInt32 attempt = 1; attempt <= policy_.max_attempts; ++attempt) {
            result.attempts = attempt;
            
            try {
                if constexpr (std::is_void_v<ResultType>) {
                    func();
                    result.succeeded = true;
                    return result;
                } else {
                    result.value = func();
                    result.succeeded = true;
                    return result;
                }
            } catch (const ImsException& e) {
                result.errors.push_back(std::format("Attempt {}: {} (code: {})", 
                    attempt, e.what(), static_cast<int>(e.code())));
                
                if (!policy_.is_retryable(e.code())) {
                    // Non-retryable error
                    return result;
                }
                
                if (attempt < policy_.max_attempts) {
                    auto delay = calculate_delay(attempt);
                    result.total_delay += delay;
                    std::this_thread::sleep_for(delay);
                }
            } catch (const std::exception& e) {
                result.errors.push_back(std::format("Attempt {}: {}", attempt, e.what()));
                
                if (attempt < policy_.max_attempts) {
                    auto delay = calculate_delay(attempt);
                    result.total_delay += delay;
                    std::this_thread::sleep_for(delay);
                }
            }
        }
        
        return result;
    }
    
    // Execute with ErrorResult return type
    template<typename T, typename Func>
    RetryResult<T> execute_with_result(Func&& func) {
        RetryResult<T> result;
        
        for (UInt32 attempt = 1; attempt <= policy_.max_attempts; ++attempt) {
            result.attempts = attempt;
            
            auto op_result = func();
            
            if (op_result.has_value()) {
                if constexpr (!std::is_void_v<T>) {
                    result.value = std::move(op_result).value();
                }
                result.succeeded = true;
                return result;
            }
            
            result.errors.push_back(std::format("Attempt {}: {}", attempt, op_result.error()));
            
            if (attempt < policy_.max_attempts) {
                auto delay = calculate_delay(attempt);
                result.total_delay += delay;
                std::this_thread::sleep_for(delay);
            }
        }
        
        return result;
    }
};

// =============================================================================
// Convenience Function
// =============================================================================

template<typename Func>
auto with_retry(Func&& func, const RetryPolicy& policy = RetryPolicy{}) 
    -> RetryResult<decltype(func())> {
    RetryExecutor executor(policy);
    return executor.execute(std::forward<Func>(func));
}

template<typename Func>
auto with_retry(Func&& func, UInt32 max_attempts) 
    -> RetryResult<decltype(func())> {
    RetryPolicy policy;
    policy.max_attempts = max_attempts;
    return with_retry(std::forward<Func>(func), policy);
}

} // namespace ims
