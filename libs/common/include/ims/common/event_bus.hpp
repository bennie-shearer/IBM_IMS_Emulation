/**
 * @file event_bus.hpp
 * @brief Event bus and publish-subscribe pattern implementation
 * @version 3.6.3
 *
 * Provides event handling including:
 * - Type-safe event bus
 * - Publish-subscribe pattern
 * - Synchronous and asynchronous dispatch
 * - Event filtering and prioritization
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_EVENT_BUS_HPP
#define IMS_COMMON_EVENT_BUS_HPP

#include "types.hpp"
#include <typeindex>
#include <any>
#include <queue>

namespace ims::common {

// =============================================================================
// Event Base
// =============================================================================

/**
 * @brief Base class for all events
 */
struct EventBase {
    SystemTimePoint timestamp{SystemClock::now()};
    String source;
    bool cancelled{false};
    int priority{0};
    
    virtual ~EventBase() = default;
    virtual std::type_index type() const = 0;
    
    void cancel() { cancelled = true; }
};

/**
 * @brief Typed event template
 */
template<typename T>
struct Event : EventBase {
    T data;
    
    Event() = default;
    explicit Event(T event_data) : data(std::move(event_data)) {}
    
    std::type_index type() const override {
        return std::type_index(typeid(T));
    }
};

// =============================================================================
// Subscription Handle
// =============================================================================

/**
 * @brief Handle for managing subscriptions
 */
class SubscriptionHandle {
private:
    Function<void()> unsubscribe_fn_;
    bool active_{true};
    
public:
    SubscriptionHandle() = default;
    explicit SubscriptionHandle(Function<void()> unsub) 
        : unsubscribe_fn_(std::move(unsub)) {}
    
    void unsubscribe() {
        if (active_ && unsubscribe_fn_) {
            unsubscribe_fn_();
            active_ = false;
        }
    }
    
    bool is_active() const { return active_; }
    
    // RAII support
    ~SubscriptionHandle() {
        // Does not auto-unsubscribe to allow handle copying
    }
};

/**
 * @brief RAII subscription that unsubscribes on destruction
 */
class ScopedSubscription {
private:
    SubscriptionHandle handle_;
    
public:
    ScopedSubscription() = default;
    explicit ScopedSubscription(SubscriptionHandle handle) 
        : handle_(std::move(handle)) {}
    
    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;
    
    ScopedSubscription(ScopedSubscription&& other) noexcept 
        : handle_(std::move(other.handle_)) {}
    
    ScopedSubscription& operator=(ScopedSubscription&& other) noexcept {
        if (this != &other) {
            handle_.unsubscribe();
            handle_ = std::move(other.handle_);
        }
        return *this;
    }
    
    ~ScopedSubscription() {
        handle_.unsubscribe();
    }
    
    void release() { handle_ = SubscriptionHandle{}; }
    bool is_active() const { return handle_.is_active(); }
};

// =============================================================================
// Event Bus
// =============================================================================

/**
 * @brief Type-safe event bus for publish-subscribe pattern
 */
class EventBus {
private:
    struct SubscriberInfo {
        Size id;
        std::any handler;
        int priority;
        bool once;
    };
    
    using SubscriberList = Vector<SubscriberInfo>;
    
    HashMap<std::type_index, SubscriberList> subscribers_;
    mutable SharedMutex mutex_;
    std::atomic<Size> next_id_{0};
    
    template<typename T>
    SubscriberList& get_subscribers() {
        auto type = std::type_index(typeid(T));
        return subscribers_[type];
    }
    
public:
    EventBus() = default;
    
    /**
     * @brief Subscribes to events of type T
     * @param handler Callback function receiving Event<T>&
     * @param priority Higher priority handlers called first
     * @return Subscription handle for unsubscribing
     */
    template<typename T>
    SubscriptionHandle subscribe(Function<void(Event<T>&)> handler, int priority = 0) {
        Size id = next_id_++;
        
        {
            LockGuard<SharedMutex> lock(mutex_);
            auto& subs = get_subscribers<T>();
            subs.push_back({id, std::move(handler), priority, false});
            
            // Sort by priority (descending)
            std::sort(subs.begin(), subs.end(), 
                [](const SubscriberInfo& a, const SubscriberInfo& b) {
                    return a.priority > b.priority;
                });
        }
        
        return SubscriptionHandle([this, id]() {
            unsubscribe<T>(id);
        });
    }
    
    /**
     * @brief Subscribes to a single event occurrence
     */
    template<typename T>
    SubscriptionHandle subscribe_once(Function<void(Event<T>&)> handler, int priority = 0) {
        Size id = next_id_++;
        
        {
            LockGuard<SharedMutex> lock(mutex_);
            auto& subs = get_subscribers<T>();
            subs.push_back({id, std::move(handler), priority, true});
            
            std::sort(subs.begin(), subs.end(),
                [](const SubscriberInfo& a, const SubscriberInfo& b) {
                    return a.priority > b.priority;
                });
        }
        
        return SubscriptionHandle([this, id]() {
            unsubscribe<T>(id);
        });
    }
    
    /**
     * @brief Unsubscribes by ID
     */
    template<typename T>
    void unsubscribe(Size id) {
        LockGuard<SharedMutex> lock(mutex_);
        auto type = std::type_index(typeid(T));
        auto it = subscribers_.find(type);
        if (it != subscribers_.end()) {
            auto& subs = it->second;
            subs.erase(
                std::remove_if(subs.begin(), subs.end(),
                    [id](const SubscriberInfo& s) { return s.id == id; }),
                subs.end());
        }
    }
    
    /**
     * @brief Publishes an event to all subscribers
     * @return Number of handlers that processed the event
     */
    template<typename T>
    Size publish(Event<T>& event) {
        Vector<SubscriberInfo> to_call;
        Vector<Size> to_remove;
        
        {
            SharedLock<SharedMutex> lock(mutex_);
            auto type = std::type_index(typeid(T));
            auto it = subscribers_.find(type);
            if (it != subscribers_.end()) {
                to_call = it->second;
            }
        }
        
        Size count = 0;
        for (auto& sub : to_call) {
            if (event.cancelled) break;
            
            try {
                auto& handler = std::any_cast<Function<void(Event<T>&)>&>(sub.handler);
                handler(event);
                ++count;
                
                if (sub.once) {
                    to_remove.push_back(sub.id);
                }
            } catch (const std::bad_any_cast&) {
                // Type mismatch, skip
            }
        }
        
        // Remove one-shot subscriptions
        if (!to_remove.empty()) {
            LockGuard<SharedMutex> lock(mutex_);
            auto type = std::type_index(typeid(T));
            auto it = subscribers_.find(type);
            if (it != subscribers_.end()) {
                auto& subs = it->second;
                for (Size id : to_remove) {
                    subs.erase(
                        std::remove_if(subs.begin(), subs.end(),
                            [id](const SubscriberInfo& s) { return s.id == id; }),
                        subs.end());
                }
            }
        }
        
        return count;
    }
    
    /**
     * @brief Publishes event data (creates Event wrapper)
     */
    template<typename T>
    Size publish(T data) {
        Event<T> event(std::move(data));
        return publish(event);
    }
    
    /**
     * @brief Checks if there are subscribers for event type
     */
    template<typename T>
    bool has_subscribers() const {
        SharedLock<SharedMutex> lock(mutex_);
        auto type = std::type_index(typeid(T));
        auto it = subscribers_.find(type);
        return it != subscribers_.end() && !it->second.empty();
    }
    
    /**
     * @brief Gets subscriber count for event type
     */
    template<typename T>
    Size subscriber_count() const {
        SharedLock<SharedMutex> lock(mutex_);
        auto type = std::type_index(typeid(T));
        auto it = subscribers_.find(type);
        return it != subscribers_.end() ? it->second.size() : 0;
    }
    
    /**
     * @brief Clears all subscribers for event type
     */
    template<typename T>
    void clear() {
        LockGuard<SharedMutex> lock(mutex_);
        auto type = std::type_index(typeid(T));
        subscribers_.erase(type);
    }
    
    /**
     * @brief Clears all subscribers
     */
    void clear_all() {
        LockGuard<SharedMutex> lock(mutex_);
        subscribers_.clear();
    }
};

// =============================================================================
// Async Event Bus
// =============================================================================

/**
 * @brief Asynchronous event bus with queued dispatch
 */
class AsyncEventBus {
private:
    struct QueuedEvent {
        std::any event;
        Function<void()> dispatch_fn;
        int priority;
        SystemTimePoint queued_at;
    };
    
    struct PriorityCompare {
        bool operator()(const QueuedEvent& a, const QueuedEvent& b) const {
            return a.priority < b.priority;  // Higher priority first
        }
    };
    
    EventBus sync_bus_;
    std::priority_queue<QueuedEvent, Vector<QueuedEvent>, PriorityCompare> queue_;
    mutable Mutex queue_mutex_;
    ConditionVar queue_cv_;
    AtomicBool running_{false};
    UniquePtr<Thread> dispatch_thread_;
    
    void dispatch_loop() {
        while (running_) {
            QueuedEvent event;
            {
                UniqueLock<Mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this]() {
                    return !running_ || !queue_.empty();
                });
                
                if (!running_ && queue_.empty()) break;
                if (queue_.empty()) continue;
                
                event = std::move(const_cast<QueuedEvent&>(queue_.top()));
                queue_.pop();
            }
            
            if (event.dispatch_fn) {
                event.dispatch_fn();
            }
        }
    }
    
public:
    AsyncEventBus() = default;
    
    ~AsyncEventBus() {
        stop();
    }
    
    /**
     * @brief Starts the async dispatch thread
     */
    void start() {
        if (running_.exchange(true)) return;
        dispatch_thread_ = std::make_unique<Thread>([this]() {
            dispatch_loop();
        });
    }
    
    /**
     * @brief Stops the async dispatch thread
     */
    void stop() {
        running_ = false;
        queue_cv_.notify_all();
        if (dispatch_thread_ && dispatch_thread_->joinable()) {
            dispatch_thread_->join();
        }
    }
    
    /**
     * @brief Subscribes to events (delegates to sync bus)
     */
    template<typename T>
    SubscriptionHandle subscribe(Function<void(Event<T>&)> handler, int priority = 0) {
        return sync_bus_.subscribe<T>(std::move(handler), priority);
    }
    
    /**
     * @brief Queues an event for async dispatch
     */
    template<typename T>
    void publish_async(T data, int priority = 0) {
        Event<T> event(std::move(data));
        
        QueuedEvent queued;
        queued.event = std::move(event);
        queued.priority = priority;
        queued.queued_at = SystemClock::now();
        queued.dispatch_fn = [this, e = std::any_cast<Event<T>>(queued.event)]() mutable {
            sync_bus_.publish(e);
        };
        
        {
            LockGuard<Mutex> lock(queue_mutex_);
            queue_.push(std::move(queued));
        }
        queue_cv_.notify_one();
    }
    
    /**
     * @brief Publishes synchronously (bypasses queue)
     */
    template<typename T>
    Size publish_sync(T data) {
        return sync_bus_.publish(std::move(data));
    }
    
    /**
     * @brief Gets pending event count
     */
    Size pending_count() const {
        LockGuard<Mutex> lock(queue_mutex_);
        return queue_.size();
    }
    
    /**
     * @brief Checks if running
     */
    bool is_running() const { return running_; }
    
    /**
     * @brief Gets underlying sync bus
     */
    EventBus& sync_bus() { return sync_bus_; }
};

// =============================================================================
// Topic-Based Pub/Sub
// =============================================================================

/**
 * @brief Topic-based publish-subscribe system
 */
class TopicPubSub {
private:
    struct Subscriber {
        Size id;
        Function<void(const String&, const std::any&)> handler;
    };
    
    HashMap<String, Vector<Subscriber>> topics_;
    mutable SharedMutex mutex_;
    std::atomic<Size> next_id_{0};
    
public:
    /**
     * @brief Subscribes to a topic
     * @param topic Topic name (supports wildcards: * for single level, # for multi-level)
     * @param handler Callback receiving topic name and data
     */
    SubscriptionHandle subscribe(const String& topic, 
                                 Function<void(const String&, const std::any&)> handler) {
        Size id = next_id_++;
        
        {
            LockGuard<SharedMutex> lock(mutex_);
            topics_[topic].push_back({id, std::move(handler)});
        }
        
        return SubscriptionHandle([this, topic, id]() {
            LockGuard<SharedMutex> lock(mutex_);
            auto it = topics_.find(topic);
            if (it != topics_.end()) {
                auto& subs = it->second;
                subs.erase(
                    std::remove_if(subs.begin(), subs.end(),
                        [id](const Subscriber& s) { return s.id == id; }),
                    subs.end());
            }
        });
    }
    
    /**
     * @brief Publishes data to a topic
     */
    template<typename T>
    Size publish(const String& topic, T data) {
        std::any any_data = std::move(data);
        Vector<Subscriber> matched;
        
        {
            SharedLock<SharedMutex> lock(mutex_);
            
            for (const auto& [pattern, subs] : topics_) {
                if (matches_topic(pattern, topic)) {
                    matched.insert(matched.end(), subs.begin(), subs.end());
                }
            }
        }
        
        Size count = 0;
        for (const auto& sub : matched) {
            try {
                sub.handler(topic, any_data);
                ++count;
            } catch (...) {
                // Handler exception, continue
            }
        }
        
        return count;
    }
    
    /**
     * @brief Checks if topic pattern matches actual topic
     */
    static bool matches_topic(const String& pattern, const String& topic) {
        if (pattern == topic) return true;
        if (pattern == "#") return true;
        
        // Simple wildcard matching
        Vector<String> pattern_parts, topic_parts;
        
        auto split = [](const String& s, char delim) {
            Vector<String> parts;
            Size start = 0;
            for (Size i = 0; i <= s.size(); ++i) {
                if (i == s.size() || s[i] == delim) {
                    parts.push_back(s.substr(start, i - start));
                    start = i + 1;
                }
            }
            return parts;
        };
        
        pattern_parts = split(pattern, '/');
        topic_parts = split(topic, '/');
        
        Size pi = 0, ti = 0;
        while (pi < pattern_parts.size() && ti < topic_parts.size()) {
            if (pattern_parts[pi] == "#") {
                return true;  // # matches rest
            }
            if (pattern_parts[pi] == "*") {
                ++pi;
                ++ti;
                continue;  // * matches single level
            }
            if (pattern_parts[pi] != topic_parts[ti]) {
                return false;
            }
            ++pi;
            ++ti;
        }
        
        return pi == pattern_parts.size() && ti == topic_parts.size();
    }
    
    /**
     * @brief Gets subscriber count for a topic pattern
     */
    Size subscriber_count(const String& topic) const {
        SharedLock<SharedMutex> lock(mutex_);
        auto it = topics_.find(topic);
        return it != topics_.end() ? it->second.size() : 0;
    }
    
    /**
     * @brief Clears all subscriptions for a topic
     */
    void clear_topic(const String& topic) {
        LockGuard<SharedMutex> lock(mutex_);
        topics_.erase(topic);
    }
    
    /**
     * @brief Clears all subscriptions
     */
    void clear_all() {
        LockGuard<SharedMutex> lock(mutex_);
        topics_.clear();
    }
};

// =============================================================================
// Global Event Bus
// =============================================================================

/**
 * @brief Gets the global event bus instance
 */
inline EventBus& global_event_bus() {
    static EventBus bus;
    return bus;
}

} // namespace ims::common

#endif // IMS_COMMON_EVENT_BUS_HPP
