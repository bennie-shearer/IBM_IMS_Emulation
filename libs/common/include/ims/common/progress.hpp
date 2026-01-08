/**
 * @file progress.hpp
 * @brief Progress tracking for long-running operations
 * @version 3.6.2
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_PROGRESS_HPP
#define IMS_COMMON_PROGRESS_HPP

#include "types.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace ims::common {

/**
 * @brief Progress information passed to callbacks
 */
struct ProgressInfo {
    Size current;           // Current progress value
    Size total;             // Total expected value
    double percentage;      // Percentage complete (0-100)
    double eta_seconds;     // Estimated time remaining
    double elapsed_seconds; // Time elapsed since start
    double rate;            // Items per second
    String message;         // Optional status message
};

/**
 * @brief Thread-safe progress tracker
 */
class ProgressTracker {
public:
    using Callback = std::function<void(const ProgressInfo&)>;
    using Clock = std::chrono::steady_clock;
    
    /**
     * @brief Create tracker with total item count
     */
    explicit ProgressTracker(Size total = 100)
        : total_(total), current_(0), start_time_(Clock::now()) {}
    
    /**
     * @brief Set the total expected items
     */
    void set_total(Size total) {
        total_.store(total, std::memory_order_release);
    }
    
    /**
     * @brief Get current total
     */
    Size total() const {
        return total_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get current progress
     */
    Size current() const {
        return current_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Set progress callback
     */
    void set_callback(Callback callback, Size notify_interval = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(callback);
        notify_interval_ = notify_interval > 0 ? notify_interval : 1;
    }
    
    /**
     * @brief Set status message
     */
    void set_message(String message) {
        std::lock_guard<std::mutex> lock(mutex_);
        message_ = std::move(message);
    }
    
    /**
     * @brief Increment progress by one
     */
    void increment() {
        advance(1);
    }
    
    /**
     * @brief Advance progress by amount
     */
    void advance(Size amount) {
        Size new_value = current_.fetch_add(amount, std::memory_order_relaxed) + amount;
        maybe_notify(new_value);
    }
    
    /**
     * @brief Set absolute progress value
     */
    void set_progress(Size value) {
        current_.store(value, std::memory_order_release);
        maybe_notify(value);
    }
    
    /**
     * @brief Get current percentage (0-100)
     */
    double percentage() const {
        Size t = total_.load(std::memory_order_acquire);
        if (t == 0) return 0.0;
        Size c = current_.load(std::memory_order_acquire);
        return (static_cast<double>(c) / static_cast<double>(t)) * 100.0;
    }
    
    /**
     * @brief Check if complete
     */
    bool is_complete() const {
        return current_.load(std::memory_order_acquire) >= 
               total_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get elapsed time in seconds
     */
    double elapsed_seconds() const {
        auto now = Clock::now();
        return std::chrono::duration<double>(now - start_time_).count();
    }
    
    /**
     * @brief Get estimated time remaining in seconds
     */
    double eta_seconds() const {
        Size c = current_.load(std::memory_order_acquire);
        Size t = total_.load(std::memory_order_acquire);
        
        if (c == 0 || c >= t) return 0.0;
        
        double elapsed = elapsed_seconds();
        double rate = static_cast<double>(c) / elapsed;
        Size remaining = t - c;
        
        return static_cast<double>(remaining) / rate;
    }
    
    /**
     * @brief Get items per second rate
     */
    double rate() const {
        Size c = current_.load(std::memory_order_acquire);
        double elapsed = elapsed_seconds();
        
        if (elapsed < 0.001) return 0.0;
        return static_cast<double>(c) / elapsed;
    }
    
    /**
     * @brief Get current progress info
     */
    ProgressInfo info() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        ProgressInfo pi;
        pi.current = current_.load(std::memory_order_acquire);
        pi.total = total_.load(std::memory_order_acquire);
        pi.percentage = percentage();
        pi.eta_seconds = eta_seconds();
        pi.elapsed_seconds = elapsed_seconds();
        pi.rate = rate();
        pi.message = message_;
        
        return pi;
    }
    
    /**
     * @brief Reset progress tracker
     */
    void reset(Size new_total = 0) {
        current_.store(0, std::memory_order_release);
        if (new_total > 0) {
            total_.store(new_total, std::memory_order_release);
        }
        start_time_ = Clock::now();
        last_notify_ = 0;
    }

private:
    std::atomic<Size> total_;
    std::atomic<Size> current_;
    Clock::time_point start_time_;
    
    mutable std::mutex mutex_;
    Callback callback_;
    Size notify_interval_ = 1;
    Size last_notify_ = 0;
    String message_;
    
    void maybe_notify(Size current_value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!callback_) return;
        
        if (current_value - last_notify_ >= notify_interval_ ||
            current_value >= total_.load(std::memory_order_acquire)) {
            last_notify_ = current_value;
            
            ProgressInfo pi;
            pi.current = current_value;
            pi.total = total_.load(std::memory_order_acquire);
            pi.percentage = (pi.total > 0) ? 
                (static_cast<double>(pi.current) / pi.total * 100.0) : 0.0;
            pi.elapsed_seconds = elapsed_seconds();
            pi.rate = (pi.elapsed_seconds > 0.001) ? 
                (static_cast<double>(pi.current) / pi.elapsed_seconds) : 0.0;
            pi.eta_seconds = (pi.rate > 0 && pi.current < pi.total) ?
                (static_cast<double>(pi.total - pi.current) / pi.rate) : 0.0;
            pi.message = message_;
            
            callback_(pi);
        }
    }
};

/**
 * @brief Console progress bar
 */
class ProgressBar {
public:
    explicit ProgressBar(Size total, Size width = 50, std::ostream& out = std::cout)
        : tracker_(total), width_(width), out_(out), last_len_(0) {}
    
    void increment() {
        tracker_.increment();
        render();
    }
    
    void advance(Size amount) {
        tracker_.advance(amount);
        render();
    }
    
    void set_progress(Size value) {
        tracker_.set_progress(value);
        render();
    }
    
    void set_message(const String& msg) {
        message_ = msg;
        render();
    }
    
    void finish() {
        tracker_.set_progress(tracker_.total());
        render();
        out_ << std::endl;
    }
    
    ProgressTracker& tracker() { return tracker_; }

private:
    ProgressTracker tracker_;
    Size width_;
    std::ostream& out_;
    String message_;
    Size last_len_;
    
    void render() {
        double pct = tracker_.percentage();
        Size filled = static_cast<Size>(pct / 100.0 * width_);
        
        std::ostringstream oss;
        oss << "\r[";
        for (Size i = 0; i < width_; ++i) {
            if (i < filled) oss << '=';
            else if (i == filled) oss << '>';
            else oss << ' ';
        }
        oss << "] ";
        
        oss << std::fixed << std::setprecision(1) << pct << "% ";
        oss << "(" << tracker_.current() << "/" << tracker_.total() << ") ";
        
        double eta = tracker_.eta_seconds();
        if (eta > 0 && !tracker_.is_complete()) {
            oss << "ETA: ";
            if (eta >= 3600) {
                oss << static_cast<int>(eta / 3600) << "h ";
                eta = std::fmod(eta, 3600.0);
            }
            if (eta >= 60) {
                oss << static_cast<int>(eta / 60) << "m ";
                eta = std::fmod(eta, 60.0);
            }
            oss << static_cast<int>(eta) << "s ";
        }
        
        if (!message_.empty()) {
            oss << "- " << message_;
        }
        
        String output = oss.str();
        
        // Pad with spaces to clear previous longer output
        if (output.length() < last_len_) {
            output.append(last_len_ - output.length(), ' ');
        }
        last_len_ = output.length();
        
        out_ << output << std::flush;
    }
};

/**
 * @brief RAII progress scope for automatic increment
 */
class ProgressScope {
public:
    explicit ProgressScope(ProgressTracker& tracker, Size increment = 1)
        : tracker_(tracker), increment_(increment), committed_(false) {}
    
    ~ProgressScope() {
        if (!committed_) {
            tracker_.advance(increment_);
        }
    }
    
    ProgressScope(const ProgressScope&) = delete;
    ProgressScope& operator=(const ProgressScope&) = delete;
    
    void commit() {
        if (!committed_) {
            tracker_.advance(increment_);
            committed_ = true;
        }
    }
    
    void cancel() {
        committed_ = true;  // Don't increment on destruction
    }

private:
    ProgressTracker& tracker_;
    Size increment_;
    bool committed_;
};

/**
 * @brief Simple spinner for indeterminate progress
 */
class Spinner {
public:
    explicit Spinner(const String& message = "", std::ostream& out = std::cout)
        : message_(message), out_(out), frame_(0) {}
    
    void spin() {
        static const char frames[] = {'|', '/', '-', '\\'};
        out_ << "\r" << frames[frame_ % 4];
        if (!message_.empty()) {
            out_ << " " << message_;
        }
        out_ << std::flush;
        ++frame_;
    }
    
    void done(const String& final_msg = "Done") {
        out_ << "\r" << final_msg;
        if (!message_.empty()) {
            out_ << " " << message_;
        }
        out_ << std::endl;
    }

private:
    String message_;
    std::ostream& out_;
    Size frame_;
};

}  // namespace ims::common

#endif  // IMS_COMMON_PROGRESS_HPP
