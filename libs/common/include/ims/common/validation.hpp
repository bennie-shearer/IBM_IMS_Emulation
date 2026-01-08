#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Data Validation Framework
// Version: 3.6.2
// =============================================================================
//
// Provides mainframe-compatible validation for dataset names, record sizes,
// key fields, and other IBM-compatible constraints.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include <regex>
#include <cctype>

namespace ims::validation {

// =============================================================================
// Validation Result
// =============================================================================

struct ValidationResult {
    bool valid{true};
    Vector<String> errors;
    Vector<String> warnings;
    
    void add_error(const String& msg) {
        valid = false;
        errors.push_back(msg);
    }
    
    void add_warning(const String& msg) {
        warnings.push_back(msg);
    }
    
    operator bool() const { return valid; }
    
    String to_string() const {
        String result;
        if (!valid) {
            result = "INVALID: ";
            for (const auto& e : errors) {
                result += e + "; ";
            }
        } else {
            result = "VALID";
        }
        if (!warnings.empty()) {
            result += " (Warnings: ";
            for (const auto& w : warnings) {
                result += w + "; ";
            }
            result += ")";
        }
        return result;
    }
};

// =============================================================================
// Dataset Name Validator (IBM 44-character convention)
// =============================================================================

class DatasetNameValidator {
public:
    static constexpr Size MAX_DATASET_NAME_LENGTH = 44;
    static constexpr Size MAX_QUALIFIER_LENGTH = 8;
    
    static ValidationResult validate(StringView name) {
        ValidationResult result;
        
        if (name.empty()) {
            result.add_error("Dataset name cannot be empty");
            return result;
        }
        
        if (name.length() > MAX_DATASET_NAME_LENGTH) {
            result.add_error(std::format("Dataset name exceeds {} characters (got {})",
                MAX_DATASET_NAME_LENGTH, name.length()));
        }
        
        // Split into qualifiers
        Vector<String> qualifiers;
        String current;
        for (char c : name) {
            if (c == '.') {
                if (current.empty()) {
                    result.add_error("Empty qualifier (consecutive dots or leading dot)");
                } else {
                    qualifiers.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (current.empty()) {
            result.add_error("Trailing dot not allowed");
        } else {
            qualifiers.push_back(current);
        }
        
        // Validate each qualifier
        for (Size i = 0; i < qualifiers.size(); ++i) {
            const auto& q = qualifiers[i];
            
            if (q.length() > MAX_QUALIFIER_LENGTH) {
                result.add_error(std::format("Qualifier {} exceeds {} characters: '{}'",
                    i + 1, MAX_QUALIFIER_LENGTH, q));
            }
            
            // First character must be alphabetic or national (@#$)
            if (!q.empty()) {
                char first = q[0];
                if (!std::isalpha(static_cast<unsigned char>(first)) && 
                    first != '@' && first != '#' && first != '$') {
                    result.add_error(std::format(
                        "Qualifier {} must start with A-Z or @#$: '{}'", i + 1, q));
                }
            }
            
            // Remaining characters must be alphanumeric or national
            for (Size j = 1; j < q.length(); ++j) {
                char c = q[j];
                if (!std::isalnum(static_cast<unsigned char>(c)) && 
                    c != '@' && c != '#' && c != '$') {
                    result.add_error(std::format(
                        "Invalid character '{}' in qualifier {}: '{}'", c, i + 1, q));
                }
            }
        }
        
        return result;
    }
    
    static bool is_valid(StringView name) {
        return validate(name).valid;
    }
    
    static String normalize(StringView name) {
        String result;
        result.reserve(name.length());
        for (char c : name) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return result;
    }
};

// =============================================================================
// Member Name Validator (PDS member names)
// =============================================================================

class MemberNameValidator {
public:
    static constexpr Size MAX_MEMBER_NAME_LENGTH = 8;
    
    static ValidationResult validate(StringView name) {
        ValidationResult result;
        
        if (name.empty()) {
            result.add_error("Member name cannot be empty");
            return result;
        }
        
        if (name.length() > MAX_MEMBER_NAME_LENGTH) {
            result.add_error(std::format("Member name exceeds {} characters", MAX_MEMBER_NAME_LENGTH));
        }
        
        // First character must be alphabetic or national
        char first = name[0];
        if (!std::isalpha(static_cast<unsigned char>(first)) && 
            first != '@' && first != '#' && first != '$') {
            result.add_error("Member name must start with A-Z or @#$");
        }
        
        // Remaining characters must be alphanumeric or national
        for (Size i = 1; i < name.length(); ++i) {
            char c = name[i];
            if (!std::isalnum(static_cast<unsigned char>(c)) && 
                c != '@' && c != '#' && c != '$') {
                result.add_error(std::format("Invalid character '{}' in member name", c));
            }
        }
        
        return result;
    }
    
    static bool is_valid(StringView name) {
        return validate(name).valid;
    }
};

// =============================================================================
// Volume Serial Validator
// =============================================================================

class VolumeSerialValidator {
public:
    static constexpr Size VOLSER_LENGTH = 6;
    
    static ValidationResult validate(StringView volser) {
        ValidationResult result;
        
        if (volser.empty()) {
            result.add_error("Volume serial cannot be empty");
            return result;
        }
        
        if (volser.length() > VOLSER_LENGTH) {
            result.add_error(std::format("Volume serial exceeds {} characters", VOLSER_LENGTH));
        }
        
        // All characters must be alphanumeric
        for (char c : volser) {
            if (!std::isalnum(static_cast<unsigned char>(c))) {
                result.add_error(std::format("Invalid character '{}' in volume serial", c));
            }
        }
        
        return result;
    }
    
    static bool is_valid(StringView volser) {
        return validate(volser).valid;
    }
    
    static String normalize(StringView volser) {
        String result;
        result.reserve(VOLSER_LENGTH);
        for (char c : volser) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        // Pad with spaces if needed
        while (result.length() < VOLSER_LENGTH) {
            result += ' ';
        }
        return result;
    }
};

// =============================================================================
// Record Size Validator
// =============================================================================

class RecordSizeValidator {
public:
    static constexpr UInt32 MIN_RECORD_SIZE = 1;
    static constexpr UInt32 MAX_VSAM_RECORD_SIZE = 32760;  // Max CI size minus overhead
    static constexpr UInt32 MAX_QSAM_RECORD_SIZE = 32760;
    static constexpr UInt32 MIN_BLOCK_SIZE = 18;
    static constexpr UInt32 MAX_BLOCK_SIZE = 32760;
    
    static ValidationResult validate_lrecl(UInt32 lrecl, bool is_vsam = false) {
        ValidationResult result;
        
        if (lrecl < MIN_RECORD_SIZE) {
            result.add_error(std::format("Record length {} is below minimum {}", lrecl, MIN_RECORD_SIZE));
        }
        
        UInt32 max_size = is_vsam ? MAX_VSAM_RECORD_SIZE : MAX_QSAM_RECORD_SIZE;
        if (lrecl > max_size) {
            result.add_error(std::format("Record length {} exceeds maximum {}", lrecl, max_size));
        }
        
        return result;
    }
    
    static ValidationResult validate_blksize(UInt32 blksize, UInt32 lrecl) {
        ValidationResult result;
        
        if (blksize < MIN_BLOCK_SIZE) {
            result.add_error(std::format("Block size {} is below minimum {}", blksize, MIN_BLOCK_SIZE));
        }
        
        if (blksize > MAX_BLOCK_SIZE) {
            result.add_error(std::format("Block size {} exceeds maximum {}", blksize, MAX_BLOCK_SIZE));
        }
        
        if (blksize < lrecl) {
            result.add_error(std::format("Block size {} is less than record length {}", blksize, lrecl));
        }
        
        // Check for optimal blocking
        if (blksize > 0 && lrecl > 0) {
            UInt32 records_per_block = blksize / lrecl;
            UInt32 waste = blksize - (records_per_block * lrecl);
            if (waste > lrecl / 2) {
                result.add_warning(std::format("Block size {} wastes {} bytes per block", blksize, waste));
            }
        }
        
        return result;
    }
};

// =============================================================================
// Key Field Validator (for VSAM KSDS)
// =============================================================================

class KeyFieldValidator {
public:
    static constexpr UInt32 MAX_KEY_LENGTH = 255;
    
    static ValidationResult validate(UInt32 key_length, UInt32 key_offset, UInt32 record_length) {
        ValidationResult result;
        
        if (key_length == 0) {
            result.add_error("Key length cannot be zero");
            return result;
        }
        
        if (key_length > MAX_KEY_LENGTH) {
            result.add_error(std::format("Key length {} exceeds maximum {}", key_length, MAX_KEY_LENGTH));
        }
        
        if (key_offset + key_length > record_length) {
            result.add_error(std::format(
                "Key field (offset {} + length {}) extends beyond record length {}",
                key_offset, key_length, record_length));
        }
        
        return result;
    }
};

// =============================================================================
// JCL Parameter Validator
// =============================================================================

class JclParameterValidator {
public:
    static ValidationResult validate_dd_name(StringView name) {
        ValidationResult result;
        
        if (name.empty()) {
            result.add_error("DD name cannot be empty");
            return result;
        }
        
        if (name.length() > 8) {
            result.add_error("DD name exceeds 8 characters");
        }
        
        // First character must be alphabetic or national
        char first = name[0];
        if (!std::isalpha(static_cast<unsigned char>(first)) && 
            first != '@' && first != '#' && first != '$') {
            result.add_error("DD name must start with A-Z or @#$");
        }
        
        // Remaining characters must be alphanumeric or national
        for (Size i = 1; i < name.length(); ++i) {
            char c = name[i];
            if (!std::isalnum(static_cast<unsigned char>(c)) && 
                c != '@' && c != '#' && c != '$') {
                result.add_error(std::format("Invalid character '{}' in DD name", c));
            }
        }
        
        return result;
    }
    
    static ValidationResult validate_job_name(StringView name) {
        ValidationResult result;
        
        if (name.empty()) {
            result.add_error("Job name cannot be empty");
            return result;
        }
        
        if (name.length() > 8) {
            result.add_error("Job name exceeds 8 characters");
        }
        
        // First character must be alphabetic or national
        char first = name[0];
        if (!std::isalpha(static_cast<unsigned char>(first)) && 
            first != '@' && first != '#' && first != '$') {
            result.add_error("Job name must start with A-Z or @#$");
        }
        
        return result;
    }
};

// =============================================================================
// Composite Validator
// =============================================================================

class CompositeValidator {
private:
    Vector<Function<ValidationResult()>> validators_;
    
public:
    CompositeValidator& add(Function<ValidationResult()> validator) {
        validators_.push_back(std::move(validator));
        return *this;
    }
    
    ValidationResult validate() const {
        ValidationResult combined;
        for (const auto& v : validators_) {
            auto result = v();
            for (const auto& e : result.errors) {
                combined.add_error(e);
            }
            for (const auto& w : result.warnings) {
                combined.add_warning(w);
            }
        }
        return combined;
    }
};

} // namespace ims::validation
