#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Benchmark Framework
// Version: 3.6.2
// =============================================================================
//
// Provides performance benchmarking utilities for measuring operation
// throughput, latency, and system performance.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include <iomanip>
#include <sstream>
#include <cmath>

namespace ims::benchmark {

// =============================================================================
// Benchmark Result
// =============================================================================

struct BenchmarkResult {
    String name;
    UInt64 iterations{0};
    Duration total_time{0};
    Duration min_time{Duration::max()};
    Duration max_time{Duration::min()};
    Vector<Duration> samples;
    
    double ops_per_second() const {
        if (total_time.count() == 0) return 0.0;
        return static_cast<double>(iterations) / 
               (static_cast<double>(total_time.count()) / 1e9);
    }
    
    double avg_time_ns() const {
        if (iterations == 0) return 0.0;
        return static_cast<double>(total_time.count()) / static_cast<double>(iterations);
    }
    
    double avg_time_us() const { return avg_time_ns() / 1000.0; }
    double avg_time_ms() const { return avg_time_ns() / 1000000.0; }
    
    double stddev_ns() const {
        if (samples.size() < 2) return 0.0;
        double mean = avg_time_ns();
        double sum_sq = 0.0;
        for (const auto& s : samples) {
            double diff = static_cast<double>(s.count()) - mean;
            sum_sq += diff * diff;
        }
        return std::sqrt(sum_sq / static_cast<double>(samples.size() - 1));
    }
    
    Duration median_time() const {
        if (samples.empty()) return Duration{0};
        Vector<Duration> sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        return sorted[sorted.size() / 2];
    }
    
    Duration percentile(double p) const {
        if (samples.empty()) return Duration{0};
        Vector<Duration> sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        Size idx = static_cast<Size>(static_cast<double>(sorted.size() - 1) * p / 100.0);
        return sorted[idx];
    }
    
    String to_string() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Benchmark: " << name << "\n";
        oss << "  Iterations: " << iterations << "\n";
        oss << "  Total Time: " << static_cast<double>(total_time.count()) / 1e6 << " ms\n";
        oss << "  Ops/sec: " << ops_per_second() << "\n";
        oss << "  Avg: " << avg_time_us() << " us\n";
        oss << "  Min: " << static_cast<double>(min_time.count()) / 1000.0 << " us\n";
        oss << "  Max: " << static_cast<double>(max_time.count()) / 1000.0 << " us\n";
        if (!samples.empty()) {
            oss << "  Median: " << static_cast<double>(median_time().count()) / 1000.0 << " us\n";
            oss << "  Stddev: " << stddev_ns() / 1000.0 << " us\n";
            oss << "  P95: " << static_cast<double>(percentile(95).count()) / 1000.0 << " us\n";
            oss << "  P99: " << static_cast<double>(percentile(99).count()) / 1000.0 << " us\n";
        }
        return oss.str();
    }
};

// =============================================================================
// Benchmark Runner
// =============================================================================

class Benchmark {
private:
    String name_;
    UInt64 warmup_iterations_{100};
    UInt64 measure_iterations_{1000};
    bool collect_samples_{true};
    
public:
    explicit Benchmark(String name) : name_(std::move(name)) {}
    
    Benchmark& warmup(UInt64 iterations) {
        warmup_iterations_ = iterations;
        return *this;
    }
    
    Benchmark& iterations(UInt64 iterations) {
        measure_iterations_ = iterations;
        return *this;
    }
    
    Benchmark& samples(bool collect) {
        collect_samples_ = collect;
        return *this;
    }
    
    template<typename Func>
    BenchmarkResult run(Func&& func) {
        BenchmarkResult result;
        result.name = name_;
        
        // Warmup phase
        for (UInt64 i = 0; i < warmup_iterations_; ++i) {
            func();
        }
        
        // Reserve space for samples
        if (collect_samples_) {
            result.samples.reserve(static_cast<Size>(measure_iterations_));
        }
        
        // Measurement phase
        auto total_start = Clock::now();
        
        for (UInt64 i = 0; i < measure_iterations_; ++i) {
            auto start = Clock::now();
            func();
            auto end = Clock::now();
            
            Duration elapsed = std::chrono::duration_cast<Duration>(end - start);
            
            if (elapsed < result.min_time) result.min_time = elapsed;
            if (elapsed > result.max_time) result.max_time = elapsed;
            
            if (collect_samples_) {
                result.samples.push_back(elapsed);
            }
        }
        
        auto total_end = Clock::now();
        result.iterations = measure_iterations_;
        result.total_time = std::chrono::duration_cast<Duration>(total_end - total_start);
        
        return result;
    }
    
    // Run with setup and teardown per iteration
    template<typename Setup, typename Func, typename Teardown>
    BenchmarkResult run_with_setup(Setup&& setup, Func&& func, Teardown&& teardown) {
        BenchmarkResult result;
        result.name = name_;
        
        // Warmup phase
        for (UInt64 i = 0; i < warmup_iterations_; ++i) {
            auto ctx = setup();
            func(ctx);
            teardown(ctx);
        }
        
        if (collect_samples_) {
            result.samples.reserve(static_cast<Size>(measure_iterations_));
        }
        
        // Measurement phase
        auto total_start = Clock::now();
        
        for (UInt64 i = 0; i < measure_iterations_; ++i) {
            auto ctx = setup();
            
            auto start = Clock::now();
            func(ctx);
            auto end = Clock::now();
            
            teardown(ctx);
            
            Duration elapsed = std::chrono::duration_cast<Duration>(end - start);
            
            if (elapsed < result.min_time) result.min_time = elapsed;
            if (elapsed > result.max_time) result.max_time = elapsed;
            
            if (collect_samples_) {
                result.samples.push_back(elapsed);
            }
        }
        
        auto total_end = Clock::now();
        result.iterations = measure_iterations_;
        result.total_time = std::chrono::duration_cast<Duration>(total_end - total_start);
        
        return result;
    }
};

// =============================================================================
// Benchmark Suite
// =============================================================================

class BenchmarkSuite {
private:
    String name_;
    Vector<BenchmarkResult> results_;
    
public:
    explicit BenchmarkSuite(String name) : name_(std::move(name)) {}
    
    void add_result(BenchmarkResult result) {
        results_.push_back(std::move(result));
    }
    
    template<typename Func>
    BenchmarkResult& run(const String& name, Func&& func, UInt64 iterations = 1000) {
        BenchmarkResult result = Benchmark(name).iterations(iterations).run(std::forward<Func>(func));
        results_.push_back(std::move(result));
        return results_.back();
    }
    
    const Vector<BenchmarkResult>& results() const { return results_; }
    
    String to_string() const {
        std::ostringstream oss;
        oss << "========================================\n";
        oss << "Benchmark Suite: " << name_ << "\n";
        oss << "========================================\n\n";
        
        for (const auto& result : results_) {
            oss << result.to_string() << "\n";
        }
        
        return oss.str();
    }
    
    // Generate CSV report
    String to_csv() const {
        std::ostringstream oss;
        oss << "Name,Iterations,Total_ms,Ops_per_sec,Avg_us,Min_us,Max_us,Median_us,Stddev_us,P95_us,P99_us\n";
        
        for (const auto& r : results_) {
            oss << std::fixed << std::setprecision(3);
            oss << r.name << ","
                << r.iterations << ","
                << static_cast<double>(r.total_time.count()) / 1e6 << ","
                << r.ops_per_second() << ","
                << r.avg_time_us() << ","
                << static_cast<double>(r.min_time.count()) / 1000.0 << ","
                << static_cast<double>(r.max_time.count()) / 1000.0 << ","
                << static_cast<double>(r.median_time().count()) / 1000.0 << ","
                << r.stddev_ns() / 1000.0 << ","
                << static_cast<double>(r.percentile(95).count()) / 1000.0 << ","
                << static_cast<double>(r.percentile(99).count()) / 1000.0 << "\n";
        }
        
        return oss.str();
    }
};

// =============================================================================
// Convenience Functions
// =============================================================================

template<typename Func>
BenchmarkResult quick_bench(const String& name, Func&& func, UInt64 iterations = 1000) {
    return Benchmark(name).iterations(iterations).run(std::forward<Func>(func));
}

// Time a single operation
template<typename Func>
Duration time_once(Func&& func) {
    auto start = Clock::now();
    func();
    auto end = Clock::now();
    return std::chrono::duration_cast<Duration>(end - start);
}

// =============================================================================
// Throughput Benchmark
// =============================================================================

struct ThroughputResult {
    String name;
    UInt64 total_bytes{0};
    Duration elapsed{0};
    
    double bytes_per_second() const {
        if (elapsed.count() == 0) return 0.0;
        return static_cast<double>(total_bytes) / 
               (static_cast<double>(elapsed.count()) / 1e9);
    }
    
    double mb_per_second() const {
        return bytes_per_second() / (1024.0 * 1024.0);
    }
    
    double gb_per_second() const {
        return bytes_per_second() / (1024.0 * 1024.0 * 1024.0);
    }
    
    String to_string() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Throughput: " << name << "\n";
        oss << "  Total Bytes: " << total_bytes << "\n";
        oss << "  Elapsed: " << static_cast<double>(elapsed.count()) / 1e6 << " ms\n";
        oss << "  Throughput: " << mb_per_second() << " MB/s\n";
        return oss.str();
    }
};

template<typename Func>
ThroughputResult throughput_bench(const String& name, UInt64 bytes_per_op, 
                                   UInt64 iterations, Func&& func) {
    ThroughputResult result;
    result.name = name;
    result.total_bytes = bytes_per_op * iterations;
    
    auto start = Clock::now();
    for (UInt64 i = 0; i < iterations; ++i) {
        func();
    }
    auto end = Clock::now();
    
    result.elapsed = std::chrono::duration_cast<Duration>(end - start);
    return result;
}

} // namespace ims::benchmark
