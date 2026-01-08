// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Resource Pool
// Version: 3.6.3
// =============================================================================
//
// Generic resource pooling with automatic lifecycle management.
// Supports connection pools, object pools, and any reusable resource.
//
// Copyright (c) 2025 CICS Systems - Complete Mainframe Emulation Framework
// =============================================================================

#ifndef IMS_COMMON_RESOURCE_POOL_HPP
#define IMS_COMMON_RESOURCE_POOL_HPP

#include "types.hpp"
#include "error.hpp"
#include <queue>
#include <functional>
#include <chrono>
#include <condition_variable>
#include <optional>
#include <atomic>

namespace ims {

// =============================================================================
// Resource Pool Configuration
// =============================================================================

struct PoolConfig {
    Size min_size{2};                    // Minimum pool size
    Size max_size{10};                   // Maximum pool size
    Milliseconds acquire_timeout{5000};  // Timeout for acquiring resource
    Milliseconds idle_timeout{60000};    // Time before idle resource is removed
    bool validate_on_acquire{true};      // Validate resource before returning
    bool validate_on_return{false};      // Validate resource on return
};

// =============================================================================
// Pool Statistics
// =============================================================================

struct PoolStatistics {
    std::atomic<Size> total_created{0};
    std::atomic<Size> total_destroyed{0};
    std::atomic<Size> current_size{0};
    std::atomic<Size> available{0};
    std::atomic<Size> in_use{0};
    std::atomic<Size> acquire_count{0};
    std::atomic<Size> acquire_timeout_count{0};
    std::atomic<Size> validation_failures{0};
    
    void reset() {
        total_created = 0;
        total_destroyed = 0;
        current_size = 0;
        available = 0;
        in_use = 0;
        acquire_count = 0;
        acquire_timeout_count = 0;
        validation_failures = 0;
    }
};

// =============================================================================
// Pooled Resource Wrapper (RAII)
// =============================================================================

template<typename T>
class ResourcePool;

template<typename T>
class PooledResource {
private:
    T* resource_{nullptr};
    ResourcePool<T>* pool_{nullptr};
    bool released_{false};
    
public:
    PooledResource() = default;
    
    PooledResource(T* resource, ResourcePool<T>* pool)
        : resource_(resource), pool_(pool), released_(false) {}
    
    // Move only
    PooledResource(const PooledResource&) = delete;
    PooledResource& operator=(const PooledResource&) = delete;
    
    PooledResource(PooledResource&& other) noexcept
        : resource_(other.resource_)
        , pool_(other.pool_)
        , released_(other.released_) {
        other.resource_ = nullptr;
        other.pool_ = nullptr;
        other.released_ = true;
    }
    
    PooledResource& operator=(PooledResource&& other) noexcept {
        if (this != &other) {
            release();
            resource_ = other.resource_;
            pool_ = other.pool_;
            released_ = other.released_;
            other.resource_ = nullptr;
            other.pool_ = nullptr;
            other.released_ = true;
        }
        return *this;
    }
    
    ~PooledResource() {
        release();
    }
    
    void release();
    
    T* get() const { return resource_; }
    T& operator*() const { return *resource_; }
    T* operator->() const { return resource_; }
    
    explicit operator bool() const { return resource_ != nullptr && !released_; }
    
    bool valid() const { return resource_ != nullptr && !released_; }
};

// =============================================================================
// Resource Pool
// =============================================================================

template<typename T>
class ResourcePool {
public:
    using Factory = std::function<T*()>;
    using Destroyer = std::function<void(T*)>;
    using Validator = std::function<bool(T*)>;
    using Resetter = std::function<void(T*)>;
    
private:
    struct PoolEntry {
        T* resource{nullptr};
        SteadyClock::time_point last_used{SteadyClock::now()};
        
        PoolEntry() = default;
        explicit PoolEntry(T* res) : resource(res), last_used(SteadyClock::now()) {}
    };
    
    PoolConfig config_;
    Factory factory_;
    Destroyer destroyer_;
    Validator validator_;
    Resetter resetter_;
    
    mutable Mutex mutex_;
    std::condition_variable cv_;
    std::queue<PoolEntry> available_;
    PoolStatistics stats_;
    bool shutdown_{false};
    
    T* create_resource() {
        T* resource = factory_();
        if (resource) {
            ++stats_.total_created;
            ++stats_.current_size;
        }
        return resource;
    }
    
    void destroy_resource(T* resource) {
        if (resource) {
            destroyer_(resource);
            ++stats_.total_destroyed;
            --stats_.current_size;
        }
    }
    
    bool validate_resource(T* resource) {
        if (!validator_) return true;
        return validator_(resource);
    }
    
    void reset_resource(T* resource) {
        if (resetter_) {
            resetter_(resource);
        }
    }
    
public:
    ResourcePool(Factory factory, Destroyer destroyer, 
                 const PoolConfig& config = PoolConfig{})
        : config_(config)
        , factory_(std::move(factory))
        , destroyer_(std::move(destroyer)) {
        
        // Pre-populate pool to minimum size
        for (Size i = 0; i < config_.min_size; ++i) {
            T* resource = create_resource();
            if (resource) {
                available_.push(PoolEntry(resource));
                ++stats_.available;
            }
        }
    }
    
    ~ResourcePool() {
        shutdown();
    }
    
    void set_validator(Validator validator) {
        LockGuard<Mutex> lock(mutex_);
        validator_ = std::move(validator);
    }
    
    void set_resetter(Resetter resetter) {
        LockGuard<Mutex> lock(mutex_);
        resetter_ = std::move(resetter);
    }
    
    ErrorResult<PooledResource<T>> acquire() {
        UniqueLock<Mutex> lock(mutex_);
        
        if (shutdown_) {
            return make_error<PooledResource<T>>("Pool is shutdown");
        }
        
        ++stats_.acquire_count;
        
        // Try to get from available pool
        while (!available_.empty()) {
            auto entry = available_.front();
            available_.pop();
            --stats_.available;
            
            // Validate if configured
            if (config_.validate_on_acquire && !validate_resource(entry.resource)) {
                ++stats_.validation_failures;
                destroy_resource(entry.resource);
                continue;
            }
            
            ++stats_.in_use;
            return PooledResource<T>(entry.resource, this);
        }
        
        // Create new if below max
        if (stats_.current_size < config_.max_size) {
            T* resource = create_resource();
            if (resource) {
                ++stats_.in_use;
                return PooledResource<T>(resource, this);
            }
        }
        
        // Wait for available resource
        auto deadline = SteadyClock::now() + config_.acquire_timeout;
        while (available_.empty() && !shutdown_) {
            if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
                ++stats_.acquire_timeout_count;
                return make_error<PooledResource<T>>("Acquire timeout");
            }
        }
        
        if (shutdown_) {
            return make_error<PooledResource<T>>("Pool is shutdown");
        }
        
        auto entry = available_.front();
        available_.pop();
        --stats_.available;
        ++stats_.in_use;
        
        return PooledResource<T>(entry.resource, this);
    }
    
    void release(T* resource) {
        if (!resource) return;
        
        LockGuard<Mutex> lock(mutex_);
        --stats_.in_use;
        
        if (shutdown_) {
            destroy_resource(resource);
            return;
        }
        
        // Validate on return if configured
        if (config_.validate_on_return && !validate_resource(resource)) {
            ++stats_.validation_failures;
            destroy_resource(resource);
            
            // Create replacement if below minimum
            if (stats_.current_size < config_.min_size) {
                T* replacement = create_resource();
                if (replacement) {
                    available_.push(PoolEntry(replacement));
                    ++stats_.available;
                }
            }
            return;
        }
        
        // Reset and return to pool
        reset_resource(resource);
        available_.push(PoolEntry(resource));
        ++stats_.available;
        cv_.notify_one();
    }
    
    void shutdown() {
        LockGuard<Mutex> lock(mutex_);
        shutdown_ = true;
        
        // Destroy all available resources
        while (!available_.empty()) {
            auto entry = available_.front();
            available_.pop();
            --stats_.available;
            destroy_resource(entry.resource);
        }
        
        cv_.notify_all();
    }
    
    const PoolStatistics& statistics() const { return stats_; }
    
    Size size() const { return stats_.current_size; }
    Size available_count() const { return stats_.available; }
    Size in_use_count() const { return stats_.in_use; }
    bool is_shutdown() const { return shutdown_; }
};

// Implementation of release
template<typename T>
void PooledResource<T>::release() {
    if (resource_ && pool_ && !released_) {
        pool_->release(resource_);
        resource_ = nullptr;
        released_ = true;
    }
}

// =============================================================================
// Connection Pool Specialization
// =============================================================================

template<typename Connection>
class ConnectionPool : public ResourcePool<Connection> {
public:
    using typename ResourcePool<Connection>::Factory;
    using typename ResourcePool<Connection>::Destroyer;
    
    ConnectionPool(Factory factory, Destroyer destroyer,
                   const PoolConfig& config = PoolConfig{})
        : ResourcePool<Connection>(std::move(factory), std::move(destroyer), config) {}
};

} // namespace ims

#endif // IMS_COMMON_RESOURCE_POOL_HPP
