#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Error Handling
// Version: 3.6.3
// =============================================================================

#include "types.hpp"
#include <stdexcept>
#include <system_error>
#include <source_location>

namespace ims {

// =============================================================================
// Error Codes
// =============================================================================

enum class ImsErrorCode : Int32 {
    SUCCESS = 0,
    
    // General errors (1-99)
    UNKNOWN_ERROR = 1,
    INVALID_ARGUMENT = 2,
    OUT_OF_MEMORY = 3,
    NOT_IMPLEMENTED = 4,
    OPERATION_CANCELLED = 5,
    TIMEOUT = 6,
    PERMISSION_DENIED = 7,
    ALREADY_EXISTS = 8,
    NOT_FOUND = 9,
    INVALID_STATE = 10,
    
    // File/IO errors (100-199)
    FILE_NOT_FOUND = 100,
    FILE_ALREADY_EXISTS = 101,
    FILE_ACCESS_DENIED = 102,
    FILE_READ_ERROR = 103,
    FILE_WRITE_ERROR = 104,
    FILE_SEEK_ERROR = 105,
    FILE_LOCK_ERROR = 106,
    DIRECTORY_NOT_FOUND = 107,
    DISK_FULL = 108,
    
    // Dataset errors (200-299)
    DATASET_NOT_FOUND = 200,
    DATASET_ALREADY_EXISTS = 201,
    DATASET_IN_USE = 202,
    DATASET_FULL = 203,
    DATASET_EMPTY = 204,
    DATASET_CORRUPTED = 205,
    INVALID_DATASET_NAME = 206,
    INVALID_DATASET_TYPE = 207,
    
    // Record errors (300-399)
    RECORD_NOT_FOUND = 300,
    RECORD_ALREADY_EXISTS = 301,
    RECORD_TOO_LARGE = 302,
    RECORD_TOO_SMALL = 303,
    INVALID_RECORD_FORMAT = 304,
    KEY_NOT_FOUND = 305,
    DUPLICATE_KEY = 306,
    INVALID_KEY = 307,
    INVALID_RBA = 308,
    INVALID_SLOT = 309,
    
    // Catalog errors (400-499)
    CATALOG_NOT_FOUND = 400,
    CATALOG_ALREADY_EXISTS = 401,
    CATALOG_FULL = 402,
    CATALOG_CORRUPTED = 403,
    INVALID_CATALOG_ENTRY = 404,
    VOLUME_NOT_FOUND = 405,
    VOLUME_OFFLINE = 406,
    
    // Security errors (500-599)
    AUTHENTICATION_FAILED = 500,
    AUTHORIZATION_FAILED = 501,
    SESSION_EXPIRED = 502,
    INVALID_CREDENTIALS = 503,
    ACCOUNT_LOCKED = 504,
    CERTIFICATE_ERROR = 505,
    ENCRYPTION_ERROR = 506,
    
    // Transaction errors (600-699)
    TRANSACTION_ABORTED = 600,
    TRANSACTION_TIMEOUT = 601,
    DEADLOCK_DETECTED = 602,
    ROLLBACK_REQUIRED = 603,
    COMMIT_FAILED = 604,
    
    // IMS/DL-I errors (700-799)
    DLI_ERROR = 700,
    SEGMENT_NOT_FOUND = 701,
    INVALID_SSA = 702,
    PCB_ERROR = 703,
    DATABASE_NOT_AVAILABLE = 704,
    
    // System errors (900-999)
    SYSTEM_ERROR = 900,
    INITIALIZATION_FAILED = 901,
    SHUTDOWN_ERROR = 902,
    RESOURCE_EXHAUSTED = 903,
    INTERNAL_ERROR = 999
};

// =============================================================================
// Error Category
// =============================================================================

class ImsErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override { return "ims"; }
    
    std::string message(int ev) const override {
        switch (static_cast<ImsErrorCode>(ev)) {
            case ImsErrorCode::SUCCESS: return "Success";
            case ImsErrorCode::UNKNOWN_ERROR: return "Unknown error";
            case ImsErrorCode::INVALID_ARGUMENT: return "Invalid argument";
            case ImsErrorCode::OUT_OF_MEMORY: return "Out of memory";
            case ImsErrorCode::NOT_IMPLEMENTED: return "Not implemented";
            case ImsErrorCode::NOT_FOUND: return "Not found";
            case ImsErrorCode::ALREADY_EXISTS: return "Already exists";
            case ImsErrorCode::PERMISSION_DENIED: return "Permission denied";
            case ImsErrorCode::TIMEOUT: return "Operation timed out";
            case ImsErrorCode::FILE_NOT_FOUND: return "File not found";
            case ImsErrorCode::DATASET_NOT_FOUND: return "Dataset not found";
            case ImsErrorCode::RECORD_NOT_FOUND: return "Record not found";
            case ImsErrorCode::DUPLICATE_KEY: return "Duplicate key";
            case ImsErrorCode::AUTHENTICATION_FAILED: return "Authentication failed";
            case ImsErrorCode::AUTHORIZATION_FAILED: return "Authorization failed";
            default: return "IMS error " + std::to_string(ev);
        }
    }
};

inline const ImsErrorCategory& get_ims_error_category() {
    static ImsErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(ImsErrorCode e) {
    return {static_cast<int>(e), get_ims_error_category()};
}

} // namespace ims

namespace std {
    template<>
    struct is_error_code_enum<ims::ImsErrorCode> : true_type {};
}

namespace ims {

// =============================================================================
// Error Information
// =============================================================================

struct ErrorInfo {
    ImsErrorCode code{ImsErrorCode::SUCCESS};
    String message;
    String detail;
    String source_file;
    Int32 source_line{0};
    String function_name;
    SystemTimePoint timestamp;
    
    ErrorInfo() : timestamp(SystemClock::now()) {}
    
    ErrorInfo(ImsErrorCode c, String msg, 
              const std::source_location& loc = std::source_location::current())
        : code(c)
        , message(std::move(msg))
        , source_file(loc.file_name())
        , source_line(static_cast<Int32>(loc.line()))
        , function_name(loc.function_name())
        , timestamp(SystemClock::now()) {}
    
    bool is_success() const { return code == ImsErrorCode::SUCCESS; }
    bool is_error() const { return code != ImsErrorCode::SUCCESS; }
    
    String to_string() const {
        if (is_success()) return "Success";
        return std::format("[{}] {} ({}:{})", 
            static_cast<int>(code), message, source_file, source_line);
    }
};

// =============================================================================
// ErrorResult Template - Fixed for T=String case
// =============================================================================

/**
 * @brief Result type for operations that can fail
 * 
 * Uses ErrorWrapper internally to avoid std::variant<T, String> ambiguity
 * when T is also String.
 */
template<typename T>
class ErrorResult {
private:
    // Wrapper type to make error distinct from T (solves variant<String, String> issue)
    struct ErrorWrapper {
        String message;
        explicit ErrorWrapper(String msg) : message(std::move(msg)) {}
    };
    
    std::variant<T, ErrorWrapper> result_;
    
    // Tag type for error construction
    struct ErrorTag {};
    
public:
    /// Construct with success value
    ErrorResult(T value) : result_(std::in_place_index<0>, std::move(value)) {}
    
    /// Construct with error message (using tag to avoid ambiguity)
    ErrorResult(ErrorTag, String error) : result_(std::in_place_index<1>, std::move(error)) {}
    
    /// Construct with error message from const char*
    ErrorResult(const char* error) : result_(std::in_place_index<1>, String(error)) {}
    
    /// Construct with error message string (disabled when T=String to avoid ambiguity)
    template<typename U = T,
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, String>>>
    ErrorResult(String error) : result_(std::in_place_index<1>, std::move(error)) {}
    
    /// Factory for creating error results (works for all T including String)
    static ErrorResult make_error(String error) {
        return ErrorResult(ErrorTag{}, std::move(error));
    }
    
    /// Factory for creating success results
    static ErrorResult make_success(T value) {
        return ErrorResult(std::move(value));
    }
    
    /// Check if result is success
    bool has_value() const noexcept {
        return result_.index() == 0;
    }
    
    /// Check if result is success (boolean conversion)
    explicit operator bool() const noexcept {
        return has_value();
    }
    
    /// Check if result is error
    bool is_error() const noexcept {
        return !has_value();
    }
    
    /// Get the success value (throws if error)
    T& value() & {
        if (!has_value()) {
            throw std::runtime_error("ErrorResult: accessing value when in error state: " + error());
        }
        return std::get<0>(result_);
    }
    
    /// Get the success value (throws if error) - const version
    const T& value() const& {
        if (!has_value()) {
            throw std::runtime_error("ErrorResult: accessing value when in error state: " + error());
        }
        return std::get<0>(result_);
    }
    
    /// Get the success value (throws if error) - rvalue version
    T&& value() && {
        if (!has_value()) {
            throw std::runtime_error("ErrorResult: accessing value when in error state: " + error());
        }
        return std::move(std::get<0>(result_));
    }
    
    /// Get the error message (throws if success)
    const String& error() const {
        if (has_value()) {
            throw std::runtime_error("ErrorResult: accessing error when in success state");
        }
        return std::get<1>(result_).message;
    }
    
    /// Get value or default
    T value_or(T default_value) const {
        return has_value() ? std::get<0>(result_) : std::move(default_value);
    }
    
    /// Map success value
    template<typename F>
    auto map(F&& f) const -> ErrorResult<decltype(f(std::declval<T>()))> {
        using R = decltype(f(std::declval<T>()));
        if (has_value()) {
            return ErrorResult<R>(f(std::get<0>(result_)));
        }
        return ErrorResult<R>::make_error(error());
    }
    
    /// Flat map success value
    template<typename F>
    auto and_then(F&& f) const -> decltype(f(std::declval<T>())) {
        if (has_value()) {
            return f(std::get<0>(result_));
        }
        using R = decltype(f(std::declval<T>()));
        return R::make_error(error());
    }
};

// =============================================================================
// Specialization for void
// =============================================================================

template<>
class ErrorResult<void> {
private:
    Optional<String> error_;
    
public:
    ErrorResult() = default;
    ErrorResult(const char* error) : error_(String(error)) {}
    ErrorResult(String error) : error_(std::move(error)) {}
    
    static ErrorResult make_error(String error) {
        return ErrorResult(std::move(error));
    }
    
    static ErrorResult make_success() {
        return ErrorResult();
    }
    
    bool has_value() const noexcept { return !error_.has_value(); }
    explicit operator bool() const noexcept { return has_value(); }
    bool is_error() const noexcept { return error_.has_value(); }
    
    void value() const {
        if (is_error()) {
            throw std::runtime_error("ErrorResult<void>: in error state: " + *error_);
        }
    }
    
    const String& error() const {
        if (!is_error()) {
            throw std::runtime_error("ErrorResult<void>: accessing error when in success state");
        }
        return *error_;
    }
};

// =============================================================================
// Helper Functions
// =============================================================================

template<typename T>
inline ErrorResult<T> make_error(String message) {
    return ErrorResult<T>::make_error(std::move(message));
}

inline ErrorResult<void> make_success() {
    return ErrorResult<void>::make_success();
}

template<typename T>
inline ErrorResult<T> make_success(T value) {
    return ErrorResult<T>::make_success(std::move(value));
}

// =============================================================================
// IMS Exception
// =============================================================================

class ImsException : public std::runtime_error {
private:
    ImsErrorCode code_;
    ErrorInfo info_;
    
public:
    explicit ImsException(const ErrorInfo& info)
        : std::runtime_error(info.to_string())
        , code_(info.code)
        , info_(info) {}
    
    ImsException(ImsErrorCode code, const String& message,
                 const std::source_location& loc = std::source_location::current())
        : std::runtime_error(message)
        , code_(code)
        , info_(code, message, loc) {}
    
    ImsErrorCode code() const noexcept { return code_; }
    const ErrorInfo& info() const noexcept { return info_; }
};

// =============================================================================
// Error Reporter (Thread-safe)
// =============================================================================

class ErrorReporter {
private:
    mutable SharedMutex mutex_;
    Vector<ErrorInfo> errors_;
    Size max_errors_{1000};
    AtomicUInt64 total_errors_{0};
    AtomicUInt64 suppressed_errors_{0};
    
public:
    void report(const ErrorInfo& error) {
        total_errors_.fetch_add(1, std::memory_order_relaxed);
        
        UniqueLock<SharedMutex> lock(mutex_);
        if (errors_.size() >= max_errors_) {
            suppressed_errors_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        errors_.push_back(error);
    }
    
    void report(ImsErrorCode code, const String& message,
                const std::source_location& loc = std::source_location::current()) {
        report(ErrorInfo(code, message, loc));
    }
    
    Vector<ErrorInfo> get_errors() const {
        SharedLock<SharedMutex> lock(mutex_);
        return errors_;
    }
    
    void clear() {
        UniqueLock<SharedMutex> lock(mutex_);
        errors_.clear();
    }
    
    Size error_count() const {
        SharedLock<SharedMutex> lock(mutex_);
        return errors_.size();
    }
    
    UInt64 total_error_count() const {
        return total_errors_.load(std::memory_order_relaxed);
    }
    
    UInt64 suppressed_count() const {
        return suppressed_errors_.load(std::memory_order_relaxed);
    }
    
    void set_max_errors(Size max) {
        UniqueLock<SharedMutex> lock(mutex_);
        max_errors_ = max;
    }
};

// Global error reporter
inline ErrorReporter& global_error_reporter() {
    static ErrorReporter reporter;
    return reporter;
}

} // namespace ims
