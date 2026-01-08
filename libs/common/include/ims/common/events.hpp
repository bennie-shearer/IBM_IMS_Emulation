#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Event System
// Version: 3.6.3
// NEW in v3.6.3: Observer pattern and event handling
// =============================================================================

#include "types.hpp"
#include <algorithm>

namespace ims {

// =============================================================================
// Event Types
// =============================================================================

enum class EventType {
    // Dataset events
    DATASET_OPENED,
    DATASET_CLOSED,
    DATASET_CREATED,
    DATASET_DELETED,
    
    // Record events
    RECORD_READ,
    RECORD_WRITTEN,
    RECORD_UPDATED,
    RECORD_DELETED,
    
    // Catalog events
    CATALOG_ENTRY_ADDED,
    CATALOG_ENTRY_REMOVED,
    CATALOG_ENTRY_UPDATED,
    
    // Security events
    USER_AUTHENTICATED,
    USER_LOGGED_OUT,
    AUTHORIZATION_GRANTED,
    AUTHORIZATION_DENIED,
    
    // Transaction events
    TRANSACTION_STARTED,
    TRANSACTION_COMMITTED,
    TRANSACTION_ROLLED_BACK,
    
    // System events
    SYSTEM_INITIALIZED,
    SYSTEM_SHUTDOWN,
    ERROR_OCCURRED,
    WARNING_RAISED,
    
    // Custom events
    CUSTOM = 1000
};

inline String event_type_to_string(EventType type) {
    switch (type) {
        case EventType::DATASET_OPENED: return "DATASET_OPENED";
        case EventType::DATASET_CLOSED: return "DATASET_CLOSED";
        case EventType::DATASET_CREATED: return "DATASET_CREATED";
        case EventType::DATASET_DELETED: return "DATASET_DELETED";
        case EventType::RECORD_READ: return "RECORD_READ";
        case EventType::RECORD_WRITTEN: return "RECORD_WRITTEN";
        case EventType::RECORD_UPDATED: return "RECORD_UPDATED";
        case EventType::RECORD_DELETED: return "RECORD_DELETED";
        case EventType::CATALOG_ENTRY_ADDED: return "CATALOG_ENTRY_ADDED";
        case EventType::CATALOG_ENTRY_REMOVED: return "CATALOG_ENTRY_REMOVED";
        case EventType::CATALOG_ENTRY_UPDATED: return "CATALOG_ENTRY_UPDATED";
        case EventType::USER_AUTHENTICATED: return "USER_AUTHENTICATED";
        case EventType::USER_LOGGED_OUT: return "USER_LOGGED_OUT";
        case EventType::AUTHORIZATION_GRANTED: return "AUTHORIZATION_GRANTED";
        case EventType::AUTHORIZATION_DENIED: return "AUTHORIZATION_DENIED";
        case EventType::TRANSACTION_STARTED: return "TRANSACTION_STARTED";
        case EventType::TRANSACTION_COMMITTED: return "TRANSACTION_COMMITTED";
        case EventType::TRANSACTION_ROLLED_BACK: return "TRANSACTION_ROLLED_BACK";
        case EventType::SYSTEM_INITIALIZED: return "SYSTEM_INITIALIZED";
        case EventType::SYSTEM_SHUTDOWN: return "SYSTEM_SHUTDOWN";
        case EventType::ERROR_OCCURRED: return "ERROR_OCCURRED";
        case EventType::WARNING_RAISED: return "WARNING_RAISED";
        default: return "CUSTOM";
    }
}

// =============================================================================
// Event Data Base
// =============================================================================

struct EventData {
    EventType type;
    SystemTimePoint timestamp;
    String source;
    String message;
    HashMap<String, String> properties;
    
    EventData() : timestamp(SystemClock::now()) {}
    
    explicit EventData(EventType t, const String& src = "", const String& msg = "")
        : type(t), timestamp(SystemClock::now()), source(src), message(msg) {}
    
    void set_property(const String& key, const String& value) {
        properties[key] = value;
    }
    
    String get_property(const String& key, const String& def = "") const {
        auto it = properties.find(key);
        return it != properties.end() ? it->second : def;
    }
    
    bool has_property(const String& key) const {
        return properties.find(key) != properties.end();
    }
};

// =============================================================================
// Event Handler Interface
// =============================================================================

class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void on_event(const EventData& event) = 0;
    virtual bool accepts(EventType type) const { return true; }
};

using EventHandlerPtr = SharedPtr<IEventHandler>;
using EventCallback = Function<void(const EventData&)>;

// =============================================================================
// Callback Event Handler
// =============================================================================

class CallbackEventHandler : public IEventHandler {
private:
    EventCallback callback_;
    Vector<EventType> accepted_types_;
    bool accept_all_;
    
public:
    explicit CallbackEventHandler(EventCallback callback)
        : callback_(std::move(callback)), accept_all_(true) {}
    
    CallbackEventHandler(EventCallback callback, Vector<EventType> types)
        : callback_(std::move(callback)), accepted_types_(std::move(types)), accept_all_(false) {}
    
    void on_event(const EventData& event) override {
        if (callback_) {
            callback_(event);
        }
    }
    
    bool accepts(EventType type) const override {
        if (accept_all_) return true;
        return std::find(accepted_types_.begin(), accepted_types_.end(), type) != accepted_types_.end();
    }
};

// =============================================================================
// Event Dispatcher
// =============================================================================

class EventDispatcher {
public:
    using SubscriptionId = UInt64;
    
private:
    struct Subscription {
        SubscriptionId id;
        EventHandlerPtr handler;
        bool enabled;
    };
    
    Vector<Subscription> subscriptions_;
    mutable SharedMutex mutex_;
    std::atomic<SubscriptionId> next_id_{1};
    std::atomic<bool> dispatching_{false};
    std::atomic<UInt64> events_dispatched_{0};
    
public:
    EventDispatcher() = default;
    
    // Subscribe with handler
    SubscriptionId subscribe(EventHandlerPtr handler) {
        std::unique_lock lock(mutex_);
        SubscriptionId id = next_id_++;
        subscriptions_.push_back({id, std::move(handler), true});
        return id;
    }
    
    // Subscribe with callback
    SubscriptionId subscribe(EventCallback callback) {
        return subscribe(std::make_shared<CallbackEventHandler>(std::move(callback)));
    }
    
    // Subscribe with callback and filter
    SubscriptionId subscribe(EventCallback callback, Vector<EventType> types) {
        return subscribe(std::make_shared<CallbackEventHandler>(std::move(callback), std::move(types)));
    }
    
    // Subscribe to single event type
    SubscriptionId subscribe(EventType type, EventCallback callback) {
        return subscribe(std::make_shared<CallbackEventHandler>(std::move(callback), Vector<EventType>{type}));
    }
    
    // Unsubscribe
    void unsubscribe(SubscriptionId id) {
        std::unique_lock lock(mutex_);
        subscriptions_.erase(
            std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                          [id](const Subscription& s) { return s.id == id; }),
            subscriptions_.end()
        );
    }
    
    // Enable/disable subscription
    void set_enabled(SubscriptionId id, bool enabled) {
        std::shared_lock lock(mutex_);
        for (auto& sub : subscriptions_) {
            if (sub.id == id) {
                sub.enabled = enabled;
                break;
            }
        }
    }
    
    // Dispatch event
    void dispatch(const EventData& event) {
        dispatching_ = true;
        ++events_dispatched_;
        
        std::shared_lock lock(mutex_);
        for (const auto& sub : subscriptions_) {
            if (sub.enabled && sub.handler && sub.handler->accepts(event.type)) {
                try {
                    sub.handler->on_event(event);
                } catch (...) {
                    // Swallow exceptions from handlers
                }
            }
        }
        
        dispatching_ = false;
    }
    
    // Convenience dispatch
    void dispatch(EventType type, const String& source = "", const String& message = "") {
        dispatch(EventData(type, source, message));
    }
    
    // Statistics
    Size subscription_count() const {
        std::shared_lock lock(mutex_);
        return subscriptions_.size();
    }
    
    UInt64 events_dispatched() const { return events_dispatched_.load(); }
    bool is_dispatching() const { return dispatching_.load(); }
    
    // Clear all subscriptions
    void clear() {
        std::unique_lock lock(mutex_);
        subscriptions_.clear();
    }
};

// =============================================================================
// Global Event Dispatcher
// =============================================================================

inline EventDispatcher& global_event_dispatcher() {
    static EventDispatcher instance;
    return instance;
}

// =============================================================================
// Event Macros
// =============================================================================

#define IMS_EMIT_EVENT(type, source, message) \
    ims::global_event_dispatcher().dispatch(type, source, message)

#define IMS_EMIT_EVENT_DATA(event_data) \
    ims::global_event_dispatcher().dispatch(event_data)

// =============================================================================
// Scoped Event Subscription (RAII)
// =============================================================================

class ScopedSubscription {
private:
    EventDispatcher& dispatcher_;
    EventDispatcher::SubscriptionId id_;
    
public:
    ScopedSubscription(EventDispatcher& dispatcher, EventCallback callback)
        : dispatcher_(dispatcher), id_(dispatcher.subscribe(std::move(callback))) {}
    
    ScopedSubscription(EventDispatcher& dispatcher, EventType type, EventCallback callback)
        : dispatcher_(dispatcher), id_(dispatcher.subscribe(type, std::move(callback))) {}
    
    ~ScopedSubscription() {
        dispatcher_.unsubscribe(id_);
    }
    
    // Non-copyable, non-movable
    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;
    
    EventDispatcher::SubscriptionId id() const { return id_; }
};

} // namespace ims
