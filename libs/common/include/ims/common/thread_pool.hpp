#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Thread Pool
// Version: 3.6.3
// NEW in v3.6.3: Concurrent operation support
// =============================================================================

#include "types.hpp"
#include <queue>

namespace ims {

// =============================================================================
// Thread Pool Implementation
// =============================================================================

class ThreadPool {
public:
    using Task = Function<void()>;
    
private:
    Vector<std::thread> workers_;
    std::queue<Task> tasks_;
    
    mutable Mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    std::atomic<Size> active_tasks_{0};
    std::atomic<Size> completed_tasks_{0};
    
    void worker_thread() {
        while (true) {
            Task task;
            {
                std::unique_lock lock(queue_mutex_);
                condition_.wait(lock, [this] {
                    return stop_.load() || !tasks_.empty();
                });
                
                if (stop_.load() && tasks_.empty()) {
                    return;
                }
                
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            
            ++active_tasks_;
            task();
            --active_tasks_;
            ++completed_tasks_;
        }
    }
    
public:
    explicit ThreadPool(Size num_threads = 0) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 4;
        }
        
        workers_.reserve(num_threads);
        for (Size i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_thread(); });
        }
    }
    
    ~ThreadPool() {
        shutdown();
    }
    
    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    // Submit a task and get a future
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock lock(queue_mutex_);
            if (stop_.load()) {
                throw std::runtime_error("Cannot submit task to stopped thread pool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    // Submit without caring about result
    void execute(Task task) {
        {
            std::unique_lock lock(queue_mutex_);
            if (stop_.load()) return;
            tasks_.emplace(std::move(task));
        }
        condition_.notify_one();
    }
    
    // Graceful shutdown
    void shutdown() {
        {
            std::unique_lock lock(queue_mutex_);
            if (stop_.load()) return;
            stop_ = true;
        }
        
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers_.clear();
    }
    
    // Wait for all tasks to complete
    void wait() {
        while (active_tasks_.load() > 0 || !empty()) {
            std::this_thread::yield();
        }
    }
    
    // Statistics
    Size worker_count() const { return workers_.size(); }
    Size pending_tasks() const {
        std::unique_lock lock(queue_mutex_);
        return tasks_.size();
    }
    Size active_tasks() const { return active_tasks_.load(); }
    Size completed_tasks() const { return completed_tasks_.load(); }
    bool empty() const {
        std::unique_lock lock(queue_mutex_);
        return tasks_.empty();
    }
    bool is_stopped() const { return stop_.load(); }
};

// =============================================================================
// Global Thread Pool
// =============================================================================

inline ThreadPool& global_thread_pool() {
    static ThreadPool instance;
    return instance;
}

// =============================================================================
// Parallel For Implementation
// =============================================================================

template<typename Iterator, typename Func>
void parallel_for(Iterator begin, Iterator end, Func&& func, ThreadPool& pool = global_thread_pool()) {
    Vector<std::future<void>> futures;
    
    for (auto it = begin; it != end; ++it) {
        futures.push_back(pool.submit([&func, it]() {
            func(*it);
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
}

template<typename Func>
void parallel_for(Size start, Size end, Func&& func, ThreadPool& pool = global_thread_pool()) {
    Vector<std::future<void>> futures;
    
    for (Size i = start; i < end; ++i) {
        futures.push_back(pool.submit([&func, i]() {
            func(i);
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
}

// Chunked parallel for (better for large ranges)
template<typename Func>
void parallel_for_chunked(Size start, Size end, Func&& func, 
                          Size chunk_size = 0, ThreadPool& pool = global_thread_pool()) {
    Size count = end - start;
    if (chunk_size == 0) {
        chunk_size = std::max(Size(1), count / (pool.worker_count() * 4));
    }
    
    Vector<std::future<void>> futures;
    
    for (Size chunk_start = start; chunk_start < end; chunk_start += chunk_size) {
        Size chunk_end = std::min(chunk_start + chunk_size, end);
        futures.push_back(pool.submit([&func, chunk_start, chunk_end]() {
            for (Size i = chunk_start; i < chunk_end; ++i) {
                func(i);
            }
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
}

} // namespace ims
