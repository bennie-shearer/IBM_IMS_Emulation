/**
 * @file memory_pool.hpp
 * @brief High-performance memory pool allocator for fixed-size objects
 * @version 3.6.2
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_MEMORY_POOL_HPP
#define IMS_COMMON_MEMORY_POOL_HPP

#include "types.hpp"
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <new>
#include <vector>

namespace ims::common {

/**
 * @brief Memory pool statistics
 */
struct PoolStats {
    Size capacity;          // Total slots
    Size allocated;         // Currently allocated
    Size peak_allocated;    // Maximum ever allocated
    Size allocation_count;  // Total allocations
    Size deallocation_count;// Total deallocations
};

/**
 * @brief Fixed-size memory pool for fast allocation/deallocation
 * 
 * Provides O(1) allocation and deallocation for objects of a fixed size.
 * Memory is pre-allocated in chunks for cache efficiency.
 */
template<typename T>
class MemoryPool {
public:
    /**
     * @brief Create a memory pool with initial capacity
     * @param initial_capacity Number of objects to pre-allocate
     * @param grow_size Number of objects to add when growing (0 = no growth)
     */
    explicit MemoryPool(Size initial_capacity = 64, Size grow_size = 64)
        : grow_size_(grow_size)
        , allocated_(0)
        , peak_allocated_(0)
        , allocation_count_(0)
        , deallocation_count_(0) {
        
        if (initial_capacity > 0) {
            allocate_chunk(initial_capacity);
        }
    }
    
    ~MemoryPool() {
        // Free all chunks
        for (void* chunk : chunks_) {
            std::free(chunk);
        }
    }
    
    // Non-copyable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    
    // Movable
    MemoryPool(MemoryPool&& other) noexcept
        : chunks_(std::move(other.chunks_))
        , free_list_(other.free_list_)
        , grow_size_(other.grow_size_)
        , allocated_(other.allocated_)
        , peak_allocated_(other.peak_allocated_)
        , allocation_count_(other.allocation_count_)
        , deallocation_count_(other.deallocation_count_) {
        other.free_list_ = nullptr;
        other.allocated_ = 0;
    }
    
    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            // Free existing chunks
            for (void* chunk : chunks_) {
                std::free(chunk);
            }
            
            chunks_ = std::move(other.chunks_);
            free_list_ = other.free_list_;
            grow_size_ = other.grow_size_;
            allocated_ = other.allocated_;
            peak_allocated_ = other.peak_allocated_;
            allocation_count_ = other.allocation_count_;
            deallocation_count_ = other.deallocation_count_;
            
            other.free_list_ = nullptr;
            other.allocated_ = 0;
        }
        return *this;
    }
    
    /**
     * @brief Allocate memory for one object
     * @return Pointer to uninitialized memory
     */
    T* allocate() {
        if (free_list_ == nullptr) {
            if (grow_size_ == 0) {
                throw std::bad_alloc();
            }
            allocate_chunk(grow_size_);
        }
        
        // Pop from free list
        Node* node = free_list_;
        free_list_ = node->next;
        
        ++allocated_;
        ++allocation_count_;
        if (allocated_ > peak_allocated_) {
            peak_allocated_ = allocated_;
        }
        
        return reinterpret_cast<T*>(node);
    }
    
    /**
     * @brief Deallocate memory for one object
     * @param ptr Pointer previously returned by allocate()
     */
    void deallocate(T* ptr) {
        if (ptr == nullptr) return;
        
        // Push onto free list
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = free_list_;
        free_list_ = node;
        
        --allocated_;
        ++deallocation_count_;
    }
    
    /**
     * @brief Construct an object in the pool
     */
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        try {
            new (ptr) T(std::forward<Args>(args)...);
            return ptr;
        } catch (...) {
            deallocate(ptr);
            throw;
        }
    }
    
    /**
     * @brief Destroy and deallocate an object
     */
    void destroy(T* ptr) {
        if (ptr != nullptr) {
            ptr->~T();
            deallocate(ptr);
        }
    }
    
    /**
     * @brief Get current statistics
     */
    PoolStats stats() const {
        PoolStats s;
        s.capacity = capacity();
        s.allocated = allocated_;
        s.peak_allocated = peak_allocated_;
        s.allocation_count = allocation_count_;
        s.deallocation_count = deallocation_count_;
        return s;
    }
    
    /**
     * @brief Get total capacity
     */
    Size capacity() const {
        Size total = 0;
        for (Size i = 0; i < chunk_sizes_.size(); ++i) {
            total += chunk_sizes_[i];
        }
        return total;
    }
    
    /**
     * @brief Get number of currently allocated objects
     */
    Size allocated() const { return allocated_; }
    
    /**
     * @brief Get number of available slots
     */
    Size available() const { return capacity() - allocated_; }
    
    /**
     * @brief Check if pool is empty (no allocations)
     */
    bool empty() const { return allocated_ == 0; }

private:
    // Free list node (embedded in unused slots)
    struct Node {
        Node* next;
    };
    
    static_assert(sizeof(T) >= sizeof(Node), 
                  "Type must be at least as large as a pointer");
    
    std::vector<void*> chunks_;
    std::vector<Size> chunk_sizes_;
    Node* free_list_ = nullptr;
    Size grow_size_;
    Size allocated_;
    Size peak_allocated_;
    Size allocation_count_;
    Size deallocation_count_;
    
    void allocate_chunk(Size count) {
        // Allocate aligned memory
        Size size = count * sizeof(T);
        void* chunk = std::aligned_alloc(alignof(T), size);
        if (chunk == nullptr) {
            throw std::bad_alloc();
        }
        
        chunks_.push_back(chunk);
        chunk_sizes_.push_back(count);
        
        // Initialize free list
        char* ptr = static_cast<char*>(chunk);
        for (Size i = 0; i < count; ++i) {
            Node* node = reinterpret_cast<Node*>(ptr + i * sizeof(T));
            node->next = free_list_;
            free_list_ = node;
        }
    }
};

/**
 * @brief Thread-safe memory pool
 */
template<typename T>
class ThreadSafeMemoryPool {
public:
    explicit ThreadSafeMemoryPool(Size initial_capacity = 64, Size grow_size = 64)
        : pool_(initial_capacity, grow_size) {}
    
    T* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.allocate();
    }
    
    void deallocate(T* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.deallocate(ptr);
    }
    
    template<typename... Args>
    T* construct(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.construct(std::forward<Args>(args)...);
    }
    
    void destroy(T* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.destroy(ptr);
    }
    
    PoolStats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.stats();
    }
    
    Size capacity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.capacity();
    }
    
    Size allocated() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.allocated();
    }

private:
    mutable std::mutex mutex_;
    MemoryPool<T> pool_;
};

/**
 * @brief STL-compatible allocator using a memory pool
 */
template<typename T, typename Pool = MemoryPool<T>>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = PoolAllocator<U, MemoryPool<U>>;
    };
    
    explicit PoolAllocator(Pool& pool) : pool_(&pool) {}
    
    template<typename U, typename P>
    PoolAllocator(const PoolAllocator<U, P>& other) : pool_(nullptr) {
        // Note: This loses the pool reference for different types
    }
    
    T* allocate(size_type n) {
        if (n != 1) {
            // Pool only supports single-object allocation
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        if (pool_) {
            return pool_->allocate();
        }
        return static_cast<T*>(::operator new(sizeof(T)));
    }
    
    void deallocate(T* ptr, size_type n) {
        if (n != 1 || pool_ == nullptr) {
            ::operator delete(ptr);
            return;
        }
        pool_->deallocate(ptr);
    }
    
    template<typename U, typename... Args>
    void construct(U* ptr, Args&&... args) {
        new (ptr) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* ptr) {
        ptr->~U();
    }
    
    bool operator==(const PoolAllocator& other) const {
        return pool_ == other.pool_;
    }
    
    bool operator!=(const PoolAllocator& other) const {
        return pool_ != other.pool_;
    }

private:
    Pool* pool_;
};

/**
 * @brief Smart pointer that returns object to pool on destruction
 */
template<typename T>
class PoolPtr {
public:
    PoolPtr() : ptr_(nullptr), pool_(nullptr) {}
    
    PoolPtr(T* ptr, MemoryPool<T>* pool) : ptr_(ptr), pool_(pool) {}
    
    ~PoolPtr() {
        reset();
    }
    
    // Non-copyable
    PoolPtr(const PoolPtr&) = delete;
    PoolPtr& operator=(const PoolPtr&) = delete;
    
    // Movable
    PoolPtr(PoolPtr&& other) noexcept 
        : ptr_(other.ptr_), pool_(other.pool_) {
        other.ptr_ = nullptr;
        other.pool_ = nullptr;
    }
    
    PoolPtr& operator=(PoolPtr&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            pool_ = other.pool_;
            other.ptr_ = nullptr;
            other.pool_ = nullptr;
        }
        return *this;
    }
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    
    explicit operator bool() const { return ptr_ != nullptr; }
    
    T* release() {
        T* p = ptr_;
        ptr_ = nullptr;
        pool_ = nullptr;
        return p;
    }
    
    void reset() {
        if (ptr_ && pool_) {
            pool_->destroy(ptr_);
        }
        ptr_ = nullptr;
        pool_ = nullptr;
    }

private:
    T* ptr_;
    MemoryPool<T>* pool_;
};

/**
 * @brief Create a PoolPtr from a pool
 */
template<typename T, typename... Args>
PoolPtr<T> make_pooled(MemoryPool<T>& pool, Args&&... args) {
    return PoolPtr<T>(pool.construct(std::forward<Args>(args)...), &pool);
}

/**
 * @brief Object pool with automatic return
 * 
 * Objects are returned to pool when the handle goes out of scope.
 */
template<typename T>
class ObjectPool {
public:
    using Handle = PoolPtr<T>;
    
    explicit ObjectPool(Size initial_capacity = 64)
        : pool_(initial_capacity, initial_capacity) {}
    
    template<typename... Args>
    Handle acquire(Args&&... args) {
        return make_pooled(pool_, std::forward<Args>(args)...);
    }
    
    PoolStats stats() const { return pool_.stats(); }

private:
    MemoryPool<T> pool_;
};

}  // namespace ims::common

#endif  // IMS_COMMON_MEMORY_POOL_HPP
