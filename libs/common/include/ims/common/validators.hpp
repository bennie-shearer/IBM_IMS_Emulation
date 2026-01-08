/**
 * @file validators.hpp
 * @brief Comprehensive data validation utilities
 * @version 3.6.3
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_VALIDATORS_HPP
#define IMS_COMMON_VALIDATORS_HPP

#include "types.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <regex>
#include <vector>

namespace ims::common {

/**
 * @brief Validation result containing success/failure and error messages
 */
class ValidationResult {
public:
    ValidationResult() : valid_(true) {}
    
    static ValidationResult success() {
        return ValidationResult();
    }
    
    static ValidationResult failure(String message) {
        ValidationResult result;
        result.valid_ = false;
        result.errors_.push_back(std::move(message));
        return result;
    }
    
    static ValidationResult failure(std::vector<String> messages) {
        ValidationResult result;
        result.valid_ = false;
        result.errors_ = std::move(messages);
        return result;
    }
    
    bool is_valid() const { return valid_; }
    bool is_invalid() const { return !valid_; }
    explicit operator bool() const { return valid_; }
    
    const std::vector<String>& errors() const { return errors_; }
    
    String first_error() const {
        return errors_.empty() ? "" : errors_.front();
    }
    
    String all_errors(const String& separator = "; ") const {
        String result;
        for (Size i = 0; i < errors_.size(); ++i) {
            if (i > 0) result += separator;
            result += errors_[i];
        }
        return result;
    }
    
    ValidationResult& add_error(String message) {
        valid_ = false;
        errors_.push_back(std::move(message));
        return *this;
    }
    
    ValidationResult& merge(const ValidationResult& other) {
        if (!other.valid_) {
            valid_ = false;
            errors_.insert(errors_.end(), other.errors_.begin(), other.errors_.end());
        }
        return *this;
    }

private:
    bool valid_;
    std::vector<String> errors_;
};

/**
 * @brief Generic validator interface
 */
template<typename T>
class Validator {
public:
    using ValidateFunc = std::function<ValidationResult(const T&)>;
    
    Validator() = default;
    
    explicit Validator(ValidateFunc func) : validators_{std::move(func)} {}
    
    Validator& add(ValidateFunc func) {
        validators_.push_back(std::move(func));
        return *this;
    }
    
    ValidationResult validate(const T& value) const {
        ValidationResult result;
        for (const auto& validator : validators_) {
            result.merge(validator(value));
        }
        return result;
    }
    
    bool is_valid(const T& value) const {
        return validate(value).is_valid();
    }

private:
    std::vector<ValidateFunc> validators_;
};

/**
 * @brief Chainable string validator
 */
class StringValidator {
public:
    StringValidator() = default;
    
    StringValidator& not_empty() {
        validators_.push_back([](const String& s) -> ValidationResult {
            if (s.empty()) {
                return ValidationResult::failure("Value cannot be empty");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& min_length(Size min) {
        validators_.push_back([min](const String& s) -> ValidationResult {
            if (s.length() < min) {
                return ValidationResult::failure(
                    "Value must be at least " + std::to_string(min) + " characters");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& max_length(Size max) {
        validators_.push_back([max](const String& s) -> ValidationResult {
            if (s.length() > max) {
                return ValidationResult::failure(
                    "Value must not exceed " + std::to_string(max) + " characters");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& exact_length(Size len) {
        validators_.push_back([len](const String& s) -> ValidationResult {
            if (s.length() != len) {
                return ValidationResult::failure(
                    "Value must be exactly " + std::to_string(len) + " characters");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& alphanumeric() {
        validators_.push_back([](const String& s) -> ValidationResult {
            for (char c : s) {
                if (!std::isalnum(static_cast<unsigned char>(c))) {
                    return ValidationResult::failure("Value must be alphanumeric");
                }
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& alphabetic() {
        validators_.push_back([](const String& s) -> ValidationResult {
            for (char c : s) {
                if (!std::isalpha(static_cast<unsigned char>(c))) {
                    return ValidationResult::failure("Value must be alphabetic");
                }
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& numeric() {
        validators_.push_back([](const String& s) -> ValidationResult {
            for (char c : s) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    return ValidationResult::failure("Value must be numeric");
                }
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& uppercase() {
        validators_.push_back([](const String& s) -> ValidationResult {
            for (char c : s) {
                if (std::isalpha(static_cast<unsigned char>(c)) && 
                    !std::isupper(static_cast<unsigned char>(c))) {
                    return ValidationResult::failure("Value must be uppercase");
                }
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& lowercase() {
        validators_.push_back([](const String& s) -> ValidationResult {
            for (char c : s) {
                if (std::isalpha(static_cast<unsigned char>(c)) && 
                    !std::islower(static_cast<unsigned char>(c))) {
                    return ValidationResult::failure("Value must be lowercase");
                }
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& matches(const String& pattern, const String& error_msg = "") {
        std::regex regex(pattern);
        String msg = error_msg.empty() ? 
            "Value does not match required pattern" : error_msg;
        validators_.push_back([regex, msg](const String& s) -> ValidationResult {
            if (!std::regex_match(s, regex)) {
                return ValidationResult::failure(msg);
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& allowed_chars(const String& chars) {
        validators_.push_back([chars](const String& s) -> ValidationResult {
            for (char c : s) {
                if (chars.find(c) == String::npos) {
                    return ValidationResult::failure(
                        String("Invalid character '") + c + "' in value");
                }
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& starts_with(const String& prefix) {
        validators_.push_back([prefix](const String& s) -> ValidationResult {
            if (s.length() < prefix.length() || 
                s.substr(0, prefix.length()) != prefix) {
                return ValidationResult::failure(
                    "Value must start with '" + prefix + "'");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& ends_with(const String& suffix) {
        validators_.push_back([suffix](const String& s) -> ValidationResult {
            if (s.length() < suffix.length() || 
                s.substr(s.length() - suffix.length()) != suffix) {
                return ValidationResult::failure(
                    "Value must end with '" + suffix + "'");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    StringValidator& custom(std::function<ValidationResult(const String&)> func) {
        validators_.push_back(std::move(func));
        return *this;
    }
    
    ValidationResult validate(const String& value) const {
        ValidationResult result;
        for (const auto& validator : validators_) {
            result.merge(validator(value));
        }
        return result;
    }
    
    bool is_valid(const String& value) const {
        return validate(value).is_valid();
    }

private:
    std::vector<std::function<ValidationResult(const String&)>> validators_;
};

/**
 * @brief Chainable numeric validator
 */
template<typename T>
class NumericValidator {
public:
    NumericValidator() = default;
    
    NumericValidator& min(T min_val) {
        validators_.push_back([min_val](T value) -> ValidationResult {
            if (value < min_val) {
                return ValidationResult::failure(
                    "Value must be at least " + std::to_string(min_val));
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    NumericValidator& max(T max_val) {
        validators_.push_back([max_val](T value) -> ValidationResult {
            if (value > max_val) {
                return ValidationResult::failure(
                    "Value must not exceed " + std::to_string(max_val));
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    NumericValidator& range(T min_val, T max_val) {
        return min(min_val).max(max_val);
    }
    
    NumericValidator& positive() {
        validators_.push_back([](T value) -> ValidationResult {
            if (value <= 0) {
                return ValidationResult::failure("Value must be positive");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    NumericValidator& non_negative() {
        validators_.push_back([](T value) -> ValidationResult {
            if (value < 0) {
                return ValidationResult::failure("Value must be non-negative");
            }
            return ValidationResult::success();
        });
        return *this;
    }
    
    NumericValidator& custom(std::function<ValidationResult(T)> func) {
        validators_.push_back(std::move(func));
        return *this;
    }
    
    ValidationResult validate(T value) const {
        ValidationResult result;
        for (const auto& validator : validators_) {
            result.merge(validator(value));
        }
        return result;
    }
    
    bool is_valid(T value) const {
        return validate(value).is_valid();
    }

private:
    std::vector<std::function<ValidationResult(T)>> validators_;
};

/**
 * @brief Mainframe-specific validators
 */
namespace Validators {

/**
 * @brief Validate z/OS dataset name
 * Rules: 1-44 chars, qualifiers 1-8 chars, A-Z 0-9 @ # $, no leading/trailing dots
 */
inline StringValidator dataset_name() {
    return StringValidator()
        .not_empty()
        .max_length(44)
        .custom([](const String& s) -> ValidationResult {
            if (s.empty()) {
                return ValidationResult::failure("Dataset name cannot be empty");
            }
            
            if (s.front() == '.' || s.back() == '.') {
                return ValidationResult::failure(
                    "Dataset name cannot start or end with a dot");
            }
            
            // Check each qualifier
            Size start = 0;
            for (Size i = 0; i <= s.length(); ++i) {
                if (i == s.length() || s[i] == '.') {
                    Size len = i - start;
                    if (len == 0) {
                        return ValidationResult::failure(
                            "Dataset name contains empty qualifier");
                    }
                    if (len > 8) {
                        return ValidationResult::failure(
                            "Dataset qualifier exceeds 8 characters");
                    }
                    
                    // First char must be alpha or national
                    char first = s[start];
                    if (!std::isalpha(static_cast<unsigned char>(first)) &&
                        first != '@' && first != '#' && first != '$') {
                        return ValidationResult::failure(
                            "Dataset qualifier must start with A-Z, @, #, or $");
                    }
                    
                    // Check all chars
                    for (Size j = start; j < i; ++j) {
                        char c = s[j];
                        if (!std::isalnum(static_cast<unsigned char>(c)) &&
                            c != '@' && c != '#' && c != '$' && c != '-') {
                            return ValidationResult::failure(
                                String("Invalid character '") + c + "' in dataset name");
                        }
                    }
                    
                    start = i + 1;
                }
            }
            
            return ValidationResult::success();
        });
}

/**
 * @brief Validate VSAM key
 */
inline StringValidator vsam_key() {
    return StringValidator()
        .not_empty()
        .max_length(255);
}

/**
 * @brief Validate DD name (1-8 chars, alphanumeric, starts with alpha)
 */
inline StringValidator dd_name() {
    return StringValidator()
        .not_empty()
        .max_length(8)
        .custom([](const String& s) -> ValidationResult {
            if (s.empty()) {
                return ValidationResult::failure("DD name cannot be empty");
            }
            
            if (!std::isalpha(static_cast<unsigned char>(s[0]))) {
                return ValidationResult::failure("DD name must start with a letter");
            }
            
            for (char c : s) {
                if (!std::isalnum(static_cast<unsigned char>(c))) {
                    return ValidationResult::failure(
                        "DD name must be alphanumeric");
                }
            }
            
            return ValidationResult::success();
        });
}

/**
 * @brief Validate member name (PDS member, 1-8 chars)
 */
inline StringValidator member_name() {
    return StringValidator()
        .not_empty()
        .max_length(8)
        .custom([](const String& s) -> ValidationResult {
            if (s.empty()) {
                return ValidationResult::failure("Member name cannot be empty");
            }
            
            if (!std::isalpha(static_cast<unsigned char>(s[0])) &&
                s[0] != '@' && s[0] != '#' && s[0] != '$') {
                return ValidationResult::failure(
                    "Member name must start with A-Z, @, #, or $");
            }
            
            for (char c : s) {
                if (!std::isalnum(static_cast<unsigned char>(c)) &&
                    c != '@' && c != '#' && c != '$') {
                    return ValidationResult::failure(
                        "Member name contains invalid character");
                }
            }
            
            return ValidationResult::success();
        });
}

/**
 * @brief Validate JOB name (1-8 chars, alphanumeric + national, starts with alpha/national)
 */
inline StringValidator job_name() {
    return member_name();  // Same rules as member name
}

/**
 * @brief Validate LRECL (record length)
 */
inline NumericValidator<int> lrecl() {
    return NumericValidator<int>().range(1, 32760);
}

/**
 * @brief Validate BLKSIZE (block size)
 */
inline NumericValidator<int> blksize() {
    return NumericValidator<int>().range(1, 32760);
}

/**
 * @brief Validate volume serial (1-6 chars, alphanumeric)
 */
inline StringValidator volume_serial() {
    return StringValidator()
        .not_empty()
        .max_length(6)
        .alphanumeric()
        .uppercase();
}

}  // namespace Validators

}  // namespace ims::common

#endif  // IMS_COMMON_VALIDATORS_HPP
