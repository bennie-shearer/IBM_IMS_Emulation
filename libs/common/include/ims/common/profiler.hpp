/**
 * @file profiler.hpp
 * @brief Lightweight performance profiling
 * @version 3.6.2
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_PROFILER_HPP
#define IMS_COMMON_PROFILER_HPP

#include "types.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

namespace ims::common {

/**
 * @brief Statistics for a profiled region
 */
struct ProfileStats {
    String name;
    Size call_count = 0;
    UInt64 total_ns = 0;
    UInt64 min_ns = UINT64_MAX;
    UInt64 max_ns = 0;
    
    double total_ms() const { return static_cast<double>(total_ns) / 1000000.0; }
    double avg_ms() const { 
        return call_count > 0 ? total_ms() / static_cast<double>(call_count) : 0.0; 
    }
    double min_ms() const { return static_cast<double>(min_ns) / 1000000.0; }
    double max_ms() const { return static_cast<double>(max_ns) / 1000000.0; }
    double avg_us() const {
        return call_count > 0 ? 
            static_cast<double>(total_ns) / (1000.0 * static_cast<double>(call_count)) : 0.0;
    }
};

/**
 * @brief Thread-safe profiler singleton
 */
class Profiler {
public:
    using Clock = std::chrono::high_resolution_clock;
    
    /**
     * @brief Get the global profiler instance
     */
    static Profiler& instance() {
        static Profiler profiler;
        return profiler;
    }
    
    /**
     * @brief Enable or disable profiling
     */
    void set_enabled(bool enabled) {
        enabled_.store(enabled, std::memory_order_release);
    }
    
    /**
     * @brief Check if profiling is enabled
     */
    bool is_enabled() const {
        return enabled_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Record a timing measurement
     */
    void record(const String& name, UInt64 duration_ns) {
        if (!is_enabled()) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& stats = stats_[name];
        stats.name = name;
        stats.call_count++;
        stats.total_ns += duration_ns;
        if (duration_ns < stats.min_ns) stats.min_ns = duration_ns;
        if (duration_ns > stats.max_ns) stats.max_ns = duration_ns;
    }
    
    /**
     * @brief Get statistics for a specific region
     */
    std::optional<ProfileStats> get_stats(const String& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = stats_.find(name);
        if (it != stats_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    /**
     * @brief Get all statistics
     */
    std::vector<ProfileStats> get_all_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<ProfileStats> result;
        result.reserve(stats_.size());
        
        for (const auto& [name, stats] : stats_) {
            result.push_back(stats);
        }
        
        return result;
    }
    
    /**
     * @brief Get statistics sorted by total time
     */
    std::vector<ProfileStats> get_stats_by_total_time() const {
        auto stats = get_all_stats();
        std::sort(stats.begin(), stats.end(), [](const auto& a, const auto& b) {
            return a.total_ns > b.total_ns;
        });
        return stats;
    }
    
    /**
     * @brief Get statistics sorted by call count
     */
    std::vector<ProfileStats> get_stats_by_call_count() const {
        auto stats = get_all_stats();
        std::sort(stats.begin(), stats.end(), [](const auto& a, const auto& b) {
            return a.call_count > b.call_count;
        });
        return stats;
    }
    
    /**
     * @brief Clear all statistics
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.clear();
    }
    
    /**
     * @brief Generate a formatted report
     */
    String report() const {
        auto stats = get_stats_by_total_time();
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        
        oss << "\n";
        oss << "=== Profile Report ===\n";
        oss << std::left << std::setw(40) << "Name"
            << std::right << std::setw(10) << "Calls"
            << std::setw(15) << "Total (ms)"
            << std::setw(12) << "Avg (us)"
            << std::setw(12) << "Min (us)"
            << std::setw(12) << "Max (us)"
            << "\n";
        oss << String(101, '-') << "\n";
        
        for (const auto& s : stats) {
            oss << std::left << std::setw(40) << s.name
                << std::right << std::setw(10) << s.call_count
                << std::setw(15) << s.total_ms()
                << std::setw(12) << s.avg_us()
                << std::setw(12) << (static_cast<double>(s.min_ns) / 1000.0)
                << std::setw(12) << (static_cast<double>(s.max_ns) / 1000.0)
                << "\n";
        }
        
        oss << String(101, '-') << "\n";
        
        return oss.str();
    }
    
    /**
     * @brief Print report to stream
     */
    void print_report(std::ostream& out = std::cout) const {
        out << report();
    }

private:
    Profiler() : enabled_(true) {}
    
    mutable std::mutex mutex_;
    std::map<String, ProfileStats> stats_;
    std::atomic<bool> enabled_;
};

/**
 * @brief RAII timing scope
 */
class ProfileScope {
public:
    explicit ProfileScope(String name)
        : name_(std::move(name))
        , start_(Profiler::Clock::now())
        , stopped_(false) {}
    
    ~ProfileScope() {
        stop();
    }
    
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
    
    ProfileScope(ProfileScope&& other) noexcept
        : name_(std::move(other.name_))
        , start_(other.start_)
        , stopped_(other.stopped_) {
        other.stopped_ = true;
    }
    
    ProfileScope& operator=(ProfileScope&& other) noexcept {
        if (this != &other) {
            stop();
            name_ = std::move(other.name_);
            start_ = other.start_;
            stopped_ = other.stopped_;
            other.stopped_ = true;
        }
        return *this;
    }
    
    /**
     * @brief Stop timing and record (called automatically on destruction)
     */
    void stop() {
        if (!stopped_) {
            auto end = Profiler::Clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end - start_).count();
            Profiler::instance().record(name_, static_cast<UInt64>(duration));
            stopped_ = true;
        }
    }
    
    /**
     * @brief Cancel timing without recording
     */
    void cancel() {
        stopped_ = true;
    }
    
    /**
     * @brief Get elapsed time so far (in nanoseconds)
     */
    UInt64 elapsed_ns() const {
        auto now = Profiler::Clock::now();
        return static_cast<UInt64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_).count());
    }
    
    /**
     * @brief Get elapsed time so far (in milliseconds)
     */
    double elapsed_ms() const {
        return static_cast<double>(elapsed_ns()) / 1000000.0;
    }

private:
    String name_;
    Profiler::Clock::time_point start_;
    bool stopped_;
};

/**
 * @brief Simple timer without profiler recording
 */
class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    
    Timer() : start_(Clock::now()) {}
    
    void reset() {
        start_ = Clock::now();
    }
    
    UInt64 elapsed_ns() const {
        auto now = Clock::now();
        return static_cast<UInt64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_).count());
    }
    
    double elapsed_us() const {
        return static_cast<double>(elapsed_ns()) / 1000.0;
    }
    
    double elapsed_ms() const {
        return static_cast<double>(elapsed_ns()) / 1000000.0;
    }
    
    double elapsed_s() const {
        return static_cast<double>(elapsed_ns()) / 1000000000.0;
    }

private:
    Clock::time_point start_;
};

/**
 * @brief Measure execution time of a callable
 */
template<typename F>
auto measure(const String& name, F&& func) -> decltype(func()) {
    ProfileScope scope(name);
    return func();
}

/**
 * @brief Measure and return timing with result
 */
template<typename F>
auto measure_with_time(F&& func) -> std::pair<decltype(func()), double> {
    Timer timer;
    auto result = func();
    return {std::move(result), timer.elapsed_ms()};
}

// Macro for easy function profiling
#define PROFILE_FUNCTION() \
    ims::common::ProfileScope _profile_scope_##__LINE__(__FUNCTION__)

#define PROFILE_SCOPE(name) \
    ims::common::ProfileScope _profile_scope_##__LINE__(name)

}  // namespace ims::common

#endif  // IMS_COMMON_PROFILER_HPP
