/**
 * @file object_pool.hpp
 * @brief High-performance object pool for frequent allocations
 * @version 3.6.3
 *
 * Provides object pooling including:
 * - Pre-allocated object pools
 * - Thread-safe acquisition and release
 * - Automatic pool growth
 * - Object lifecycle management
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_OBJECT_POOL_HPP
#define IMS_COMMON_OBJECT_POOL_HPP

#include "types.hpp"
#include <stack>

namespace ims::common {

// =============================================================================
// Object Pool
// =============================================================================

/**
 * @brief High-performance object pool for frequent allocations
 * 
 * Reduces allocation overhead by reusing objects. Thread-safe.
 */
template<typename T>
class ObjectPool {
public:
    using Factory = Function<UniquePtr<T>()>;
    using Resetter = Function<void(T&)>;
    
private:
    struct PooledObject {
        UniquePtr<T> object;
        TimePoint created_at;
        Size use_count{0};
    };
    
    std::stack<PooledObject> pool_;
    Factory factory_;
    Resetter resetter_;
    Size initial_size_;
    Size max_size_;
    Size total_created_{0};
    Size total_acquired_{0};
    Size total_released_{0};
    mutable Mutex mutex_;
    
    void create_object() {
        PooledObject obj;
        obj.object = factory_ ? factory_() : std::make_unique<T>();
        obj.created_at = Clock::now();
        obj.use_count = 0;
        pool_.push(std::move(obj));
        ++total_created_;
    }
    
public:
    /**
     * @brief Constructs an object pool
     * @param initial_size Initial number of objects to pre-allocate
     * @param max_size Maximum pool size (0 = unlimited)
     * @param factory Optional factory function for creating objects
     */
    explicit ObjectPool(Size initial_size = 10, Size max_size = 100,
                       Factory factory = nullptr)
        : factory_(std::move(factory))
        , initial_size_(initial_size)
        , max_size_(max_size) {
        
        for (Size i = 0; i < initial_size_; ++i) {
            create_object();
        }
    }
    
    /**
     * @brief Sets a reset function called when objects are returned
     */
    void set_resetter(Resetter resetter) {
        LockGuard<Mutex> lock(mutex_);
        resetter_ = std::move(resetter);
    }
    
    /**
     * @brief Acquires an object from the pool
     * @return Pointer to object, or nullptr if pool exhausted and at max size
     */
    T* acquire() {
        LockGuard<Mutex> lock(mutex_);
        
        if (pool_.empty()) {
            if (max_size_ == 0 || total_created_ < max_size_) {
                create_object();
            } else {
                return nullptr;  // Pool exhausted
            }
        }
        
        auto obj = std::move(pool_.top());
        pool_.pop();
        ++obj.use_count;
        ++total_acquired_;
        
        return obj.object.release();
    }
    
    /**
     * @brief Releases an object back to the pool
     */
    void release(T* obj) {
        if (!obj) return;
        
        LockGuard<Mutex> lock(mutex_);
        
        if (resetter_) {
            resetter_(*obj);
        }
        
        PooledObject pooled;
        pooled.object.reset(obj);
        pooled.created_at = Clock::now();
        pool_.push(std::move(pooled));
        ++total_released_;
    }
    
    /**
     * @brief RAII wrapper for automatic release
     */
    class ScopedObject {
    private:
        ObjectPool* pool_;
        T* obj_;
        
    public:
        ScopedObject(ObjectPool* pool, T* obj) : pool_(pool), obj_(obj) {}
        ~ScopedObject() { if (pool_ && obj_) pool_->release(obj_); }
        
        ScopedObject(const ScopedObject&) = delete;
        ScopedObject& operator=(const ScopedObject&) = delete;
        
        ScopedObject(ScopedObject&& other) noexcept
            : pool_(other.pool_), obj_(other.obj_) {
            other.pool_ = nullptr;
            other.obj_ = nullptr;
        }
        
        T* get() const { return obj_; }
        T* operator->() const { return obj_; }
        T& operator*() const { return *obj_; }
        explicit operator bool() const { return obj_ != nullptr; }
        
        T* release() {
            T* tmp = obj_;
            obj_ = nullptr;
            pool_ = nullptr;
            return tmp;
        }
    };
    
    /**
     * @brief Acquires an object with automatic release
     */
    ScopedObject acquire_scoped() {
        return ScopedObject(this, acquire());
    }
    
    /**
     * @brief Gets current pool size (available objects)
     */
    Size available() const {
        LockGuard<Mutex> lock(mutex_);
        return pool_.size();
    }
    
    /**
     * @brief Gets total objects created
     */
    Size total_created() const {
        LockGuard<Mutex> lock(mutex_);
        return total_created_;
    }
    
    /**
     * @brief Gets statistics
     */
    struct Stats {
        Size available;
        Size total_created;
        Size total_acquired;
        Size total_released;
        Size in_use;
        double hit_rate;
    };
    
    Stats stats() const {
        LockGuard<Mutex> lock(mutex_);
        Stats s;
        s.available = pool_.size();
        s.total_created = total_created_;
        s.total_acquired = total_acquired_;
        s.total_released = total_released_;
        s.in_use = total_acquired_ - total_released_;
        s.hit_rate = total_acquired_ > 0 
            ? static_cast<double>(total_acquired_ - total_created_) / total_acquired_ * 100.0
            : 0.0;
        return s;
    }
    
    /**
     * @brief Clears all pooled objects
     */
    void clear() {
        LockGuard<Mutex> lock(mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }
    
    /**
     * @brief Pre-allocates objects up to specified count
     */
    void reserve(Size count) {
        LockGuard<Mutex> lock(mutex_);
        while (pool_.size() < count && (max_size_ == 0 || total_created_ < max_size_)) {
            create_object();
        }
    }
};

// =============================================================================
// Typed Object Pool with Reset
// =============================================================================

/**
 * @brief Convenience pool for objects with clear() method
 */
template<typename T>
class ClearableObjectPool : public ObjectPool<T> {
public:
    explicit ClearableObjectPool(Size initial_size = 10, Size max_size = 100)
        : ObjectPool<T>(initial_size, max_size) {
        this->set_resetter([](T& obj) {
            if constexpr (requires { obj.clear(); }) {
                obj.clear();
            }
        });
    }
};

/**
 * @brief Pool for string buffers
 */
using StringPool = ClearableObjectPool<String>;

/**
 * @brief Pool for byte buffers
 */
using ByteBufferPool = ClearableObjectPool<ByteBuffer>;

} // namespace ims::common

#endif // IMS_COMMON_OBJECT_POOL_HPP
