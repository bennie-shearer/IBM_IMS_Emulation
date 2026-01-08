// =============================================================================
// IBM IMS Emulation Enterprise - Rate Limiter
// Version: 3.6.2
// =============================================================================
//
// Token bucket and sliding window rate limiting for request throttling.
// Supports multiple rate limit policies and distributed scenarios.
//
// Copyright (c) 2025 CICS Systems - Complete Mainframe Emulation Framework
// =============================================================================

#ifndef IMS_COMMON_RATE_LIMITER_HPP
#define IMS_COMMON_RATE_LIMITER_HPP

#include "types.hpp"
#include <chrono>
#include <deque>
#include <atomic>
#include <cmath>

namespace ims {

// =============================================================================
// Rate Limit Result
// =============================================================================

struct RateLimitResult {
    bool allowed{false};
    Size remaining{0};
    Milliseconds retry_after{0};
    SteadyClock::time_point reset_time{};
    
    bool is_allowed() const { return allowed; }
    bool should_retry() const { return !allowed && retry_after.count() > 0; }
};

// =============================================================================
// Token Bucket Rate Limiter
// =============================================================================

class TokenBucketLimiter {
private:
    Size capacity_;              // Maximum tokens
    double refill_rate_;         // Tokens per second
    mutable double tokens_;      // Current tokens
    mutable SteadyClock::time_point last_refill_;
    mutable Mutex mutex_;
    
    void refill() const {
        auto now = SteadyClock::now();
        auto elapsed = std::chrono::duration<double>(now - last_refill_).count();
        tokens_ = std::min(static_cast<double>(capacity_), 
                          tokens_ + elapsed * refill_rate_);
        last_refill_ = now;
    }
    
public:
    TokenBucketLimiter(Size capacity, double refill_rate_per_second)
        : capacity_(capacity)
        , refill_rate_(refill_rate_per_second)
        , tokens_(static_cast<double>(capacity))
        , last_refill_(SteadyClock::now()) {}
    
    RateLimitResult try_acquire(Size tokens = 1) {
        LockGuard<Mutex> lock(mutex_);
        refill();
        
        RateLimitResult result;
        
        if (tokens_ >= static_cast<double>(tokens)) {
            tokens_ -= static_cast<double>(tokens);
            result.allowed = true;
            result.remaining = static_cast<Size>(tokens_);
        } else {
            result.allowed = false;
            result.remaining = static_cast<Size>(tokens_);
            // Calculate time to wait for enough tokens
            double tokens_needed = static_cast<double>(tokens) - tokens_;
            double seconds_to_wait = tokens_needed / refill_rate_;
            result.retry_after = Milliseconds(static_cast<Int64>(seconds_to_wait * 1000));
            result.reset_time = SteadyClock::now() + 
                std::chrono::duration_cast<SteadyClock::duration>(
                    std::chrono::duration<double>(seconds_to_wait));
        }
        
        return result;
    }
    
    bool acquire(Size tokens = 1) {
        return try_acquire(tokens).allowed;
    }
    
    Size available() const {
        LockGuard<Mutex> lock(mutex_);
        refill();
        return static_cast<Size>(tokens_);
    }
    
    Size capacity() const { return capacity_; }
    double refill_rate() const { return refill_rate_; }
    
    void reset() {
        LockGuard<Mutex> lock(mutex_);
        tokens_ = static_cast<double>(capacity_);
        last_refill_ = SteadyClock::now();
    }
};

// =============================================================================
// Sliding Window Rate Limiter
// =============================================================================

class SlidingWindowLimiter {
private:
    Size max_requests_;          // Maximum requests in window
    Milliseconds window_size_;   // Window duration
    mutable std::deque<SteadyClock::time_point> requests_;
    mutable Mutex mutex_;
    
    void cleanup() const {
        auto cutoff = SteadyClock::now() - window_size_;
        while (!requests_.empty() && requests_.front() < cutoff) {
            requests_.pop_front();
        }
    }
    
public:
    SlidingWindowLimiter(Size max_requests, Milliseconds window_size)
        : max_requests_(max_requests)
        , window_size_(window_size) {}
    
    RateLimitResult try_acquire() {
        LockGuard<Mutex> lock(mutex_);
        cleanup();
        
        RateLimitResult result;
        auto now = SteadyClock::now();
        
        if (requests_.size() < max_requests_) {
            requests_.push_back(now);
            result.allowed = true;
            result.remaining = max_requests_ - requests_.size();
            result.reset_time = now + window_size_;
        } else {
            result.allowed = false;
            result.remaining = 0;
            // Oldest request will expire at this time
            auto oldest = requests_.front();
            auto expire_time = oldest + window_size_;
            result.retry_after = std::chrono::duration_cast<Milliseconds>(
                expire_time - now);
            if (result.retry_after.count() < 0) {
                result.retry_after = Milliseconds(0);
            }
            result.reset_time = expire_time;
        }
        
        return result;
    }
    
    bool acquire() {
        return try_acquire().allowed;
    }
    
    Size current_count() const {
        LockGuard<Mutex> lock(mutex_);
        cleanup();
        return requests_.size();
    }
    
    Size remaining() const {
        LockGuard<Mutex> lock(mutex_);
        cleanup();
        return max_requests_ > requests_.size() ? 
               max_requests_ - requests_.size() : 0;
    }
    
    Size max_requests() const { return max_requests_; }
    Milliseconds window_size() const { return window_size_; }
    
    void reset() {
        LockGuard<Mutex> lock(mutex_);
        requests_.clear();
    }
};

// =============================================================================
// Fixed Window Rate Limiter
// =============================================================================

class FixedWindowLimiter {
private:
    Size max_requests_;
    Milliseconds window_size_;
    mutable std::atomic<Size> current_count_{0};
    mutable SteadyClock::time_point window_start_;
    mutable Mutex mutex_;
    
    void check_window_reset() const {
        auto now = SteadyClock::now();
        if (now >= window_start_ + window_size_) {
            window_start_ = now;
            current_count_ = 0;
        }
    }
    
public:
    FixedWindowLimiter(Size max_requests, Milliseconds window_size)
        : max_requests_(max_requests)
        , window_size_(window_size)
        , window_start_(SteadyClock::now()) {}
    
    RateLimitResult try_acquire() {
        LockGuard<Mutex> lock(mutex_);
        check_window_reset();
        
        RateLimitResult result;
        
        if (current_count_ < max_requests_) {
            ++current_count_;
            result.allowed = true;
            result.remaining = max_requests_ - current_count_;
            result.reset_time = window_start_ + window_size_;
        } else {
            result.allowed = false;
            result.remaining = 0;
            result.reset_time = window_start_ + window_size_;
            result.retry_after = std::chrono::duration_cast<Milliseconds>(
                result.reset_time - SteadyClock::now());
            if (result.retry_after.count() < 0) {
                result.retry_after = Milliseconds(0);
            }
        }
        
        return result;
    }
    
    bool acquire() {
        return try_acquire().allowed;
    }
    
    Size current_count() const {
        LockGuard<Mutex> lock(mutex_);
        check_window_reset();
        return current_count_;
    }
    
    Size remaining() const {
        LockGuard<Mutex> lock(mutex_);
        check_window_reset();
        return max_requests_ > current_count_ ? 
               max_requests_ - current_count_ : 0;
    }
    
    void reset() {
        LockGuard<Mutex> lock(mutex_);
        current_count_ = 0;
        window_start_ = SteadyClock::now();
    }
};

// =============================================================================
// Composite Rate Limiter
// =============================================================================

class CompositeRateLimiter {
private:
    Vector<TokenBucketLimiter*> token_limiters_;
    Vector<SlidingWindowLimiter*> sliding_limiters_;
    Vector<FixedWindowLimiter*> fixed_limiters_;
    
public:
    void add_token_bucket(TokenBucketLimiter* limiter) {
        token_limiters_.push_back(limiter);
    }
    
    void add_sliding_window(SlidingWindowLimiter* limiter) {
        sliding_limiters_.push_back(limiter);
    }
    
    void add_fixed_window(FixedWindowLimiter* limiter) {
        fixed_limiters_.push_back(limiter);
    }
    
    RateLimitResult try_acquire() {
        RateLimitResult result;
        result.allowed = true;
        result.remaining = SIZE_MAX;
        
        // Check all token bucket limiters
        for (auto* limiter : token_limiters_) {
            auto r = limiter->try_acquire();
            if (!r.allowed) {
                result.allowed = false;
                if (r.retry_after > result.retry_after) {
                    result.retry_after = r.retry_after;
                    result.reset_time = r.reset_time;
                }
            }
            result.remaining = std::min(result.remaining, r.remaining);
        }
        
        // Check all sliding window limiters
        for (auto* limiter : sliding_limiters_) {
            auto r = limiter->try_acquire();
            if (!r.allowed) {
                result.allowed = false;
                if (r.retry_after > result.retry_after) {
                    result.retry_after = r.retry_after;
                    result.reset_time = r.reset_time;
                }
            }
            result.remaining = std::min(result.remaining, r.remaining);
        }
        
        // Check all fixed window limiters
        for (auto* limiter : fixed_limiters_) {
            auto r = limiter->try_acquire();
            if (!r.allowed) {
                result.allowed = false;
                if (r.retry_after > result.retry_after) {
                    result.retry_after = r.retry_after;
                    result.reset_time = r.reset_time;
                }
            }
            result.remaining = std::min(result.remaining, r.remaining);
        }
        
        if (result.remaining == SIZE_MAX) {
            result.remaining = 0;
        }
        
        return result;
    }
    
    bool acquire() {
        return try_acquire().allowed;
    }
    
    void reset_all() {
        for (auto* limiter : token_limiters_) limiter->reset();
        for (auto* limiter : sliding_limiters_) limiter->reset();
        for (auto* limiter : fixed_limiters_) limiter->reset();
    }
};

} // namespace ims

#endif // IMS_COMMON_RATE_LIMITER_HPP
