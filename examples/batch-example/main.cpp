// =============================================================================
// IBM IMS Emulation Enterprise - Batch Operations Example
// Version: 3.6.2
// NEW in v3.6.2: Demonstrates bulk processing capabilities
// =============================================================================

#include "ims/common/types.hpp"
#include "ims/common/error.hpp"
#include "ims/common/batch.hpp"
#include "ims/common/thread_pool.hpp"

#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>

using namespace ims;

void print_separator(const char* title) {
    std::cout << std::endl << "--- " << title << " ---" << std::endl;
}

// Sample data structure (renamed to avoid conflict with ims::DataRecord)
struct SampleRecord {
    Int64 id{0};
    String name;
    Float64 value{0.0};
};

// Sample processing result
struct SampleResult {
    Int64 id{0};
    Float64 computed_value{0.0};
    String status;
};

int main() {
    std::cout << "=== IMS Batch Operations Example (v3.6.2) ===" << std::endl;
    
    // =========================================================================
    // Thread Pool Demo
    // =========================================================================
    print_separator("Thread Pool");
    
    auto& pool = global_thread_pool();
    std::cout << "Worker threads: " << pool.worker_count() << std::endl;
    
    // Submit some tasks
    Vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i]() {
            std::this_thread::sleep_for(Milliseconds(50));
            return i * i;
        }));
    }
    
    std::cout << "Submitted 10 tasks, waiting for results..." << std::endl;
    
    int sum = 0;
    for (auto& f : futures) {
        sum += f.get();
    }
    
    std::cout << "Sum of squares: " << sum << std::endl;
    std::cout << "Completed tasks: " << pool.completed_tasks() << std::endl;
    
    // =========================================================================
    // Sequential Batch Processing
    // =========================================================================
    print_separator("Sequential Batch Processing");
    
    // Create sample data
    Vector<SampleRecord> records;
    for (Int64 i = 1; i <= 20; ++i) {
        SampleRecord rec;
        rec.id = i;
        rec.name = "Record_" + std::to_string(i);
        rec.value = static_cast<Float64>(i * 10);
        records.push_back(rec);
    }
    
    std::cout << "Processing " << records.size() << " records sequentially..." << std::endl;
    
    // Define processor function
    auto processor = [](const SampleRecord& rec) -> ErrorResult<SampleResult> {
        // Simulate some processing
        std::this_thread::sleep_for(Milliseconds(10));
        
        // Simulate occasional failure
        if (rec.id % 7 == 0) {
            return make_error<SampleResult>("Processing failed for ID " + std::to_string(rec.id));
        }
        
        SampleResult result;
        result.id = rec.id;
        result.computed_value = std::sqrt(rec.value) * 3.14159;
        result.status = "PROCESSED";
        return result;
    };
    
    // Process sequentially
    BatchProcessor<SampleRecord, SampleResult> batch_proc(processor);
    auto seq_result = batch_proc.execute(records);
    
    std::cout << "Sequential Results:" << std::endl;
    std::cout << "  Total: " << seq_result.total << std::endl;
    std::cout << "  Succeeded: " << seq_result.succeeded << std::endl;
    std::cout << "  Failed: " << seq_result.failed << std::endl;
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1) 
              << seq_result.success_rate() << "%" << std::endl;
    std::cout << "  Avg Duration: " << std::fixed << std::setprecision(2) 
              << seq_result.avg_duration_ns() / 1000000.0 << " ms" << std::endl;
    
    // Show sample results
    std::cout << std::endl << "Sample Results:" << std::endl;
    for (Size i = 0; i < std::min(Size(5), seq_result.results.size()); ++i) {
        const auto& r = seq_result.results[i];
        std::cout << "  [" << r.index << "] ";
        if (r.is_success()) {
            std::cout << "SUCCESS - value=" << std::fixed << std::setprecision(2) 
                      << r.value->computed_value << std::endl;
        } else {
            std::cout << "FAILED - " << r.error_message << std::endl;
        }
    }
    
    // =========================================================================
    // Parallel Batch Processing
    // =========================================================================
    print_separator("Parallel Batch Processing");
    
    // Create larger dataset
    Vector<SampleRecord> large_records;
    for (Int64 i = 1; i <= 100; ++i) {
        SampleRecord rec;
        rec.id = i;
        rec.name = "LargeRecord_" + std::to_string(i);
        rec.value = static_cast<Float64>(i);
        large_records.push_back(rec);
    }
    
    std::cout << "Processing " << large_records.size() << " records in parallel..." << std::endl;
    
    auto parallel_processor = [](const SampleRecord& rec) -> ErrorResult<SampleResult> {
        // Simulate heavier processing
        std::this_thread::sleep_for(Milliseconds(5));
        
        SampleResult result;
        result.id = rec.id;
        result.computed_value = std::log(rec.value + 1) * static_cast<double>(rec.id);
        result.status = "COMPUTED";
        return result;
    };
    
    // Process in parallel
    ParallelBatchProcessor<SampleRecord, SampleResult> parallel_batch(parallel_processor, 4);
    auto start = std::chrono::steady_clock::now();
    auto par_result = parallel_batch.execute(large_records);
    auto end = std::chrono::steady_clock::now();
    
    auto total_time = std::chrono::duration_cast<Milliseconds>(end - start).count();
    
    std::cout << "Parallel Results:" << std::endl;
    std::cout << "  Total: " << par_result.total << std::endl;
    std::cout << "  Succeeded: " << par_result.succeeded << std::endl;
    std::cout << "  Failed: " << par_result.failed << std::endl;
    std::cout << "  Total Time: " << total_time << " ms" << std::endl;
    std::cout << "  Theoretical Sequential Time: " << large_records.size() * 5 << " ms" << std::endl;
    std::cout << "  Speedup: " << std::fixed << std::setprecision(1) 
              << (static_cast<double>(large_records.size() * 5) / static_cast<double>(total_time)) << "x" << std::endl;
    
    // =========================================================================
    // Batch Processing with Progress Callback
    // =========================================================================
    print_separator("Batch Processing with Progress");
    
    Vector<Int64> numbers;
    for (Int64 i = 1; i <= 50; ++i) {
        numbers.push_back(i);
    }
    
    std::cout << "Processing with progress updates..." << std::endl;
    
    BatchOptions options;
    options.progress_callback = [](Size completed, Size total) {
        int percent = static_cast<int>((completed * 100) / total);
        std::cout << "\r  Progress: [";
        int bar_width = 30;
        int pos = bar_width * percent / 100;
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << percent << "% (" << completed << "/" << total << ")" << std::flush;
    };
    options.continue_on_error = true;
    
    auto number_processor = [](const Int64& n) -> ErrorResult<Int64> {
        std::this_thread::sleep_for(Milliseconds(20));
        return n * n;
    };
    
    BatchProcessor<Int64, Int64> progress_batch(number_processor, options);
    auto progress_result = progress_batch.execute(numbers);
    
    std::cout << std::endl << "  Complete!" << std::endl;
    std::cout << "  Results: " << progress_result.succeeded << " succeeded" << std::endl;
    
    // =========================================================================
    // Convenience Function
    // =========================================================================
    print_separator("Convenience Function");
    
    Vector<Float64> floats = {1.0, 4.0, 9.0, 16.0, 25.0, 36.0, 49.0, 64.0, 81.0, 100.0};
    
    auto sqrt_processor = [](const Float64& x) -> ErrorResult<Float64> {
        if (x < 0) return make_error<Float64>("Cannot compute sqrt of negative number");
        return std::sqrt(x);
    };
    
    std::cout << "Computing square roots..." << std::endl;
    auto sqrt_results = batch_process<Float64, Float64>(floats, sqrt_processor, false);
    
    std::cout << "Results:" << std::endl;
    for (const auto& r : sqrt_results.results) {
        if (r.is_success()) {
            std::cout << "  sqrt(" << floats[r.index] << ") = " 
                      << std::fixed << std::setprecision(1) << *r.value << std::endl;
        }
    }
    
    // =========================================================================
    // Parallel For
    // =========================================================================
    print_separator("Parallel For Loop");
    
    std::atomic<Int64> sum_atomic{0};
    
    std::cout << "Parallel sum of 1 to 1000..." << std::endl;
    
    parallel_for_chunked(Size(1), Size(1001), [&sum_atomic](Size i) {
        sum_atomic += static_cast<Int64>(i);
    });
    
    std::cout << "  Sum: " << sum_atomic.load() << std::endl;
    std::cout << "  Expected: " << (1000 * 1001 / 2) << std::endl;
    
    // =========================================================================
    // Batch Result Analysis
    // =========================================================================
    print_separator("Batch Result Analysis");
    
    std::cout << "Analysis of sequential batch results:" << std::endl;
    
    if (seq_result.has_failures()) {
        std::cout << "  Failed items:" << std::endl;
        for (const auto& r : seq_result.results) {
            if (r.is_failed()) {
                std::cout << "    Index " << r.index << ": " << r.error_message << std::endl;
            }
        }
    }
    
    if (seq_result.all_succeeded()) {
        std::cout << "  All items processed successfully!" << std::endl;
    } else {
        std::cout << "  " << seq_result.failed << " failures, " 
                  << seq_result.skipped << " skipped" << std::endl;
    }
    
    std::cout << std::endl << "=== Batch Operations Example Complete ===" << std::endl;
    
    return 0;
}
