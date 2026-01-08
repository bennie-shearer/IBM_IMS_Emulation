/**
 * @file task_scheduler.hpp
 * @brief Simple task scheduling and execution
 * @version 3.6.3
 *
 * Provides task scheduling including:
 * - Delayed task execution
 * - Periodic task scheduling
 * - Task cancellation
 * - Priority-based scheduling
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_TASK_SCHEDULER_HPP
#define IMS_COMMON_TASK_SCHEDULER_HPP

#include "types.hpp"
#include <queue>
#include <set>

namespace ims::common {

// =============================================================================
// Task Definition
// =============================================================================

/**
 * @brief Unique task identifier
 */
using TaskId = UInt64;

/**
 * @brief Task execution result
 */
struct TaskResult {
    bool success{true};
    String error;
    Milliseconds execution_time{0};
    
    static TaskResult ok() { return {true, "", Milliseconds{0}}; }
    static TaskResult fail(const String& err) { return {false, err, Milliseconds{0}}; }
};

/**
 * @brief Scheduled task information
 */
struct ScheduledTask {
    TaskId id{0};
    String name;
    Function<TaskResult()> action;
    SystemTimePoint scheduled_time;
    Milliseconds interval{0};  // 0 = one-shot, >0 = periodic
    int priority{0};           // Higher = more important
    bool cancelled{false};
    Size execution_count{0};
    
    bool operator>(const ScheduledTask& other) const {
        if (scheduled_time != other.scheduled_time) {
            return scheduled_time > other.scheduled_time;
        }
        return priority < other.priority;  // Higher priority first when same time
    }
};

// =============================================================================
// Task Scheduler
// =============================================================================

/**
 * @brief Simple task scheduler
 */
class TaskScheduler {
private:
    using TaskQueue = std::priority_queue<ScheduledTask, Vector<ScheduledTask>,
                                          std::greater<ScheduledTask>>;
    
    TaskQueue tasks_;
    HashMap<TaskId, bool> cancelled_tasks_;
    std::atomic<TaskId> next_id_{1};
    mutable Mutex mutex_;
    ConditionVar task_available_;
    AtomicBool running_{false};
    UniquePtr<Thread> scheduler_thread_;
    
    // Statistics
    std::atomic<Size> tasks_executed_{0};
    std::atomic<Size> tasks_failed_{0};
    std::atomic<Size> tasks_cancelled_{0};
    
    void scheduler_loop() {
        while (running_) {
            ScheduledTask task;
            
            {
                UniqueLock<Mutex> lock(mutex_);
                
                if (tasks_.empty()) {
                    task_available_.wait_for(lock, Milliseconds{100});
                    continue;
                }
                
                auto now = SystemClock::now();
                auto next_time = tasks_.top().scheduled_time;
                
                if (next_time > now) {
                    auto wait_time = std::chrono::duration_cast<Milliseconds>(
                        next_time - now);
                    task_available_.wait_for(lock, wait_time);
                    continue;
                }
                
                task = tasks_.top();
                tasks_.pop();
                
                // Check if cancelled
                auto it = cancelled_tasks_.find(task.id);
                if (it != cancelled_tasks_.end() && it->second) {
                    cancelled_tasks_.erase(it);
                    ++tasks_cancelled_;
                    continue;
                }
            }
            
            // Execute task
            auto start = Clock::now();
            TaskResult result;
            
            try {
                result = task.action();
            } catch (const std::exception& e) {
                result = TaskResult::fail(e.what());
            } catch (...) {
                result = TaskResult::fail("Unknown exception");
            }
            
            result.execution_time = std::chrono::duration_cast<Milliseconds>(
                Clock::now() - start);
            
            if (result.success) {
                ++tasks_executed_;
            } else {
                ++tasks_failed_;
            }
            
            // Reschedule if periodic
            if (task.interval.count() > 0 && !task.cancelled) {
                LockGuard<Mutex> lock(mutex_);
                task.scheduled_time = SystemClock::now() + task.interval;
                ++task.execution_count;
                tasks_.push(std::move(task));
            }
        }
    }
    
public:
    TaskScheduler() = default;
    
    ~TaskScheduler() {
        stop();
    }
    
    /**
     * @brief Starts the scheduler
     */
    void start() {
        if (running_.exchange(true)) return;
        
        scheduler_thread_ = std::make_unique<Thread>([this]() {
            scheduler_loop();
        });
    }
    
    /**
     * @brief Stops the scheduler
     */
    void stop() {
        running_ = false;
        task_available_.notify_all();
        
        if (scheduler_thread_ && scheduler_thread_->joinable()) {
            scheduler_thread_->join();
        }
    }
    
    /**
     * @brief Schedules a task for immediate execution
     */
    TaskId schedule(String name, Function<TaskResult()> action, int priority = 0) {
        return schedule_at(std::move(name), std::move(action),
                          SystemClock::now(), priority);
    }
    
    /**
     * @brief Schedules a task after a delay
     */
    TaskId schedule_after(String name, Function<TaskResult()> action,
                         Milliseconds delay, int priority = 0) {
        return schedule_at(std::move(name), std::move(action),
                          SystemClock::now() + delay, priority);
    }
    
    /**
     * @brief Schedules a task at a specific time
     */
    TaskId schedule_at(String name, Function<TaskResult()> action,
                      SystemTimePoint time, int priority = 0) {
        TaskId id = next_id_++;
        
        ScheduledTask task;
        task.id = id;
        task.name = std::move(name);
        task.action = std::move(action);
        task.scheduled_time = time;
        task.priority = priority;
        
        {
            LockGuard<Mutex> lock(mutex_);
            tasks_.push(std::move(task));
        }
        task_available_.notify_one();
        
        return id;
    }
    
    /**
     * @brief Schedules a periodic task
     */
    TaskId schedule_periodic(String name, Function<TaskResult()> action,
                            Milliseconds interval, int priority = 0) {
        TaskId id = next_id_++;
        
        ScheduledTask task;
        task.id = id;
        task.name = std::move(name);
        task.action = std::move(action);
        task.scheduled_time = SystemClock::now() + interval;
        task.interval = interval;
        task.priority = priority;
        
        {
            LockGuard<Mutex> lock(mutex_);
            tasks_.push(std::move(task));
        }
        task_available_.notify_one();
        
        return id;
    }
    
    /**
     * @brief Cancels a scheduled task
     */
    bool cancel(TaskId id) {
        LockGuard<Mutex> lock(mutex_);
        cancelled_tasks_[id] = true;
        return true;
    }
    
    /**
     * @brief Gets pending task count
     */
    Size pending_count() const {
        LockGuard<Mutex> lock(mutex_);
        return tasks_.size();
    }
    
    /**
     * @brief Gets scheduler statistics
     */
    struct Stats {
        Size pending;
        Size executed;
        Size failed;
        Size cancelled;
        bool running;
    };
    
    Stats stats() const {
        LockGuard<Mutex> lock(mutex_);
        return {
            tasks_.size(),
            tasks_executed_.load(),
            tasks_failed_.load(),
            tasks_cancelled_.load(),
            running_.load()
        };
    }
    
    /**
     * @brief Checks if scheduler is running
     */
    bool is_running() const { return running_; }
    
    /**
     * @brief Clears all pending tasks
     */
    void clear() {
        LockGuard<Mutex> lock(mutex_);
        while (!tasks_.empty()) {
            tasks_.pop();
        }
        cancelled_tasks_.clear();
    }
};

// =============================================================================
// Cron-like Scheduler
// =============================================================================

/**
 * @brief Simple cron-like time specification
 */
struct CronSpec {
    int minute{-1};      // 0-59, -1 = any
    int hour{-1};        // 0-23, -1 = any
    int day_of_month{-1}; // 1-31, -1 = any
    int month{-1};       // 1-12, -1 = any
    int day_of_week{-1}; // 0-6 (Sun-Sat), -1 = any
    
    static CronSpec every_minute() { return {}; }
    static CronSpec every_hour() { return {0}; }
    static CronSpec daily_at(int hour, int minute = 0) { return {minute, hour}; }
    static CronSpec weekly_on(int day, int hour = 0, int minute = 0) {
        return {minute, hour, -1, -1, day};
    }
    
    /**
     * @brief Checks if spec matches given time
     */
    bool matches(const std::tm& tm) const {
        if (minute != -1 && tm.tm_min != minute) return false;
        if (hour != -1 && tm.tm_hour != hour) return false;
        if (day_of_month != -1 && tm.tm_mday != day_of_month) return false;
        if (month != -1 && (tm.tm_mon + 1) != month) return false;
        if (day_of_week != -1 && tm.tm_wday != day_of_week) return false;
        return true;
    }
    
    /**
     * @brief Gets next matching time after given time
     */
    SystemTimePoint next_occurrence(SystemTimePoint after = SystemClock::now()) const {
        auto time_t_val = SystemClock::to_time_t(after);
        std::tm tm = *std::localtime(&time_t_val);
        
        // Move to next minute
        tm.tm_sec = 0;
        tm.tm_min++;
        
        // Simple forward search (up to 1 year)
        for (int i = 0; i < 525600; ++i) {  // minutes in a year
            std::time_t check = std::mktime(&tm);
            std::tm check_tm = *std::localtime(&check);
            
            if (matches(check_tm)) {
                return SystemClock::from_time_t(check);
            }
            
            tm.tm_min++;
            std::mktime(&tm);  // Normalize
        }
        
        // Fallback: 1 minute from now
        return after + std::chrono::minutes(1);
    }
};

/**
 * @brief Cron-style scheduled task
 */
class CronScheduler {
private:
    struct CronTask {
        TaskId id;
        String name;
        CronSpec spec;
        Function<TaskResult()> action;
        bool enabled{true};
    };
    
    Vector<CronTask> tasks_;
    std::atomic<TaskId> next_id_{1};
    mutable Mutex mutex_;
    AtomicBool running_{false};
    UniquePtr<Thread> scheduler_thread_;
    
    void scheduler_loop() {
        while (running_) {
            auto now = SystemClock::now();
            auto now_t = SystemClock::to_time_t(now);
            std::tm now_tm = *std::localtime(&now_t);
            
            {
                LockGuard<Mutex> lock(mutex_);
                for (auto& task : tasks_) {
                    if (task.enabled && task.spec.matches(now_tm)) {
                        try {
                            task.action();
                        } catch (...) {
                            // Log error in production
                        }
                    }
                }
            }
            
            // Wait until next minute
            auto next_minute = now + std::chrono::minutes(1);
            auto next_t = SystemClock::to_time_t(next_minute);
            std::tm next_tm = *std::localtime(&next_t);
            next_tm.tm_sec = 0;
            auto target = SystemClock::from_time_t(std::mktime(&next_tm));
            
            std::this_thread::sleep_until(target);
        }
    }
    
public:
    void start() {
        if (running_.exchange(true)) return;
        scheduler_thread_ = std::make_unique<Thread>([this]() {
            scheduler_loop();
        });
    }
    
    void stop() {
        running_ = false;
        if (scheduler_thread_ && scheduler_thread_->joinable()) {
            scheduler_thread_->join();
        }
    }
    
    TaskId add(String name, CronSpec spec, Function<TaskResult()> action) {
        TaskId id = next_id_++;
        
        LockGuard<Mutex> lock(mutex_);
        tasks_.push_back({id, std::move(name), spec, std::move(action), true});
        
        return id;
    }
    
    void enable(TaskId id) {
        LockGuard<Mutex> lock(mutex_);
        for (auto& task : tasks_) {
            if (task.id == id) {
                task.enabled = true;
                break;
            }
        }
    }
    
    void disable(TaskId id) {
        LockGuard<Mutex> lock(mutex_);
        for (auto& task : tasks_) {
            if (task.id == id) {
                task.enabled = false;
                break;
            }
        }
    }
    
    void remove(TaskId id) {
        LockGuard<Mutex> lock(mutex_);
        tasks_.erase(
            std::remove_if(tasks_.begin(), tasks_.end(),
                [id](const CronTask& t) { return t.id == id; }),
            tasks_.end());
    }
    
    Size task_count() const {
        LockGuard<Mutex> lock(mutex_);
        return tasks_.size();
    }
    
    bool is_running() const { return running_; }
};

} // namespace ims::common

#endif // IMS_COMMON_TASK_SCHEDULER_HPP
