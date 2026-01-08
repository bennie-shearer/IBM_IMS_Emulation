/**
 * @file async_queue.hpp
 * @brief Thread-safe asynchronous message queue
 * @version 3.6.2
 *
 * Provides async messaging including:
 * - Thread-safe bounded and unbounded queues
 * - Blocking and non-blocking operations
 * - Timeout support
 * - Priority queuing
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_ASYNC_QUEUE_HPP
#define IMS_COMMON_ASYNC_QUEUE_HPP

#include "types.hpp"
#include <queue>
#include <deque>

namespace ims::common {

// =============================================================================
// Async Queue
// =============================================================================

/**
 * @brief Thread-safe unbounded queue
 */
template<typename T>
class AsyncQueue {
private:
    std::deque<T> queue_;
    mutable Mutex mutex_;
    ConditionVar not_empty_;
    AtomicBool closed_{false};
    
public:
    AsyncQueue() = default;
    
    /**
     * @brief Pushes an item to the queue
     * @return true if successful, false if queue is closed
     */
    bool push(T item) {
        if (closed_) return false;
        
        {
            LockGuard<Mutex> lock(mutex_);
            queue_.push_back(std::move(item));
        }
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Pushes an item to the front of the queue (priority)
     */
    bool push_front(T item) {
        if (closed_) return false;
        
        {
            LockGuard<Mutex> lock(mutex_);
            queue_.push_front(std::move(item));
        }
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Pops an item, blocking if empty
     * @return Optional containing item, or empty if queue closed
     */
    Optional<T> pop() {
        UniqueLock<Mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() {
            return !queue_.empty() || closed_;
        });
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop_front();
        return item;
    }
    
    /**
     * @brief Pops an item with timeout
     * @param timeout Maximum time to wait
     * @return Optional containing item, or empty if timeout/closed
     */
    Optional<T> pop(Milliseconds timeout) {
        UniqueLock<Mutex> lock(mutex_);
        bool success = not_empty_.wait_for(lock, timeout, [this]() {
            return !queue_.empty() || closed_;
        });
        
        if (!success || queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop_front();
        return item;
    }
    
    /**
     * @brief Tries to pop an item without blocking
     * @return Optional containing item, or empty if queue empty
     */
    Optional<T> try_pop() {
        LockGuard<Mutex> lock(mutex_);
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop_front();
        return item;
    }
    
    /**
     * @brief Peeks at front item without removing
     */
    Optional<T> peek() const {
        LockGuard<Mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        return queue_.front();
    }
    
    /**
     * @brief Closes the queue, waking all waiters
     */
    void close() {
        closed_ = true;
        not_empty_.notify_all();
    }
    
    /**
     * @brief Reopens a closed queue
     */
    void reopen() {
        closed_ = false;
    }
    
    /**
     * @brief Checks if queue is closed
     */
    bool is_closed() const { return closed_; }
    
    /**
     * @brief Checks if queue is empty
     */
    bool empty() const {
        LockGuard<Mutex> lock(mutex_);
        return queue_.empty();
    }
    
    /**
     * @brief Gets queue size
     */
    Size size() const {
        LockGuard<Mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
     * @brief Clears all items
     */
    void clear() {
        LockGuard<Mutex> lock(mutex_);
        queue_.clear();
    }
    
    /**
     * @brief Drains all items to a vector
     */
    Vector<T> drain() {
        LockGuard<Mutex> lock(mutex_);
        Vector<T> items;
        items.reserve(queue_.size());
        while (!queue_.empty()) {
            items.push_back(std::move(queue_.front()));
            queue_.pop_front();
        }
        return items;
    }
};

// =============================================================================
// Bounded Async Queue
// =============================================================================

/**
 * @brief Thread-safe bounded queue with back-pressure
 */
template<typename T>
class BoundedAsyncQueue {
private:
    std::deque<T> queue_;
    Size max_size_;
    mutable Mutex mutex_;
    ConditionVar not_empty_;
    ConditionVar not_full_;
    AtomicBool closed_{false};
    std::atomic<Size> dropped_{0};
    
public:
    /**
     * @brief Constructs a bounded queue
     * @param max_size Maximum queue size
     */
    explicit BoundedAsyncQueue(Size max_size = 1000)
        : max_size_(max_size) {}
    
    /**
     * @brief Pushes an item, blocking if full
     * @return true if successful, false if closed
     */
    bool push(T item) {
        if (closed_) return false;
        
        UniqueLock<Mutex> lock(mutex_);
        not_full_.wait(lock, [this]() {
            return queue_.size() < max_size_ || closed_;
        });
        
        if (closed_) return false;
        
        queue_.push_back(std::move(item));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Pushes an item with timeout
     * @return true if successful, false if timeout/closed
     */
    bool push(T item, Milliseconds timeout) {
        if (closed_) return false;
        
        UniqueLock<Mutex> lock(mutex_);
        bool success = not_full_.wait_for(lock, timeout, [this]() {
            return queue_.size() < max_size_ || closed_;
        });
        
        if (!success || closed_) return false;
        
        queue_.push_back(std::move(item));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Tries to push without blocking, drops if full
     * @return true if pushed, false if dropped
     */
    bool try_push(T item) {
        if (closed_) return false;
        
        LockGuard<Mutex> lock(mutex_);
        if (queue_.size() >= max_size_) {
            ++dropped_;
            return false;
        }
        
        queue_.push_back(std::move(item));
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Pops an item, blocking if empty
     */
    Optional<T> pop() {
        UniqueLock<Mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() {
            return !queue_.empty() || closed_;
        });
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop_front();
        lock.unlock();
        not_full_.notify_one();
        return item;
    }
    
    /**
     * @brief Pops an item with timeout
     */
    Optional<T> pop(Milliseconds timeout) {
        UniqueLock<Mutex> lock(mutex_);
        bool success = not_empty_.wait_for(lock, timeout, [this]() {
            return !queue_.empty() || closed_;
        });
        
        if (!success || queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop_front();
        lock.unlock();
        not_full_.notify_one();
        return item;
    }
    
    /**
     * @brief Tries to pop without blocking
     */
    Optional<T> try_pop() {
        LockGuard<Mutex> lock(mutex_);
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop_front();
        not_full_.notify_one();
        return item;
    }
    
    void close() {
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }
    
    void reopen() { closed_ = false; }
    bool is_closed() const { return closed_; }
    bool empty() const { LockGuard<Mutex> lock(mutex_); return queue_.empty(); }
    Size size() const { LockGuard<Mutex> lock(mutex_); return queue_.size(); }
    Size capacity() const { return max_size_; }
    Size dropped() const { return dropped_; }
    
    bool is_full() const {
        LockGuard<Mutex> lock(mutex_);
        return queue_.size() >= max_size_;
    }
    
    void clear() {
        LockGuard<Mutex> lock(mutex_);
        queue_.clear();
        not_full_.notify_all();
    }
};

// =============================================================================
// Priority Async Queue
// =============================================================================

/**
 * @brief Thread-safe priority queue
 */
template<typename T, typename Compare = std::less<T>>
class PriorityAsyncQueue {
private:
    std::priority_queue<T, Vector<T>, Compare> queue_;
    mutable Mutex mutex_;
    ConditionVar not_empty_;
    AtomicBool closed_{false};
    
public:
    PriorityAsyncQueue() = default;
    explicit PriorityAsyncQueue(Compare comp)
        : queue_(comp) {}
    
    /**
     * @brief Pushes an item maintaining priority order
     */
    bool push(T item) {
        if (closed_) return false;
        
        {
            LockGuard<Mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        not_empty_.notify_one();
        return true;
    }
    
    /**
     * @brief Pops highest priority item, blocking if empty
     */
    Optional<T> pop() {
        UniqueLock<Mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() {
            return !queue_.empty() || closed_;
        });
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(const_cast<T&>(queue_.top()));
        queue_.pop();
        return item;
    }
    
    /**
     * @brief Pops with timeout
     */
    Optional<T> pop(Milliseconds timeout) {
        UniqueLock<Mutex> lock(mutex_);
        bool success = not_empty_.wait_for(lock, timeout, [this]() {
            return !queue_.empty() || closed_;
        });
        
        if (!success || queue_.empty()) return std::nullopt;
        
        T item = std::move(const_cast<T&>(queue_.top()));
        queue_.pop();
        return item;
    }
    
    /**
     * @brief Tries to pop without blocking
     */
    Optional<T> try_pop() {
        LockGuard<Mutex> lock(mutex_);
        
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(const_cast<T&>(queue_.top()));
        queue_.pop();
        return item;
    }
    
    void close() {
        closed_ = true;
        not_empty_.notify_all();
    }
    
    void reopen() { closed_ = false; }
    bool is_closed() const { return closed_; }
    bool empty() const { LockGuard<Mutex> lock(mutex_); return queue_.empty(); }
    Size size() const { LockGuard<Mutex> lock(mutex_); return queue_.size(); }
    
    void clear() {
        LockGuard<Mutex> lock(mutex_);
        while (!queue_.empty()) queue_.pop();
    }
};

// =============================================================================
// Work Stealing Queue
// =============================================================================

/**
 * @brief Work-stealing deque for load balancing
 * 
 * Owner pushes/pops from back, thieves steal from front.
 */
template<typename T>
class WorkStealingQueue {
private:
    std::deque<T> queue_;
    mutable Mutex mutex_;
    
public:
    /**
     * @brief Push work (owner only, from back)
     */
    void push(T item) {
        LockGuard<Mutex> lock(mutex_);
        queue_.push_back(std::move(item));
    }
    
    /**
     * @brief Pop work (owner only, from back)
     */
    Optional<T> pop() {
        LockGuard<Mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.back());
        queue_.pop_back();
        return item;
    }
    
    /**
     * @brief Steal work (thieves, from front)
     */
    Optional<T> steal() {
        LockGuard<Mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        
        T item = std::move(queue_.front());
        queue_.pop_front();
        return item;
    }
    
    bool empty() const {
        LockGuard<Mutex> lock(mutex_);
        return queue_.empty();
    }
    
    Size size() const {
        LockGuard<Mutex> lock(mutex_);
        return queue_.size();
    }
};

} // namespace ims::common

#endif // IMS_COMMON_ASYNC_QUEUE_HPP
