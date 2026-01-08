#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Batch Operations
// Version: 3.6.3
// NEW in v3.6.3: Bulk read/write support
// =============================================================================

#include "types.hpp"
#include "error.hpp"
#include "thread_pool.hpp"
#include <chrono>

namespace ims {

// =============================================================================
// Batch Operation Status
// =============================================================================

enum class BatchItemStatus {
    SUCCESS,
    FAILED,
    SKIPPED,
    PENDING
};

inline String batch_status_to_string(BatchItemStatus status) {
    switch (status) {
        case BatchItemStatus::SUCCESS: return "SUCCESS";
        case BatchItemStatus::FAILED: return "FAILED";
        case BatchItemStatus::SKIPPED: return "SKIPPED";
        case BatchItemStatus::PENDING: return "PENDING";
        default: return "UNKNOWN";
    }
}

// =============================================================================
// Batch Operation Result
// =============================================================================

template<typename T>
struct BatchItemResult {
    Size index;
    BatchItemStatus status{BatchItemStatus::PENDING};
    Optional<T> value;
    String error_message;
    UInt64 duration_ns{0};
    
    bool is_success() const { return status == BatchItemStatus::SUCCESS; }
    bool is_failed() const { return status == BatchItemStatus::FAILED; }
};

template<typename T>
struct BatchResult {
    Vector<BatchItemResult<T>> results;
    Size total{0};
    Size succeeded{0};
    Size failed{0};
    Size skipped{0};
    UInt64 total_duration_ns{0};
    
    void add_success(Size index, T value, UInt64 duration = 0) {
        BatchItemResult<T> item;
        item.index = index;
        item.status = BatchItemStatus::SUCCESS;
        item.value = std::move(value);
        item.duration_ns = duration;
        results.push_back(std::move(item));
        ++succeeded;
        ++total;
        total_duration_ns += duration;
    }
    
    void add_failure(Size index, const String& error, UInt64 duration = 0) {
        BatchItemResult<T> item;
        item.index = index;
        item.status = BatchItemStatus::FAILED;
        item.error_message = error;
        item.duration_ns = duration;
        results.push_back(std::move(item));
        ++failed;
        ++total;
        total_duration_ns += duration;
    }
    
    void add_skipped(Size index) {
        BatchItemResult<T> item;
        item.index = index;
        item.status = BatchItemStatus::SKIPPED;
        results.push_back(std::move(item));
        ++skipped;
        ++total;
    }
    
    double success_rate() const {
        return total > 0 ? static_cast<double>(succeeded) / static_cast<double>(total) * 100.0 : 0.0;
    }
    
    double avg_duration_ns() const {
        return total > 0 ? static_cast<double>(total_duration_ns) / static_cast<double>(total) : 0.0;
    }
    
    bool all_succeeded() const { return failed == 0 && skipped == 0 && total > 0; }
    bool has_failures() const { return failed > 0; }
};

// =============================================================================
// Batch Options
// =============================================================================

struct BatchOptions {
    Size batch_size{100};
    Size max_parallel{0};  // 0 = auto (based on CPU count)
    bool continue_on_error{true};
    bool parallel{false};
    UInt32 retry_count{0};
    UInt32 retry_delay_ms{100};
    Function<void(Size, Size)> progress_callback;  // (completed, total)
};

// =============================================================================
// Batch Processor (Simple Sequential)
// =============================================================================

template<typename TInput, typename TOutput>
class BatchProcessor {
public:
    using ProcessFunc = Function<ErrorResult<TOutput>(const TInput&)>;
    
private:
    ProcessFunc processor_;
    BatchOptions options_;
    
    ErrorResult<TOutput> process_with_retry(const TInput& input) {
        ErrorResult<TOutput> result = make_error<TOutput>("Not executed");
        
        for (UInt32 attempt = 0; attempt <= options_.retry_count; ++attempt) {
            result = processor_(input);
            
            if (result.has_value()) {
                return result;
            }
            
            if (attempt < options_.retry_count && options_.retry_delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(options_.retry_delay_ms));
            }
        }
        
        return result;
    }
    
public:
    BatchProcessor(ProcessFunc processor, BatchOptions options = {})
        : processor_(std::move(processor)), options_(std::move(options)) {}
    
    BatchResult<TOutput> execute(const Vector<TInput>& inputs) {
        BatchResult<TOutput> results;
        results.results.reserve(inputs.size());
        
        for (Size i = 0; i < inputs.size(); ++i) {
            auto start = std::chrono::steady_clock::now();
            
            auto result = process_with_retry(inputs[i]);
            
            auto end = std::chrono::steady_clock::now();
            UInt64 duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            
            if (result.has_value()) {
                results.add_success(i, std::move(result.value()), duration);
            } else {
                results.add_failure(i, result.error(), duration);
                
                if (!options_.continue_on_error) {
                    // Mark remaining as skipped
                    for (Size j = i + 1; j < inputs.size(); ++j) {
                        results.add_skipped(j);
                    }
                    break;
                }
            }
            
            if (options_.progress_callback) {
                options_.progress_callback(i + 1, inputs.size());
            }
        }
        
        return results;
    }
};

// =============================================================================
// Parallel Batch Processor
// =============================================================================

template<typename TInput, typename TOutput>
class ParallelBatchProcessor {
public:
    using ProcessFunc = Function<ErrorResult<TOutput>(const TInput&)>;
    
private:
    ProcessFunc processor_;
    Size max_threads_;
    
public:
    ParallelBatchProcessor(ProcessFunc processor, Size max_threads = 0)
        : processor_(std::move(processor)), max_threads_(max_threads) {
        if (max_threads_ == 0) {
            max_threads_ = std::thread::hardware_concurrency();
            if (max_threads_ == 0) max_threads_ = 4;
        }
    }
    
    BatchResult<TOutput> execute(const Vector<TInput>& inputs) {
        BatchResult<TOutput> results;
        
        ThreadPool pool(max_threads_);
        
        struct ItemResult {
            Size index{0};
            bool success{false};
            TOutput value;
            String error;
            UInt64 duration_ns{0};
        };
        
        Vector<std::future<ItemResult>> futures;
        futures.reserve(inputs.size());
        
        for (Size i = 0; i < inputs.size(); ++i) {
            futures.push_back(pool.submit([this, i, &inputs]() {
                ItemResult item;
                item.index = i;
                
                auto start = std::chrono::steady_clock::now();
                auto result = processor_(inputs[i]);
                auto end = std::chrono::steady_clock::now();
                
                item.duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                item.success = result.has_value();
                
                if (result.has_value()) {
                    item.value = std::move(result.value());
                } else {
                    item.error = result.error();
                }
                
                return item;
            }));
        }
        
        // Collect results
        for (auto& future : futures) {
            auto item = future.get();
            
            if (item.success) {
                results.add_success(item.index, std::move(item.value), item.duration_ns);
            } else {
                results.add_failure(item.index, item.error, item.duration_ns);
            }
        }
        
        // Sort results by index
        std::sort(results.results.begin(), results.results.end(),
                  [](const auto& a, const auto& b) { return a.index < b.index; });
        
        return results;
    }
};

// =============================================================================
// Convenience Functions
// =============================================================================

template<typename TInput, typename TOutput>
BatchResult<TOutput> batch_process(
    const Vector<TInput>& inputs,
    Function<ErrorResult<TOutput>(const TInput&)> processor,
    bool parallel = false,
    Size max_threads = 0
) {
    if (parallel) {
        ParallelBatchProcessor<TInput, TOutput> batch(processor, max_threads);
        return batch.execute(inputs);
    } else {
        BatchProcessor<TInput, TOutput> batch(processor);
        return batch.execute(inputs);
    }
}

} // namespace ims
