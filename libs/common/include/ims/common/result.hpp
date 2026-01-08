/**
 * @file result.hpp
 * @brief Type-safe result type with error chaining for error handling
 * @version 3.6.3
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_RESULT_HPP
#define IMS_COMMON_RESULT_HPP

#include "types.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

namespace ims::common {

/**
 * @brief Error context for building error chains
 */
class ErrorContext {
public:
    explicit ErrorContext(String message, String location = "")
        : message_(std::move(message)), location_(std::move(location)) {}
    
    const String& message() const { return message_; }
    const String& location() const { return location_; }
    
    String to_string() const {
        if (location_.empty()) {
            return message_;
        }
        return location_ + ": " + message_;
    }

private:
    String message_;
    String location_;
};

/**
 * @brief Chain of error contexts for debugging
 */
class ErrorChain {
public:
    ErrorChain() = default;
    
    explicit ErrorChain(String message, String location = "") {
        contexts_.emplace_back(std::move(message), std::move(location));
    }
    
    ErrorChain& add_context(String message, String location = "") {
        contexts_.emplace_back(std::move(message), std::move(location));
        return *this;
    }
    
    ErrorChain with_context(String message, String location = "") const {
        ErrorChain result = *this;
        result.add_context(std::move(message), std::move(location));
        return result;
    }
    
    const std::vector<ErrorContext>& contexts() const { return contexts_; }
    
    bool empty() const { return contexts_.empty(); }
    
    String root_message() const {
        return contexts_.empty() ? "" : contexts_.front().message();
    }
    
    String full_message() const {
        String result;
        for (Size i = 0; i < contexts_.size(); ++i) {
            if (i > 0) result += "\n  caused by: ";
            result += contexts_[i].to_string();
        }
        return result;
    }
    
    String to_string() const { return full_message(); }

private:
    std::vector<ErrorContext> contexts_;
};

/**
 * @brief Type-safe Result type holding either a value or an error
 * 
 * Provides monadic operations for chaining computations that may fail.
 */
template<typename T, typename E = ErrorChain>
class Result {
public:
    using ValueType = T;
    using ErrorType = E;
    
    // Construction
    static Result ok(T value) {
        return Result(std::in_place_index<0>, std::move(value));
    }
    
    static Result err(E error) {
        return Result(std::in_place_index<1>, std::move(error));
    }
    
    // For ErrorChain convenience
    static Result err(String message, String location = "") {
        return Result(std::in_place_index<1>, E(std::move(message), std::move(location)));
    }
    
    // Query state
    bool is_ok() const { return data_.index() == 0; }
    bool is_err() const { return data_.index() == 1; }
    explicit operator bool() const { return is_ok(); }
    
    // Value access (throws on error)
    T& value() & {
        if (is_err()) throw std::runtime_error("Result contains error");
        return std::get<0>(data_);
    }
    
    const T& value() const& {
        if (is_err()) throw std::runtime_error("Result contains error");
        return std::get<0>(data_);
    }
    
    T&& value() && {
        if (is_err()) throw std::runtime_error("Result contains error");
        return std::move(std::get<0>(data_));
    }
    
    // Error access (throws on ok)
    E& error() & {
        if (is_ok()) throw std::runtime_error("Result contains value");
        return std::get<1>(data_);
    }
    
    const E& error() const& {
        if (is_ok()) throw std::runtime_error("Result contains value");
        return std::get<1>(data_);
    }
    
    E&& error() && {
        if (is_ok()) throw std::runtime_error("Result contains value");
        return std::move(std::get<1>(data_));
    }
    
    // Safe access
    T value_or(T default_value) const& {
        return is_ok() ? std::get<0>(data_) : std::move(default_value);
    }
    
    T value_or(T default_value) && {
        return is_ok() ? std::move(std::get<0>(data_)) : std::move(default_value);
    }
    
    std::optional<T> to_optional() const& {
        return is_ok() ? std::optional<T>(std::get<0>(data_)) : std::nullopt;
    }
    
    std::optional<T> to_optional() && {
        return is_ok() ? std::optional<T>(std::move(std::get<0>(data_))) : std::nullopt;
    }
    
    // Pointer-like access
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }
    T& operator*() & { return value(); }
    const T& operator*() const& { return value(); }
    T&& operator*() && { return std::move(value()); }
    
    // Monadic operations
    
    /**
     * @brief Transform the value if present
     */
    template<typename F>
    auto map(F&& f) const& -> Result<std::invoke_result_t<F, const T&>, E> {
        using U = std::invoke_result_t<F, const T&>;
        if (is_ok()) {
            return Result<U, E>::ok(std::invoke(std::forward<F>(f), std::get<0>(data_)));
        }
        return Result<U, E>::err(std::get<1>(data_));
    }
    
    template<typename F>
    auto map(F&& f) && -> Result<std::invoke_result_t<F, T&&>, E> {
        using U = std::invoke_result_t<F, T&&>;
        if (is_ok()) {
            return Result<U, E>::ok(std::invoke(std::forward<F>(f), std::move(std::get<0>(data_))));
        }
        return Result<U, E>::err(std::move(std::get<1>(data_)));
    }
    
    /**
     * @brief Transform the error if present
     */
    template<typename F>
    auto map_error(F&& f) const& -> Result<T, std::invoke_result_t<F, const E&>> {
        using U = std::invoke_result_t<F, const E&>;
        if (is_err()) {
            return Result<T, U>::err(std::invoke(std::forward<F>(f), std::get<1>(data_)));
        }
        return Result<T, U>::ok(std::get<0>(data_));
    }
    
    template<typename F>
    auto map_error(F&& f) && -> Result<T, std::invoke_result_t<F, E&&>> {
        using U = std::invoke_result_t<F, E&&>;
        if (is_err()) {
            return Result<T, U>::err(std::invoke(std::forward<F>(f), std::move(std::get<1>(data_))));
        }
        return Result<T, U>::ok(std::move(std::get<0>(data_)));
    }
    
    /**
     * @brief Chain operations that return Result
     */
    template<typename F>
    auto and_then(F&& f) const& -> std::invoke_result_t<F, const T&> {
        using ResultType = std::invoke_result_t<F, const T&>;
        if (is_ok()) {
            return std::invoke(std::forward<F>(f), std::get<0>(data_));
        }
        return ResultType::err(std::get<1>(data_));
    }
    
    template<typename F>
    auto and_then(F&& f) && -> std::invoke_result_t<F, T&&> {
        using ResultType = std::invoke_result_t<F, T&&>;
        if (is_ok()) {
            return std::invoke(std::forward<F>(f), std::move(std::get<0>(data_)));
        }
        return ResultType::err(std::move(std::get<1>(data_)));
    }
    
    /**
     * @brief Provide fallback on error
     */
    template<typename F>
    auto or_else(F&& f) const& -> std::invoke_result_t<F, const E&> {
        using ResultType = std::invoke_result_t<F, const E&>;
        if (is_err()) {
            return std::invoke(std::forward<F>(f), std::get<1>(data_));
        }
        return ResultType::ok(std::get<0>(data_));
    }
    
    template<typename F>
    auto or_else(F&& f) && -> std::invoke_result_t<F, E&&> {
        using ResultType = std::invoke_result_t<F, E&&>;
        if (is_err()) {
            return std::invoke(std::forward<F>(f), std::move(std::get<1>(data_)));
        }
        return ResultType::ok(std::move(std::get<0>(data_)));
    }
    
    /**
     * @brief Add context to error
     */
    Result with_context(String message, String location = "") const& {
        if (is_err()) {
            if constexpr (std::is_same_v<E, ErrorChain>) {
                return Result::err(std::get<1>(data_).with_context(std::move(message), std::move(location)));
            } else {
                return *this;
            }
        }
        return *this;
    }
    
    Result with_context(String message, String location = "") && {
        if (is_err()) {
            if constexpr (std::is_same_v<E, ErrorChain>) {
                return Result::err(std::move(std::get<1>(data_)).with_context(std::move(message), std::move(location)));
            } else {
                return std::move(*this);
            }
        }
        return std::move(*this);
    }

private:
    template<typename... Args>
    explicit Result(Args&&... args) : data_(std::forward<Args>(args)...) {}
    
    std::variant<T, E> data_;
};

/**
 * @brief Specialization for void value type
 */
template<typename E>
class Result<void, E> {
public:
    using ValueType = void;
    using ErrorType = E;
    
    static Result ok() {
        return Result(true);
    }
    
    static Result err(E error) {
        return Result(std::move(error));
    }
    
    static Result err(String message, String location = "") {
        return Result(E(std::move(message), std::move(location)));
    }
    
    bool is_ok() const { return !error_.has_value(); }
    bool is_err() const { return error_.has_value(); }
    explicit operator bool() const { return is_ok(); }
    
    const E& error() const {
        if (is_ok()) throw std::runtime_error("Result contains value");
        return *error_;
    }
    
    E& error() {
        if (is_ok()) throw std::runtime_error("Result contains value");
        return *error_;
    }
    
    template<typename F>
    auto map(F&& f) const -> Result<std::invoke_result_t<F>, E> {
        using U = std::invoke_result_t<F>;
        if (is_ok()) {
            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f));
                return Result<void, E>::ok();
            } else {
                return Result<U, E>::ok(std::invoke(std::forward<F>(f)));
            }
        }
        return Result<U, E>::err(*error_);
    }
    
    template<typename F>
    auto and_then(F&& f) const -> std::invoke_result_t<F> {
        using ResultType = std::invoke_result_t<F>;
        if (is_ok()) {
            return std::invoke(std::forward<F>(f));
        }
        return ResultType::err(*error_);
    }
    
    Result with_context(String message, String location = "") const {
        if (is_err()) {
            if constexpr (std::is_same_v<E, ErrorChain>) {
                return Result::err(error_->with_context(std::move(message), std::move(location)));
            }
        }
        return *this;
    }

private:
    explicit Result(bool) : error_(std::nullopt) {}
    explicit Result(E error) : error_(std::move(error)) {}
    
    std::optional<E> error_;
};

// Convenience type aliases
template<typename T>
using ResultE = Result<T, ErrorChain>;

using VoidResult = Result<void, ErrorChain>;

// Helper functions
template<typename T>
ResultE<T> make_ok(T value) {
    return ResultE<T>::ok(std::move(value));
}

inline VoidResult make_ok() {
    return VoidResult::ok();
}

template<typename T = void>
Result<T, ErrorChain> make_err(String message, String location = "") {
    return Result<T, ErrorChain>::err(std::move(message), std::move(location));
}

}  // namespace ims::common

#endif  // IMS_COMMON_RESULT_HPP
