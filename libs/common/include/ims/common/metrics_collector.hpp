/**
 * @file metrics_collector.hpp
 * @brief Real-time performance metrics collection and aggregation
 * @version 3.6.2
 *
 * Provides metrics collection including:
 * - Real-time performance metrics
 * - Histogram and percentile calculations
 * - Time-series data aggregation
 * - Export to CSV format
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_METRICS_COLLECTOR_HPP
#define IMS_COMMON_METRICS_COLLECTOR_HPP

#include "types.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace ims::common {

// =============================================================================
// Metric Types
// =============================================================================

/**
 * @brief Types of metrics that can be collected
 */
enum class MetricType {
    Counter,      ///< Monotonically increasing value
    Gauge,        ///< Value that can go up or down
    Histogram,    ///< Distribution of values
    Timer         ///< Duration measurements
};

/**
 * @brief Converts metric type to string
 */
inline String metric_type_to_string(MetricType type) {
    switch (type) {
        case MetricType::Counter: return "counter";
        case MetricType::Gauge: return "gauge";
        case MetricType::Histogram: return "histogram";
        case MetricType::Timer: return "timer";
        default: return "unknown";
    }
}

// =============================================================================
// Statistical Summary
// =============================================================================

/**
 * @brief Statistical summary of a metric
 */
struct MetricSummary {
    Size count{0};
    double sum{0.0};
    double min{std::numeric_limits<double>::max()};
    double max{std::numeric_limits<double>::lowest()};
    double mean{0.0};
    double variance{0.0};
    double std_dev{0.0};
    double p50{0.0};   ///< 50th percentile (median)
    double p90{0.0};   ///< 90th percentile
    double p95{0.0};   ///< 95th percentile
    double p99{0.0};   ///< 99th percentile
    double p999{0.0};  ///< 99.9th percentile
    
    String to_string() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << "count=" << count;
        oss << ", sum=" << sum;
        oss << ", min=" << min;
        oss << ", max=" << max;
        oss << ", mean=" << mean;
        oss << ", std_dev=" << std_dev;
        oss << ", p50=" << p50;
        oss << ", p90=" << p90;
        oss << ", p95=" << p95;
        oss << ", p99=" << p99;
        return oss.str();
    }
};

// =============================================================================
// Histogram
// =============================================================================

/**
 * @brief Histogram for tracking value distributions
 */
class Histogram {
private:
    Vector<double> values_;
    mutable Mutex mutex_;
    Size max_samples_{10000};
    bool sorted_{false};
    
    void ensure_sorted() const {
        if (!sorted_) {
            auto& values = const_cast<Vector<double>&>(values_);
            std::sort(values.begin(), values.end());
            const_cast<bool&>(sorted_) = true;
        }
    }
    
public:
    explicit Histogram(Size max_samples = 10000) : max_samples_(max_samples) {
        values_.reserve(max_samples);
    }
    
    void record(double value) {
        LockGuard<Mutex> lock(mutex_);
        if (values_.size() >= max_samples_) {
            // Reservoir sampling for bounded memory
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<Size> dist(0, values_.size());
            Size index = dist(rng);
            if (index < values_.size()) {
                values_[index] = value;
            }
        } else {
            values_.push_back(value);
        }
        sorted_ = false;
    }
    
    double percentile(double p) const {
        LockGuard<Mutex> lock(mutex_);
        if (values_.empty()) return 0.0;
        
        ensure_sorted();
        
        double index = (p / 100.0) * (values_.size() - 1);
        Size lower = static_cast<Size>(std::floor(index));
        Size upper = static_cast<Size>(std::ceil(index));
        
        if (lower == upper || upper >= values_.size()) {
            return values_[lower];
        }
        
        double fraction = index - lower;
        return values_[lower] * (1.0 - fraction) + values_[upper] * fraction;
    }
    
    MetricSummary summary() const {
        LockGuard<Mutex> lock(mutex_);
        MetricSummary result;
        
        if (values_.empty()) return result;
        
        ensure_sorted();
        
        result.count = values_.size();
        result.min = values_.front();
        result.max = values_.back();
        
        // Calculate sum and mean
        result.sum = 0.0;
        for (double v : values_) {
            result.sum += v;
        }
        result.mean = result.sum / result.count;
        
        // Calculate variance
        double sq_sum = 0.0;
        for (double v : values_) {
            double diff = v - result.mean;
            sq_sum += diff * diff;
        }
        result.variance = sq_sum / result.count;
        result.std_dev = std::sqrt(result.variance);
        
        // Calculate percentiles
        auto calc_percentile = [this](double p) {
            double index = (p / 100.0) * (values_.size() - 1);
            Size lower = static_cast<Size>(std::floor(index));
            Size upper = static_cast<Size>(std::ceil(index));
            if (lower == upper || upper >= values_.size()) {
                return values_[lower];
            }
            double fraction = index - lower;
            return values_[lower] * (1.0 - fraction) + values_[upper] * fraction;
        };
        
        result.p50 = calc_percentile(50.0);
        result.p90 = calc_percentile(90.0);
        result.p95 = calc_percentile(95.0);
        result.p99 = calc_percentile(99.0);
        result.p999 = calc_percentile(99.9);
        
        return result;
    }
    
    Size count() const {
        LockGuard<Mutex> lock(mutex_);
        return values_.size();
    }
    
    void reset() {
        LockGuard<Mutex> lock(mutex_);
        values_.clear();
        sorted_ = false;
    }
    
    Vector<double> values() const {
        LockGuard<Mutex> lock(mutex_);
        return values_;
    }
};

// =============================================================================
// Time Series Data Point
// =============================================================================

/**
 * @brief Single data point in a time series
 */
struct TimeSeriesPoint {
    SystemTimePoint timestamp;
    double value;
    
    TimeSeriesPoint(SystemTimePoint ts, double val) 
        : timestamp(ts), value(val) {}
};

// =============================================================================
// Time Series
// =============================================================================

/**
 * @brief Time series data with aggregation support
 */
class TimeSeries {
private:
    std::deque<TimeSeriesPoint> points_;
    mutable Mutex mutex_;
    Seconds retention_period_{3600};  // Default 1 hour
    Size max_points_{3600};           // Default 1 point per second for 1 hour
    
    void cleanup() {
        auto cutoff = SystemClock::now() - retention_period_;
        while (!points_.empty() && points_.front().timestamp < cutoff) {
            points_.pop_front();
        }
        while (points_.size() > max_points_) {
            points_.pop_front();
        }
    }
    
public:
    TimeSeries() = default;
    explicit TimeSeries(Seconds retention, Size max_points = 3600) 
        : retention_period_(retention), max_points_(max_points) {}
    
    void record(double value, SystemTimePoint timestamp = SystemClock::now()) {
        LockGuard<Mutex> lock(mutex_);
        points_.emplace_back(timestamp, value);
        cleanup();
    }
    
    Vector<TimeSeriesPoint> get_points(SystemTimePoint from, SystemTimePoint to) const {
        LockGuard<Mutex> lock(mutex_);
        Vector<TimeSeriesPoint> result;
        for (const auto& point : points_) {
            if (point.timestamp >= from && point.timestamp <= to) {
                result.push_back(point);
            }
        }
        return result;
    }
    
    Vector<TimeSeriesPoint> get_recent(Seconds duration) const {
        auto now = SystemClock::now();
        return get_points(now - duration, now);
    }
    
    /**
     * @brief Aggregates data into buckets
     */
    Vector<std::pair<SystemTimePoint, double>> aggregate(
            Seconds bucket_size, 
            SystemTimePoint from, 
            SystemTimePoint to) const {
        
        LockGuard<Mutex> lock(mutex_);
        
        if (points_.empty()) return {};
        
        Vector<std::pair<SystemTimePoint, double>> result;
        
        auto bucket_start = from;
        double sum = 0.0;
        Size count = 0;
        
        for (const auto& point : points_) {
            if (point.timestamp < from) continue;
            if (point.timestamp > to) break;
            
            while (point.timestamp >= bucket_start + bucket_size) {
                if (count > 0) {
                    result.emplace_back(bucket_start, sum / count);
                }
                bucket_start += bucket_size;
                sum = 0.0;
                count = 0;
            }
            
            sum += point.value;
            ++count;
        }
        
        if (count > 0) {
            result.emplace_back(bucket_start, sum / count);
        }
        
        return result;
    }
    
    Size size() const {
        LockGuard<Mutex> lock(mutex_);
        return points_.size();
    }
    
    void clear() {
        LockGuard<Mutex> lock(mutex_);
        points_.clear();
    }
};

// =============================================================================
// Metric
// =============================================================================

/**
 * @brief Single metric with various tracking capabilities
 */
class Metric {
private:
    String name_;
    String description_;
    MetricType type_;
    Map<String, String> labels_;
    
    std::atomic<double> value_{0.0};
    Histogram histogram_;
    TimeSeries time_series_;
    mutable Mutex mutex_;
    SystemTimePoint created_at_{SystemClock::now()};
    
public:
    Metric(String name, MetricType type, String description = "")
        : name_(std::move(name))
        , description_(std::move(description))
        , type_(type) {}
    
    const String& name() const { return name_; }
    const String& description() const { return description_; }
    MetricType type() const { return type_; }
    const Map<String, String>& labels() const { return labels_; }
    
    void set_label(const String& key, const String& value) {
        LockGuard<Mutex> lock(mutex_);
        labels_[key] = value;
    }
    
    // Counter operations
    void increment(double amount = 1.0) {
        double old_val = value_.load();
        while (!value_.compare_exchange_weak(old_val, old_val + amount)) {}
        
        if (type_ == MetricType::Histogram || type_ == MetricType::Timer) {
            histogram_.record(amount);
        }
        time_series_.record(old_val + amount);
    }
    
    // Gauge operations
    void set(double value) {
        value_.store(value);
        time_series_.record(value);
    }
    
    void add(double delta) {
        double old_val = value_.load();
        while (!value_.compare_exchange_weak(old_val, old_val + delta)) {}
        time_series_.record(old_val + delta);
    }
    
    void subtract(double delta) {
        add(-delta);
    }
    
    // Histogram/Timer operations
    void observe(double value) {
        histogram_.record(value);
        time_series_.record(value);
    }
    
    // Get current value
    double value() const { return value_.load(); }
    
    // Get histogram summary
    MetricSummary summary() const { return histogram_.summary(); }
    
    // Get time series data
    const TimeSeries& time_series() const { return time_series_; }
    
    // Get histogram
    const Histogram& histogram() const { return histogram_; }
    
    void reset() {
        value_.store(0.0);
        histogram_.reset();
        time_series_.clear();
    }
};

// =============================================================================
// Scoped Timer for Metrics
// =============================================================================

/**
 * @brief RAII timer that records duration to a metric
 */
class ScopedMetricTimer {
private:
    Metric& metric_;
    TimePoint start_;
    bool stopped_{false};
    
public:
    explicit ScopedMetricTimer(Metric& metric) 
        : metric_(metric), start_(Clock::now()) {}
    
    ~ScopedMetricTimer() {
        stop();
    }
    
    void stop() {
        if (!stopped_) {
            auto duration = std::chrono::duration_cast<Microseconds>(Clock::now() - start_);
            metric_.observe(static_cast<double>(duration.count()) / 1000.0);  // Record as ms
            stopped_ = true;
        }
    }
    
    double elapsed_ms() const {
        auto duration = std::chrono::duration_cast<Microseconds>(Clock::now() - start_);
        return static_cast<double>(duration.count()) / 1000.0;
    }
};

// =============================================================================
// Metrics Registry
// =============================================================================

/**
 * @brief Central registry for all metrics
 */
class MetricsRegistry {
private:
    HashMap<String, UniquePtr<Metric>> metrics_;
    mutable SharedMutex mutex_;
    String prefix_;
    
    String make_key(const String& name) const {
        return prefix_.empty() ? name : prefix_ + "." + name;
    }
    
public:
    MetricsRegistry() = default;
    explicit MetricsRegistry(String prefix) : prefix_(std::move(prefix)) {}
    
    /**
     * @brief Gets or creates a counter metric
     */
    Metric& counter(const String& name, const String& description = "") {
        String key = make_key(name);
        
        {
            SharedLock<SharedMutex> lock(mutex_);
            auto it = metrics_.find(key);
            if (it != metrics_.end()) {
                return *it->second;
            }
        }
        
        LockGuard<SharedMutex> lock(mutex_);
        auto it = metrics_.find(key);
        if (it != metrics_.end()) {
            return *it->second;
        }
        
        auto metric = std::make_unique<Metric>(key, MetricType::Counter, description);
        auto& ref = *metric;
        metrics_[key] = std::move(metric);
        return ref;
    }
    
    /**
     * @brief Gets or creates a gauge metric
     */
    Metric& gauge(const String& name, const String& description = "") {
        String key = make_key(name);
        
        {
            SharedLock<SharedMutex> lock(mutex_);
            auto it = metrics_.find(key);
            if (it != metrics_.end()) {
                return *it->second;
            }
        }
        
        LockGuard<SharedMutex> lock(mutex_);
        auto it = metrics_.find(key);
        if (it != metrics_.end()) {
            return *it->second;
        }
        
        auto metric = std::make_unique<Metric>(key, MetricType::Gauge, description);
        auto& ref = *metric;
        metrics_[key] = std::move(metric);
        return ref;
    }
    
    /**
     * @brief Gets or creates a histogram metric
     */
    Metric& histogram(const String& name, const String& description = "") {
        String key = make_key(name);
        
        {
            SharedLock<SharedMutex> lock(mutex_);
            auto it = metrics_.find(key);
            if (it != metrics_.end()) {
                return *it->second;
            }
        }
        
        LockGuard<SharedMutex> lock(mutex_);
        auto it = metrics_.find(key);
        if (it != metrics_.end()) {
            return *it->second;
        }
        
        auto metric = std::make_unique<Metric>(key, MetricType::Histogram, description);
        auto& ref = *metric;
        metrics_[key] = std::move(metric);
        return ref;
    }
    
    /**
     * @brief Gets or creates a timer metric
     */
    Metric& timer(const String& name, const String& description = "") {
        String key = make_key(name);
        
        {
            SharedLock<SharedMutex> lock(mutex_);
            auto it = metrics_.find(key);
            if (it != metrics_.end()) {
                return *it->second;
            }
        }
        
        LockGuard<SharedMutex> lock(mutex_);
        auto it = metrics_.find(key);
        if (it != metrics_.end()) {
            return *it->second;
        }
        
        auto metric = std::make_unique<Metric>(key, MetricType::Timer, description);
        auto& ref = *metric;
        metrics_[key] = std::move(metric);
        return ref;
    }
    
    /**
     * @brief Gets a metric by name
     */
    Metric* get(const String& name) {
        String key = make_key(name);
        SharedLock<SharedMutex> lock(mutex_);
        auto it = metrics_.find(key);
        return it != metrics_.end() ? it->second.get() : nullptr;
    }
    
    /**
     * @brief Gets all metric names
     */
    Vector<String> names() const {
        SharedLock<SharedMutex> lock(mutex_);
        Vector<String> result;
        result.reserve(metrics_.size());
        for (const auto& [name, _] : metrics_) {
            result.push_back(name);
        }
        return result;
    }
    
    /**
     * @brief Exports all metrics to CSV format
     */
    String export_csv() const {
        SharedLock<SharedMutex> lock(mutex_);
        std::ostringstream oss;
        
        // Header
        oss << "name,type,value,count,mean,min,max,p50,p90,p95,p99\n";
        
        for (const auto& [name, metric] : metrics_) {
            auto summary = metric->summary();
            oss << name << ","
                << metric_type_to_string(metric->type()) << ","
                << std::fixed << std::setprecision(6)
                << metric->value() << ","
                << summary.count << ","
                << summary.mean << ","
                << summary.min << ","
                << summary.max << ","
                << summary.p50 << ","
                << summary.p90 << ","
                << summary.p95 << ","
                << summary.p99 << "\n";
        }
        
        return oss.str();
    }
    
    /**
     * @brief Exports metrics to a CSV file
     */
    bool export_to_file(const Path& path) const {
        std::ofstream file(path);
        if (!file) return false;
        file << export_csv();
        return file.good();
    }
    
    /**
     * @brief Generates a text report of all metrics
     */
    String report() const {
        SharedLock<SharedMutex> lock(mutex_);
        std::ostringstream oss;
        
        oss << "=== Metrics Report ===\n\n";
        
        for (const auto& [name, metric] : metrics_) {
            oss << name << " (" << metric_type_to_string(metric->type()) << ")\n";
            
            if (!metric->description().empty()) {
                oss << "  Description: " << metric->description() << "\n";
            }
            
            oss << "  Value: " << std::fixed << std::setprecision(3) 
                << metric->value() << "\n";
            
            if (metric->type() == MetricType::Histogram || 
                metric->type() == MetricType::Timer) {
                auto summary = metric->summary();
                oss << "  " << summary.to_string() << "\n";
            }
            
            oss << "\n";
        }
        
        return oss.str();
    }
    
    /**
     * @brief Resets all metrics
     */
    void reset_all() {
        SharedLock<SharedMutex> lock(mutex_);
        for (auto& [_, metric] : metrics_) {
            metric->reset();
        }
    }
    
    /**
     * @brief Clears all metrics from registry
     */
    void clear() {
        LockGuard<SharedMutex> lock(mutex_);
        metrics_.clear();
    }
    
    /**
     * @brief Gets the number of registered metrics
     */
    Size size() const {
        SharedLock<SharedMutex> lock(mutex_);
        return metrics_.size();
    }
};

// =============================================================================
// Global Metrics Registry
// =============================================================================

/**
 * @brief Gets the global metrics registry
 */
inline MetricsRegistry& global_metrics() {
    static MetricsRegistry registry;
    return registry;
}

// =============================================================================
// Convenience Macros
// =============================================================================

#define IMS_COUNTER(name) ims::common::global_metrics().counter(name)
#define IMS_GAUGE(name) ims::common::global_metrics().gauge(name)
#define IMS_HISTOGRAM(name) ims::common::global_metrics().histogram(name)
#define IMS_TIMER(name) ims::common::global_metrics().timer(name)

#define IMS_TIME_SCOPE(name) \
    ims::common::ScopedMetricTimer _timer_##__LINE__(IMS_TIMER(name))

} // namespace ims::common

#endif // IMS_COMMON_METRICS_COLLECTOR_HPP
