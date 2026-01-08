/**
 * @file signal_handler.hpp
 * @brief Cross-platform signal handling for graceful shutdown
 * @version 3.6.2
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_SIGNAL_HANDLER_HPP
#define IMS_COMMON_SIGNAL_HANDLER_HPP

#include "types.hpp"
#include <atomic>
#include <csignal>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#endif

namespace ims::common {

/**
 * @brief Signal types supported across platforms
 */
enum class SignalType {
    Interrupt,      // SIGINT (Ctrl+C)
    Terminate,      // SIGTERM
    Hangup,         // SIGHUP (Unix only)
    User1,          // SIGUSR1 (Unix only)
    User2,          // SIGUSR2 (Unix only)
    Pipe,           // SIGPIPE (Unix only)
    Alarm,          // SIGALRM (Unix only)
    Child           // SIGCHLD (Unix only)
};

/**
 * @brief Convert SignalType to native signal number
 */
inline int signal_to_native(SignalType sig) {
    switch (sig) {
        case SignalType::Interrupt:  return SIGINT;
        case SignalType::Terminate:  return SIGTERM;
#ifndef _WIN32
        case SignalType::Hangup:     return SIGHUP;
        case SignalType::User1:      return SIGUSR1;
        case SignalType::User2:      return SIGUSR2;
        case SignalType::Pipe:       return SIGPIPE;
        case SignalType::Alarm:      return SIGALRM;
        case SignalType::Child:      return SIGCHLD;
#endif
        default: return 0;
    }
}

/**
 * @brief Convert native signal number to SignalType
 */
inline SignalType native_to_signal(int sig) {
    switch (sig) {
        case SIGINT:  return SignalType::Interrupt;
        case SIGTERM: return SignalType::Terminate;
#ifndef _WIN32
        case SIGHUP:  return SignalType::Hangup;
        case SIGUSR1: return SignalType::User1;
        case SIGUSR2: return SignalType::User2;
        case SIGPIPE: return SignalType::Pipe;
        case SIGALRM: return SignalType::Alarm;
        case SIGCHLD: return SignalType::Child;
#endif
        default: return SignalType::Interrupt;
    }
}

/**
 * @brief Thread-safe signal handler singleton
 */
class SignalHandler {
public:
    using Handler = std::function<void()>;
    using SignalCallback = std::function<void(SignalType)>;
    
    /**
     * @brief Get the singleton instance
     */
    static SignalHandler& instance() {
        static SignalHandler handler;
        return handler;
    }
    
    /**
     * @brief Check if shutdown has been requested
     */
    bool should_shutdown() const {
        return shutdown_requested_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Request shutdown
     */
    void request_shutdown() {
        shutdown_requested_.store(true, std::memory_order_release);
    }
    
    /**
     * @brief Reset shutdown flag
     */
    void reset_shutdown() {
        shutdown_requested_.store(false, std::memory_order_release);
    }
    
    /**
     * @brief Get the last received signal
     */
    SignalType last_signal() const {
        return last_signal_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get the count of signals received
     */
    int signal_count() const {
        return signal_count_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Register handler for shutdown signals (SIGINT, SIGTERM)
     */
    void register_shutdown_handler(Handler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_handlers_.push_back(std::move(handler));
        
        install_signal_handler(SignalType::Interrupt);
        install_signal_handler(SignalType::Terminate);
    }
    
    /**
     * @brief Register handler for specific signal
     */
    void register_handler(SignalType signal, Handler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        signal_handlers_[signal].push_back(std::move(handler));
        install_signal_handler(signal);
    }
    
    /**
     * @brief Register callback for any signal
     */
    void register_callback(SignalCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.push_back(std::move(callback));
    }
    
    /**
     * @brief Clear all handlers
     */
    void clear_handlers() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_handlers_.clear();
        signal_handlers_.clear();
        callbacks_.clear();
    }
    
    /**
     * @brief Ignore a specific signal
     */
    void ignore_signal(SignalType signal) {
        std::signal(signal_to_native(signal), SIG_IGN);
    }
    
    /**
     * @brief Restore default handler for a signal
     */
    void restore_default(SignalType signal) {
        std::signal(signal_to_native(signal), SIG_DFL);
    }

private:
    SignalHandler() = default;
    ~SignalHandler() = default;
    
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;
    
    std::atomic<bool> shutdown_requested_{false};
    std::atomic<SignalType> last_signal_{SignalType::Interrupt};
    std::atomic<int> signal_count_{0};
    
    std::mutex mutex_;
    std::vector<Handler> shutdown_handlers_;
    std::unordered_map<SignalType, std::vector<Handler>> signal_handlers_;
    std::vector<SignalCallback> callbacks_;
    
    void install_signal_handler(SignalType signal) {
        std::signal(signal_to_native(signal), &SignalHandler::signal_dispatcher);
    }
    
    static void signal_dispatcher(int sig) {
        SignalHandler& self = instance();
        SignalType signal_type = native_to_signal(sig);
        
        self.last_signal_.store(signal_type, std::memory_order_release);
        self.signal_count_.fetch_add(1, std::memory_order_relaxed);
        
        // For shutdown signals, set the flag
        if (signal_type == SignalType::Interrupt || 
            signal_type == SignalType::Terminate) {
            self.shutdown_requested_.store(true, std::memory_order_release);
        }
        
        // Note: Signal handlers should be async-signal-safe
        // The actual handler execution is deferred
    }

public:
    /**
     * @brief Process pending signal handlers (call from main thread)
     */
    void process_handlers() {
        if (signal_count_.load(std::memory_order_acquire) == 0) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        SignalType sig = last_signal_.load(std::memory_order_acquire);
        
        // Execute callbacks
        for (const auto& callback : callbacks_) {
            callback(sig);
        }
        
        // Execute signal-specific handlers
        auto it = signal_handlers_.find(sig);
        if (it != signal_handlers_.end()) {
            for (const auto& handler : it->second) {
                handler();
            }
        }
        
        // Execute shutdown handlers for shutdown signals
        if (sig == SignalType::Interrupt || sig == SignalType::Terminate) {
            for (const auto& handler : shutdown_handlers_) {
                handler();
            }
        }
    }
};

/**
 * @brief RAII signal blocking guard
 * 
 * Blocks specified signals during its lifetime, restores on destruction.
 */
class SignalGuard {
public:
    explicit SignalGuard(std::initializer_list<SignalType> signals) {
#ifndef _WIN32
        sigemptyset(&blocked_set_);
        for (SignalType sig : signals) {
            sigaddset(&blocked_set_, signal_to_native(sig));
        }
        sigprocmask(SIG_BLOCK, &blocked_set_, &old_set_);
        active_ = true;
#else
        (void)signals;  // Not supported on Windows
        active_ = false;
#endif
    }
    
    ~SignalGuard() {
        restore();
    }
    
    SignalGuard(const SignalGuard&) = delete;
    SignalGuard& operator=(const SignalGuard&) = delete;
    
    SignalGuard(SignalGuard&& other) noexcept 
#ifndef _WIN32
        : blocked_set_(other.blocked_set_), old_set_(other.old_set_), active_(other.active_) 
#else
        : active_(false)
#endif
    {
        other.active_ = false;
    }
    
    SignalGuard& operator=(SignalGuard&& other) noexcept {
        if (this != &other) {
            restore();
#ifndef _WIN32
            blocked_set_ = other.blocked_set_;
            old_set_ = other.old_set_;
#endif
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }
    
    void restore() {
#ifndef _WIN32
        if (active_) {
            sigprocmask(SIG_SETMASK, &old_set_, nullptr);
            active_ = false;
        }
#endif
    }

private:
#ifndef _WIN32
    sigset_t blocked_set_;
    sigset_t old_set_;
#endif
    bool active_;
};

/**
 * @brief Convenience function to wait for shutdown
 */
inline void wait_for_shutdown() {
    while (!SignalHandler::instance().should_shutdown()) {
#if defined(_WIN32) || defined(_WIN64)
        Sleep(100);
#else
        usleep(100000);
#endif
    }
}

/**
 * @brief Convenience function to check and process shutdown
 */
inline bool check_shutdown() {
    SignalHandler::instance().process_handlers();
    return SignalHandler::instance().should_shutdown();
}

}  // namespace ims::common

#endif  // IMS_COMMON_SIGNAL_HANDLER_HPP
