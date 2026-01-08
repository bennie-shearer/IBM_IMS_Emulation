/**
 * @file connection_pool.hpp
 * @brief Generic connection pooling with health checks and leak detection
 * @version 3.6.2
 *
 * Provides connection pool management including:
 * - Generic connection pooling
 * - Automatic health checks
 * - Configurable pool sizing
 * - Connection leak detection
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_CONNECTION_POOL_HPP
#define IMS_COMMON_CONNECTION_POOL_HPP

#include "types.hpp"
#include "error.hpp"
#include <functional>
#include <deque>
#include <set>

namespace ims::common {

// =============================================================================
// Connection Pool Configuration
// =============================================================================

/**
 * @brief Configuration for connection pool behavior
 */
struct ConnectionPoolConfig {
    Size min_connections{2};          ///< Minimum idle connections
    Size max_connections{10};         ///< Maximum total connections
    Milliseconds connection_timeout{5000};  ///< Timeout for acquiring connection
    Milliseconds idle_timeout{60000};       ///< Max time connection can be idle
    Milliseconds health_check_interval{30000}; ///< Interval between health checks
    Milliseconds max_lifetime{3600000};     ///< Maximum connection lifetime
    Size max_wait_queue{100};         ///< Maximum queued requests
    bool validate_on_borrow{true};    ///< Validate connection when borrowed
    bool validate_on_return{false};   ///< Validate connection when returned
    bool test_on_idle{true};          ///< Test idle connections periodically
};

// =============================================================================
// Connection Pool Statistics
// =============================================================================

/**
 * @brief Statistics for connection pool monitoring
 */
struct ConnectionPoolStats {
    std::atomic<Size> total_connections{0};     ///< Total connections created
    std::atomic<Size> active_connections{0};    ///< Currently borrowed connections
    std::atomic<Size> idle_connections{0};      ///< Currently idle connections
    std::atomic<Size> pending_requests{0};      ///< Waiting acquire requests
    std::atomic<Size> successful_acquires{0};   ///< Successful connection acquires
    std::atomic<Size> failed_acquires{0};       ///< Failed acquire attempts
    std::atomic<Size> timeouts{0};              ///< Acquire timeouts
    std::atomic<Size> connections_created{0};   ///< Total connections created
    std::atomic<Size> connections_destroyed{0}; ///< Total connections destroyed
    std::atomic<Size> health_check_failures{0}; ///< Failed health checks
    std::atomic<Size> leaked_connections{0};    ///< Detected leaked connections
    TimePoint start_time{Clock::now()};
    
    void reset() {
        total_connections = 0;
        active_connections = 0;
        idle_connections = 0;
        pending_requests = 0;
        successful_acquires = 0;
        failed_acquires = 0;
        timeouts = 0;
        connections_created = 0;
        connections_destroyed = 0;
        health_check_failures = 0;
        leaked_connections = 0;
        start_time = Clock::now();
    }
    
    double utilization() const {
        Size total = total_connections.load();
        if (total == 0) return 0.0;
        return static_cast<double>(active_connections.load()) / 
               static_cast<double>(total) * 100.0;
    }
};

// =============================================================================
// Pooled Connection Wrapper
// =============================================================================

/**
 * @brief Wrapper for a pooled connection with metadata
 */
template<typename T>
struct PooledConnection {
    UniquePtr<T> connection;
    TimePoint created_at{Clock::now()};
    TimePoint last_used{Clock::now()};
    TimePoint borrowed_at{};
    Size borrow_count{0};
    bool is_valid{true};
    String borrower_info;  ///< For leak detection
    
    PooledConnection() = default;
    explicit PooledConnection(UniquePtr<T> conn) 
        : connection(std::move(conn)), created_at(Clock::now()), last_used(Clock::now()) {}
    
    Milliseconds age() const {
        return std::chrono::duration_cast<Milliseconds>(Clock::now() - created_at);
    }
    
    Milliseconds idle_time() const {
        return std::chrono::duration_cast<Milliseconds>(Clock::now() - last_used);
    }
    
    Milliseconds borrow_duration() const {
        if (borrowed_at.time_since_epoch().count() == 0) return Milliseconds{0};
        return std::chrono::duration_cast<Milliseconds>(Clock::now() - borrowed_at);
    }
};

// =============================================================================
// Connection Pool
// =============================================================================

/**
 * @brief Generic connection pool with health checks and monitoring
 * @tparam T Connection type (must be default constructible or use factory)
 */
template<typename T>
class ConnectionPool {
public:
    using ConnectionPtr = T*;
    using Factory = Function<UniquePtr<T>()>;
    using Validator = Function<bool(T&)>;
    using Destroyer = Function<void(T&)>;
    
private:
    ConnectionPoolConfig config_;
    ConnectionPoolStats stats_;
    Factory factory_;
    Validator validator_;
    Destroyer destroyer_;
    
    mutable Mutex mutex_;
    ConditionVar available_condition_;
    std::deque<PooledConnection<T>> idle_connections_;
    std::set<T*> active_connections_;
    HashMap<T*, PooledConnection<T>*> connection_map_;
    
    AtomicBool shutdown_{false};
    AtomicBool health_check_running_{false};
    UniquePtr<Thread> health_check_thread_;
    
    /**
     * @brief Creates a new connection
     */
    PooledConnection<T> create_connection() {
        auto conn = factory_();
        if (!conn) {
            throw std::runtime_error("Failed to create connection");
        }
        
        ++stats_.connections_created;
        ++stats_.total_connections;
        
        return PooledConnection<T>(std::move(conn));
    }
    
    /**
     * @brief Destroys a connection
     */
    void destroy_connection(PooledConnection<T>& pooled) {
        if (pooled.connection && destroyer_) {
            destroyer_(*pooled.connection);
        }
        pooled.connection.reset();
        ++stats_.connections_destroyed;
        --stats_.total_connections;
    }
    
    /**
     * @brief Validates a connection
     */
    bool validate_connection(PooledConnection<T>& pooled) {
        if (!pooled.connection) return false;
        if (!pooled.is_valid) return false;
        
        // Check lifetime
        if (pooled.age() > config_.max_lifetime) {
            return false;
        }
        
        // Run validator if provided
        if (validator_) {
            try {
                if (!validator_(*pooled.connection)) {
                    ++stats_.health_check_failures;
                    return false;
                }
            } catch (...) {
                ++stats_.health_check_failures;
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * @brief Health check thread function
     */
    void health_check_loop() {
        while (!shutdown_) {
            {
                LockGuard<Mutex> lock(mutex_);
                
                // Check idle connections
                auto it = idle_connections_.begin();
                while (it != idle_connections_.end()) {
                    bool should_remove = false;
                    
                    // Check idle timeout
                    if (it->idle_time() > config_.idle_timeout &&
                        idle_connections_.size() > config_.min_connections) {
                        should_remove = true;
                    }
                    // Check max lifetime
                    else if (it->age() > config_.max_lifetime) {
                        should_remove = true;
                    }
                    // Test on idle
                    else if (config_.test_on_idle && !validate_connection(*it)) {
                        should_remove = true;
                    }
                    
                    if (should_remove) {
                        destroy_connection(*it);
                        it = idle_connections_.erase(it);
                        --stats_.idle_connections;
                    } else {
                        ++it;
                    }
                }
                
                // Ensure minimum connections
                while (idle_connections_.size() < config_.min_connections &&
                       stats_.total_connections < config_.max_connections) {
                    try {
                        idle_connections_.push_back(create_connection());
                        ++stats_.idle_connections;
                    } catch (...) {
                        break;
                    }
                }
            }
            
            // Sleep until next check
            std::this_thread::sleep_for(config_.health_check_interval);
        }
    }
    
public:
    /**
     * @brief Constructs connection pool with factory function
     */
    explicit ConnectionPool(Factory factory, 
                           const ConnectionPoolConfig& config = ConnectionPoolConfig{})
        : config_(config), factory_(std::move(factory)) {
        
        // Pre-create minimum connections
        for (Size i = 0; i < config_.min_connections; ++i) {
            try {
                idle_connections_.push_back(create_connection());
                ++stats_.idle_connections;
            } catch (...) {
                // Ignore creation failures during startup
            }
        }
    }
    
    ~ConnectionPool() {
        shutdown();
    }
    
    // Non-copyable
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    
    /**
     * @brief Sets the connection validator
     */
    void set_validator(Validator validator) {
        LockGuard<Mutex> lock(mutex_);
        validator_ = std::move(validator);
    }
    
    /**
     * @brief Sets the connection destroyer
     */
    void set_destroyer(Destroyer destroyer) {
        LockGuard<Mutex> lock(mutex_);
        destroyer_ = std::move(destroyer);
    }
    
    /**
     * @brief Starts background health check thread
     */
    void start_health_checks() {
        if (health_check_running_.exchange(true)) return;
        
        health_check_thread_ = std::make_unique<Thread>([this]() {
            health_check_loop();
        });
    }
    
    /**
     * @brief Acquires a connection from the pool
     * @param borrower_info Optional info for leak detection
     * @return Pointer to connection or nullptr on failure
     */
    T* acquire(const String& borrower_info = "") {
        UniqueLock<Mutex> lock(mutex_);
        ++stats_.pending_requests;
        
        auto deadline = Clock::now() + config_.connection_timeout;
        
        while (true) {
            // Try to get an idle connection
            while (!idle_connections_.empty()) {
                auto pooled = std::move(idle_connections_.front());
                idle_connections_.pop_front();
                --stats_.idle_connections;
                
                // Validate if required
                if (config_.validate_on_borrow && !validate_connection(pooled)) {
                    destroy_connection(pooled);
                    continue;
                }
                
                // Mark as borrowed
                pooled.borrowed_at = Clock::now();
                pooled.last_used = Clock::now();
                ++pooled.borrow_count;
                pooled.borrower_info = borrower_info;
                
                T* conn_ptr = pooled.connection.get();
                active_connections_.insert(conn_ptr);
                
                // Store mapping for return
                auto* stored = new PooledConnection<T>(std::move(pooled));
                connection_map_[conn_ptr] = stored;
                
                ++stats_.active_connections;
                ++stats_.successful_acquires;
                --stats_.pending_requests;
                
                return conn_ptr;
            }
            
            // Try to create new connection
            if (stats_.total_connections < config_.max_connections) {
                try {
                    auto pooled = create_connection();
                    pooled.borrowed_at = Clock::now();
                    pooled.borrower_info = borrower_info;
                    
                    T* conn_ptr = pooled.connection.get();
                    active_connections_.insert(conn_ptr);
                    
                    auto* stored = new PooledConnection<T>(std::move(pooled));
                    connection_map_[conn_ptr] = stored;
                    
                    ++stats_.active_connections;
                    ++stats_.successful_acquires;
                    --stats_.pending_requests;
                    
                    return conn_ptr;
                } catch (...) {
                    // Fall through to wait
                }
            }
            
            // Wait for a connection to become available
            if (available_condition_.wait_until(lock, deadline) == std::cv_status::timeout) {
                ++stats_.timeouts;
                ++stats_.failed_acquires;
                --stats_.pending_requests;
                return nullptr;
            }
            
            if (shutdown_) {
                --stats_.pending_requests;
                return nullptr;
            }
        }
    }
    
    /**
     * @brief Returns a connection to the pool
     */
    void release(T* connection) {
        if (!connection) return;
        
        LockGuard<Mutex> lock(mutex_);
        
        auto map_it = connection_map_.find(connection);
        if (map_it == connection_map_.end()) {
            // Unknown connection - possible double release
            return;
        }
        
        PooledConnection<T>* stored = map_it->second;
        connection_map_.erase(map_it);
        active_connections_.erase(connection);
        --stats_.active_connections;
        
        // Validate if required
        bool should_keep = true;
        if (config_.validate_on_return) {
            should_keep = validate_connection(*stored);
        }
        
        // Check lifetime
        if (stored->age() > config_.max_lifetime) {
            should_keep = false;
        }
        
        if (should_keep && !shutdown_) {
            stored->last_used = Clock::now();
            stored->borrowed_at = TimePoint{};
            stored->borrower_info.clear();
            idle_connections_.push_back(std::move(*stored));
            ++stats_.idle_connections;
        } else {
            destroy_connection(*stored);
        }
        
        delete stored;
        available_condition_.notify_one();
    }
    
    /**
     * @brief RAII wrapper for automatic connection release
     */
    class ScopedConnection {
    private:
        ConnectionPool* pool_;
        T* connection_;
        
    public:
        ScopedConnection(ConnectionPool* pool, T* conn) 
            : pool_(pool), connection_(conn) {}
        
        ~ScopedConnection() {
            if (pool_ && connection_) {
                pool_->release(connection_);
            }
        }
        
        // Move only
        ScopedConnection(const ScopedConnection&) = delete;
        ScopedConnection& operator=(const ScopedConnection&) = delete;
        ScopedConnection(ScopedConnection&& other) noexcept 
            : pool_(other.pool_), connection_(other.connection_) {
            other.pool_ = nullptr;
            other.connection_ = nullptr;
        }
        
        T* get() const { return connection_; }
        T* operator->() const { return connection_; }
        T& operator*() const { return *connection_; }
        explicit operator bool() const { return connection_ != nullptr; }
        
        void release() {
            if (pool_ && connection_) {
                pool_->release(connection_);
                connection_ = nullptr;
            }
        }
    };
    
    /**
     * @brief Acquires a connection with automatic release
     */
    ScopedConnection acquire_scoped(const String& borrower_info = "") {
        return ScopedConnection(this, acquire(borrower_info));
    }
    
    /**
     * @brief Shuts down the pool and releases all connections
     */
    void shutdown() {
        shutdown_ = true;
        
        {
            LockGuard<Mutex> lock(mutex_);
            available_condition_.notify_all();
            
            // Destroy idle connections
            for (auto& pooled : idle_connections_) {
                destroy_connection(pooled);
            }
            idle_connections_.clear();
            stats_.idle_connections = 0;
            
            // Log leaked connections
            for (const auto& entry : connection_map_) {
                ++stats_.leaked_connections;
                // In production, would log: entry.second->borrower_info, entry.second->borrow_duration()
                (void)entry;  // Suppress unused warning
            }
        }
        
        // Stop health check thread
        if (health_check_thread_ && health_check_thread_->joinable()) {
            health_check_thread_->join();
        }
    }
    
    /**
     * @brief Gets pool statistics
     */
    const ConnectionPoolStats& stats() const { return stats_; }
    
    /**
     * @brief Gets pool configuration
     */
    const ConnectionPoolConfig& config() const { return config_; }
    
    /**
     * @brief Gets current pool size
     */
    Size size() const { return stats_.total_connections.load(); }
    
    /**
     * @brief Gets number of idle connections
     */
    Size idle() const { return stats_.idle_connections.load(); }
    
    /**
     * @brief Gets number of active connections
     */
    Size active() const { return stats_.active_connections.load(); }
    
    /**
     * @brief Checks if pool is empty
     */
    bool empty() const { return size() == 0; }
};

// =============================================================================
// Connection Pool Builder
// =============================================================================

/**
 * @brief Builder pattern for connection pool configuration
 */
template<typename T>
class ConnectionPoolBuilder {
private:
    ConnectionPoolConfig config_;
    typename ConnectionPool<T>::Factory factory_;
    typename ConnectionPool<T>::Validator validator_;
    typename ConnectionPool<T>::Destroyer destroyer_;
    
public:
    ConnectionPoolBuilder& min_connections(Size count) {
        config_.min_connections = count;
        return *this;
    }
    
    ConnectionPoolBuilder& max_connections(Size count) {
        config_.max_connections = count;
        return *this;
    }
    
    ConnectionPoolBuilder& connection_timeout(Milliseconds timeout) {
        config_.connection_timeout = timeout;
        return *this;
    }
    
    ConnectionPoolBuilder& idle_timeout(Milliseconds timeout) {
        config_.idle_timeout = timeout;
        return *this;
    }
    
    ConnectionPoolBuilder& health_check_interval(Milliseconds interval) {
        config_.health_check_interval = interval;
        return *this;
    }
    
    ConnectionPoolBuilder& max_lifetime(Milliseconds lifetime) {
        config_.max_lifetime = lifetime;
        return *this;
    }
    
    ConnectionPoolBuilder& validate_on_borrow(bool enabled) {
        config_.validate_on_borrow = enabled;
        return *this;
    }
    
    ConnectionPoolBuilder& validate_on_return(bool enabled) {
        config_.validate_on_return = enabled;
        return *this;
    }
    
    ConnectionPoolBuilder& test_on_idle(bool enabled) {
        config_.test_on_idle = enabled;
        return *this;
    }
    
    ConnectionPoolBuilder& factory(typename ConnectionPool<T>::Factory f) {
        factory_ = std::move(f);
        return *this;
    }
    
    ConnectionPoolBuilder& validator(typename ConnectionPool<T>::Validator v) {
        validator_ = std::move(v);
        return *this;
    }
    
    ConnectionPoolBuilder& destroyer(typename ConnectionPool<T>::Destroyer d) {
        destroyer_ = std::move(d);
        return *this;
    }
    
    UniquePtr<ConnectionPool<T>> build() {
        if (!factory_) {
            throw std::runtime_error("Connection factory is required");
        }
        
        auto pool = std::make_unique<ConnectionPool<T>>(factory_, config_);
        
        if (validator_) {
            pool->set_validator(validator_);
        }
        
        if (destroyer_) {
            pool->set_destroyer(destroyer_);
        }
        
        return pool;
    }
};

} // namespace ims::common

#endif // IMS_COMMON_CONNECTION_POOL_HPP
