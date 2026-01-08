#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - LRU Cache Implementation
// Version: 3.6.3
// =============================================================================
//
// Thread-safe LRU (Least Recently Used) cache for frequently accessed
// catalog entries and records.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include <list>

namespace ims {

// =============================================================================
// LRU Cache Statistics
// =============================================================================

struct CacheStatistics {
    AtomicUInt64 hits{0};
    AtomicUInt64 misses{0};
    AtomicUInt64 evictions{0};
    AtomicUInt64 insertions{0};
    
    void reset() {
        hits.store(0, std::memory_order_relaxed);
        misses.store(0, std::memory_order_relaxed);
        evictions.store(0, std::memory_order_relaxed);
        insertions.store(0, std::memory_order_relaxed);
    }
    
    double hit_rate() const {
        UInt64 total = hits.load() + misses.load();
        return total > 0 ? static_cast<double>(hits.load()) / static_cast<double>(total) * 100.0 : 0.0;
    }
    
    String to_string() const {
        return std::format("Cache: {} hits, {} misses ({:.2f}% hit rate), {} evictions",
            hits.load(), misses.load(), hit_rate(), evictions.load());
    }
};

// =============================================================================
// LRU Cache Implementation
// =============================================================================

template<typename Key, typename Value>
class LruCache {
private:
    using ListType = std::list<std::pair<Key, Value>>;
    using ListIterator = typename ListType::iterator;
    using MapType = HashMap<Key, ListIterator>;
    
    ListType items_;          // Ordered list (most recent at front)
    MapType lookup_;          // Key -> iterator lookup
    Size capacity_;           // Maximum cache size
    mutable SharedMutex mutex_;
    CacheStatistics stats_;
    
    void evict_one() {
        if (items_.empty()) return;
        
        // Remove least recently used (back of list)
        auto& last = items_.back();
        lookup_.erase(last.first);
        items_.pop_back();
        stats_.evictions.fetch_add(1, std::memory_order_relaxed);
    }
    
public:
    explicit LruCache(Size capacity = 1000) : capacity_(capacity) {
        lookup_.reserve(capacity);
    }
    
    // Get value (returns empty optional if not found)
    Optional<Value> get(const Key& key) {
        SharedLock<SharedMutex> lock(mutex_);
        
        auto it = lookup_.find(key);
        if (it == lookup_.end()) {
            stats_.misses.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
        
        stats_.hits.fetch_add(1, std::memory_order_relaxed);
        
        // Move to front (most recently used) - need unique lock
        lock.unlock();
        UniqueLock<SharedMutex> write_lock(mutex_);
        
        // Re-check after acquiring write lock
        it = lookup_.find(key);
        if (it == lookup_.end()) {
            return std::nullopt;
        }
        
        auto list_it = it->second;
        if (list_it != items_.begin()) {
            items_.splice(items_.begin(), items_, list_it);
        }
        
        return list_it->second;
    }
    
    // Put value (returns true if key was new, false if updated)
    bool put(const Key& key, Value value) {
        UniqueLock<SharedMutex> lock(mutex_);
        
        auto it = lookup_.find(key);
        if (it != lookup_.end()) {
            // Update existing entry and move to front
            it->second->second = std::move(value);
            items_.splice(items_.begin(), items_, it->second);
            return false;
        }
        
        // Add new entry
        if (items_.size() >= capacity_) {
            evict_one();
        }
        
        items_.emplace_front(key, std::move(value));
        lookup_[key] = items_.begin();
        stats_.insertions.fetch_add(1, std::memory_order_relaxed);
        
        return true;
    }
    
    // Remove entry
    bool remove(const Key& key) {
        UniqueLock<SharedMutex> lock(mutex_);
        
        auto it = lookup_.find(key);
        if (it == lookup_.end()) {
            return false;
        }
        
        items_.erase(it->second);
        lookup_.erase(it);
        return true;
    }
    
    // Check if key exists
    bool contains(const Key& key) const {
        SharedLock<SharedMutex> lock(mutex_);
        return lookup_.find(key) != lookup_.end();
    }
    
    // Clear all entries
    void clear() {
        UniqueLock<SharedMutex> lock(mutex_);
        items_.clear();
        lookup_.clear();
    }
    
    // Get current size
    Size size() const {
        SharedLock<SharedMutex> lock(mutex_);
        return items_.size();
    }
    
    // Get capacity
    Size capacity() const { return capacity_; }
    
    // Resize cache (may trigger evictions)
    void resize(Size new_capacity) {
        UniqueLock<SharedMutex> lock(mutex_);
        capacity_ = new_capacity;
        while (items_.size() > capacity_) {
            evict_one();
        }
    }
    
    // Get statistics
    const CacheStatistics& statistics() const { return stats_; }
    
    // Reset statistics
    void reset_statistics() { stats_.reset(); }
    
    // Get all keys (for diagnostics)
    Vector<Key> keys() const {
        SharedLock<SharedMutex> lock(mutex_);
        Vector<Key> result;
        result.reserve(items_.size());
        for (const auto& item : items_) {
            result.push_back(item.first);
        }
        return result;
    }
    
    // Iteration support (snapshot)
    Vector<std::pair<Key, Value>> snapshot() const {
        SharedLock<SharedMutex> lock(mutex_);
        return Vector<std::pair<Key, Value>>(items_.begin(), items_.end());
    }
};

// =============================================================================
// Time-based Cache (with expiration)
// =============================================================================

template<typename Key, typename Value>
class TimedCache {
private:
    struct CacheEntry {
        Value value;
        SystemTimePoint expires_at;
    };
    
    HashMap<Key, CacheEntry> entries_;
    Duration default_ttl_;
    mutable SharedMutex mutex_;
    CacheStatistics stats_;
    
    void remove_expired() {
        auto now = SystemClock::now();
        for (auto it = entries_.begin(); it != entries_.end(); ) {
            if (it->second.expires_at <= now) {
                it = entries_.erase(it);
                stats_.evictions.fetch_add(1, std::memory_order_relaxed);
            } else {
                ++it;
            }
        }
    }
    
public:
    explicit TimedCache(Duration default_ttl = Seconds(300))  // 5 minutes default
        : default_ttl_(default_ttl) {}
    
    Optional<Value> get(const Key& key) {
        SharedLock<SharedMutex> lock(mutex_);
        
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            stats_.misses.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
        
        if (it->second.expires_at <= SystemClock::now()) {
            // Entry expired - need write lock to remove
            lock.unlock();
            UniqueLock<SharedMutex> write_lock(mutex_);
            entries_.erase(key);
            stats_.misses.fetch_add(1, std::memory_order_relaxed);
            stats_.evictions.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
        
        stats_.hits.fetch_add(1, std::memory_order_relaxed);
        return it->second.value;
    }
    
    void put(const Key& key, Value value, Optional<Duration> ttl = std::nullopt) {
        UniqueLock<SharedMutex> lock(mutex_);
        
        Duration effective_ttl = ttl.value_or(default_ttl_);
        entries_[key] = CacheEntry{std::move(value), SystemClock::now() + effective_ttl};
        stats_.insertions.fetch_add(1, std::memory_order_relaxed);
    }
    
    bool remove(const Key& key) {
        UniqueLock<SharedMutex> lock(mutex_);
        return entries_.erase(key) > 0;
    }
    
    void clear() {
        UniqueLock<SharedMutex> lock(mutex_);
        entries_.clear();
    }
    
    void cleanup() {
        UniqueLock<SharedMutex> lock(mutex_);
        remove_expired();
    }
    
    Size size() const {
        SharedLock<SharedMutex> lock(mutex_);
        return entries_.size();
    }
    
    const CacheStatistics& statistics() const { return stats_; }
    void reset_statistics() { stats_.reset(); }
};

// =============================================================================
// Global Caches
// =============================================================================

template<typename Key, typename Value>
inline LruCache<Key, Value>& get_global_cache() {
    static LruCache<Key, Value> cache(10000);
    return cache;
}

} // namespace ims
