#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Buffer Pool
// Version: 3.6.3
// NEW in v3.6.3: Memory-efficient buffer management
// =============================================================================

#include "types.hpp"
#include <list>

namespace ims {

// =============================================================================
// Pooled Buffer (RAII wrapper)
// =============================================================================

class BufferPool;

class PooledBuffer {
    friend class BufferPool;
    
private:
    ByteBuffer buffer_;
    BufferPool* pool_{nullptr};
    Size original_size_{0};
    
    PooledBuffer(Size size, BufferPool* pool) 
        : buffer_(size), pool_(pool), original_size_(size) {}
    
public:
    PooledBuffer() = default;
    ~PooledBuffer();
    
    // Move only
    PooledBuffer(PooledBuffer&& other) noexcept
        : buffer_(std::move(other.buffer_))
        , pool_(other.pool_)
        , original_size_(other.original_size_) {
        other.pool_ = nullptr;
        other.original_size_ = 0;
    }
    
    PooledBuffer& operator=(PooledBuffer&& other) noexcept {
        if (this != &other) {
            release();
            buffer_ = std::move(other.buffer_);
            pool_ = other.pool_;
            original_size_ = other.original_size_;
            other.pool_ = nullptr;
            other.original_size_ = 0;
        }
        return *this;
    }
    
    PooledBuffer(const PooledBuffer&) = delete;
    PooledBuffer& operator=(const PooledBuffer&) = delete;
    
    // Access
    Byte* data() { return buffer_.data(); }
    const Byte* data() const { return buffer_.data(); }
    Size size() const { return buffer_.size(); }
    Size capacity() const { return original_size_; }
    bool empty() const { return buffer_.empty(); }
    
    Byte& operator[](Size idx) { return buffer_[idx]; }
    const Byte& operator[](Size idx) const { return buffer_[idx]; }
    
    // Resize (within original capacity)
    void resize(Size new_size) {
        if (new_size <= original_size_) {
            buffer_.resize(new_size);
        }
    }
    
    // Clear contents
    void clear() {
        std::fill(buffer_.begin(), buffer_.end(), 0);
    }
    
    // Release back to pool early
    void release();
    
    // Conversion
    ByteBuffer& as_vector() { return buffer_; }
    const ByteBuffer& as_vector() const { return buffer_; }
    
    Span<Byte> as_span() { return Span<Byte>(buffer_); }
    Span<const Byte> as_span() const { return Span<const Byte>(buffer_.data(), buffer_.size()); }
};

// =============================================================================
// Buffer Pool Implementation
// =============================================================================

class BufferPool {
public:
    struct PoolStats {
        Size total_buffers{0};
        Size available_buffers{0};
        Size allocated_buffers{0};
        Size total_allocations{0};
        Size pool_hits{0};
        Size pool_misses{0};
        Size total_memory{0};
        
        double hit_rate() const {
            return total_allocations > 0 
                ? static_cast<double>(pool_hits) / total_allocations * 100.0 
                : 0.0;
        }
    };
    
private:
    struct PoolBucket {
        Size buffer_size;
        std::list<ByteBuffer> free_buffers;
        Size max_buffers;
        Size total_allocated{0};
    };
    
    Vector<PoolBucket> buckets_;
    mutable Mutex mutex_;
    PoolStats stats_;
    
    // Standard bucket sizes (powers of 2)
    static constexpr Size BUCKET_SIZES[] = {
        256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144
    };
    static constexpr Size NUM_BUCKETS = sizeof(BUCKET_SIZES) / sizeof(BUCKET_SIZES[0]);
    
    Size find_bucket_index(Size size) const {
        for (Size i = 0; i < NUM_BUCKETS; ++i) {
            if (size <= BUCKET_SIZES[i]) {
                return i;
            }
        }
        return NUM_BUCKETS;  // Oversized
    }
    
public:
    explicit BufferPool(Size max_buffers_per_bucket = 64) {
        buckets_.resize(NUM_BUCKETS);
        for (Size i = 0; i < NUM_BUCKETS; ++i) {
            buckets_[i].buffer_size = BUCKET_SIZES[i];
            buckets_[i].max_buffers = max_buffers_per_bucket;
        }
    }
    
    ~BufferPool() = default;
    
    // Non-copyable
    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;
    
    // Acquire a buffer
    PooledBuffer acquire(Size size) {
        std::unique_lock lock(mutex_);
        ++stats_.total_allocations;
        
        Size bucket_idx = find_bucket_index(size);
        
        if (bucket_idx < NUM_BUCKETS) {
            auto& bucket = buckets_[bucket_idx];
            
            if (!bucket.free_buffers.empty()) {
                // Pool hit
                ++stats_.pool_hits;
                ++stats_.allocated_buffers;
                --stats_.available_buffers;
                
                ByteBuffer buf = std::move(bucket.free_buffers.front());
                bucket.free_buffers.pop_front();
                
                PooledBuffer result(0, this);
                result.buffer_ = std::move(buf);
                result.original_size_ = bucket.buffer_size;
                result.buffer_.resize(size);
                return result;
            }
            
            // Pool miss - create new buffer
            ++stats_.pool_misses;
            ++bucket.total_allocated;
            ++stats_.total_buffers;
            ++stats_.allocated_buffers;
            stats_.total_memory += bucket.buffer_size;
            
            PooledBuffer result(bucket.buffer_size, this);
            result.buffer_.resize(size);
            return result;
        }
        
        // Oversized buffer - no pooling
        ++stats_.pool_misses;
        ++stats_.total_buffers;
        ++stats_.allocated_buffers;
        stats_.total_memory += size;
        
        return PooledBuffer(size, nullptr);  // Not pooled
    }
    
    // Return a buffer to the pool
    void release(ByteBuffer&& buffer, Size original_size) {
        std::unique_lock lock(mutex_);
        --stats_.allocated_buffers;
        
        Size bucket_idx = find_bucket_index(original_size);
        
        if (bucket_idx < NUM_BUCKETS) {
            auto& bucket = buckets_[bucket_idx];
            
            if (bucket.free_buffers.size() < bucket.max_buffers) {
                buffer.resize(bucket.buffer_size);
                buffer.shrink_to_fit();
                bucket.free_buffers.push_back(std::move(buffer));
                ++stats_.available_buffers;
                return;
            }
        }
        
        // Discard if pool is full or oversized
        stats_.total_memory -= original_size;
        --stats_.total_buffers;
    }
    
    // Pre-allocate buffers
    void warm(Size bucket_index, Size count) {
        if (bucket_index >= NUM_BUCKETS) return;
        
        std::unique_lock lock(mutex_);
        auto& bucket = buckets_[bucket_index];
        
        for (Size i = 0; i < count && bucket.free_buffers.size() < bucket.max_buffers; ++i) {
            bucket.free_buffers.emplace_back(bucket.buffer_size);
            ++bucket.total_allocated;
            ++stats_.total_buffers;
            ++stats_.available_buffers;
            stats_.total_memory += bucket.buffer_size;
        }
    }
    
    // Warm common sizes
    void warm_common(Size count_per_bucket = 8) {
        for (Size i = 0; i < NUM_BUCKETS; ++i) {
            warm(i, count_per_bucket);
        }
    }
    
    // Clear all pooled buffers
    void clear() {
        std::unique_lock lock(mutex_);
        for (auto& bucket : buckets_) {
            stats_.total_memory -= bucket.buffer_size * bucket.free_buffers.size();
            stats_.total_buffers -= bucket.free_buffers.size();
            stats_.available_buffers -= bucket.free_buffers.size();
            bucket.free_buffers.clear();
            bucket.total_allocated = 0;
        }
    }
    
    // Get statistics
    PoolStats statistics() const {
        std::unique_lock lock(mutex_);
        return stats_;
    }
    
    // Bucket information
    Size bucket_count() const { return NUM_BUCKETS; }
    
    Size bucket_size(Size index) const {
        return index < NUM_BUCKETS ? BUCKET_SIZES[index] : 0;
    }
    
    Size bucket_available(Size index) const {
        if (index >= NUM_BUCKETS) return 0;
        std::unique_lock lock(mutex_);
        return buckets_[index].free_buffers.size();
    }
};

// PooledBuffer destructor implementation
inline PooledBuffer::~PooledBuffer() {
    release();
}

inline void PooledBuffer::release() {
    if (pool_ && original_size_ > 0) {
        pool_->release(std::move(buffer_), original_size_);
        pool_ = nullptr;
        original_size_ = 0;
    }
}

// =============================================================================
// Global Buffer Pool
// =============================================================================

inline BufferPool& global_buffer_pool() {
    static BufferPool instance;
    return instance;
}

} // namespace ims
