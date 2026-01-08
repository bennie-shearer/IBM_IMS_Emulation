#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - String Utilities
// Version: 3.6.3
// =============================================================================
//
// Provides mainframe-compatible string operations including padding,
// trimming, case conversion, and field formatting.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>

namespace ims::strings {

// =============================================================================
// Padding Operations (Mainframe Style)
// =============================================================================

/// Pad string on the right with spaces (left-justified)
inline String pad_right(StringView str, Size width, char pad_char = ' ') {
    if (str.length() >= width) {
        return String(str.substr(0, width));
    }
    String result(str);
    result.append(width - str.length(), pad_char);
    return result;
}

/// Pad string on the left with spaces (right-justified)
inline String pad_left(StringView str, Size width, char pad_char = ' ') {
    if (str.length() >= width) {
        return String(str.substr(str.length() - width));
    }
    String result(width - str.length(), pad_char);
    result.append(str);
    return result;
}

/// Pad number with leading zeros
inline String pad_zero(UInt64 value, Size width) {
    String result = std::to_string(value);
    if (result.length() < width) {
        result.insert(0, width - result.length(), '0');
    }
    return result;
}

/// Center string within width
inline String center(StringView str, Size width, char pad_char = ' ') {
    if (str.length() >= width) {
        return String(str.substr(0, width));
    }
    Size total_padding = width - str.length();
    Size left_padding = total_padding / 2;
    Size right_padding = total_padding - left_padding;
    String result(left_padding, pad_char);
    result.append(str);
    result.append(right_padding, pad_char);
    return result;
}

// =============================================================================
// Trimming Operations
// =============================================================================

/// Trim whitespace from left
inline StringView trim_left(StringView str) {
    auto it = std::find_if(str.begin(), str.end(), [](unsigned char c) {
        return !std::isspace(c);
    });
    return str.substr(static_cast<Size>(it - str.begin()));
}

/// Trim whitespace from right
inline StringView trim_right(StringView str) {
    auto it = std::find_if(str.rbegin(), str.rend(), [](unsigned char c) {
        return !std::isspace(c);
    });
    return str.substr(0, static_cast<Size>(str.rend() - it));
}

/// Trim whitespace from both ends
inline StringView trim(StringView str) {
    return trim_right(trim_left(str));
}

/// Trim specific character from both ends
inline StringView trim_char(StringView str, char c) {
    Size start = 0;
    Size end = str.length();
    while (start < end && str[start] == c) ++start;
    while (end > start && str[end - 1] == c) --end;
    return str.substr(start, end - start);
}

// =============================================================================
// Case Conversion
// =============================================================================

/// Convert string to uppercase
inline String to_upper(StringView str) {
    String result;
    result.reserve(str.length());
    for (char c : str) {
        result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

/// Convert string to lowercase
inline String to_lower(StringView str) {
    String result;
    result.reserve(str.length());
    for (char c : str) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

/// Capitalize first letter of each word
inline String capitalize(StringView str) {
    String result;
    result.reserve(str.length());
    bool new_word = true;
    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            new_word = true;
            result += c;
        } else if (new_word) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            new_word = false;
        } else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return result;
}

// =============================================================================
// Splitting and Joining
// =============================================================================

/// Split string by delimiter
inline Vector<String> split(StringView str, char delimiter) {
    Vector<String> result;
    Size start = 0;
    for (Size i = 0; i < str.length(); ++i) {
        if (str[i] == delimiter) {
            result.emplace_back(str.substr(start, i - start));
            start = i + 1;
        }
    }
    result.emplace_back(str.substr(start));
    return result;
}

/// Split string by string delimiter
inline Vector<String> split(StringView str, StringView delimiter) {
    Vector<String> result;
    Size start = 0;
    Size pos;
    while ((pos = str.find(delimiter, start)) != StringView::npos) {
        result.emplace_back(str.substr(start, pos - start));
        start = pos + delimiter.length();
    }
    result.emplace_back(str.substr(start));
    return result;
}

/// Join strings with delimiter
inline String join(const Vector<String>& parts, StringView delimiter) {
    if (parts.empty()) return "";
    
    Size total_size = 0;
    for (const auto& part : parts) {
        total_size += part.length();
    }
    total_size += delimiter.length() * (parts.size() - 1);
    
    String result;
    result.reserve(total_size);
    
    for (Size i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            result.append(delimiter);
        }
        result.append(parts[i]);
    }
    
    return result;
}

// =============================================================================
// Search and Replace
// =============================================================================

/// Replace all occurrences of a substring
inline String replace_all(StringView str, StringView from, StringView to) {
    if (from.empty()) return String(str);
    
    String result;
    result.reserve(str.length());
    
    Size pos = 0;
    Size last_pos = 0;
    while ((pos = str.find(from, last_pos)) != StringView::npos) {
        result.append(str.substr(last_pos, pos - last_pos));
        result.append(to);
        last_pos = pos + from.length();
    }
    result.append(str.substr(last_pos));
    
    return result;
}

/// Check if string starts with prefix
inline bool starts_with(StringView str, StringView prefix) {
    return str.length() >= prefix.length() && 
           str.substr(0, prefix.length()) == prefix;
}

/// Check if string ends with suffix
inline bool ends_with(StringView str, StringView suffix) {
    return str.length() >= suffix.length() && 
           str.substr(str.length() - suffix.length()) == suffix;
}

/// Check if string contains substring
inline bool contains(StringView str, StringView substr) {
    return str.find(substr) != StringView::npos;
}

// =============================================================================
// Mainframe Field Formatting
// =============================================================================

/// Format as COBOL PIC 9 numeric field
inline String format_pic9(Int64 value, Size width) {
    bool negative = value < 0;
    UInt64 abs_value = negative ? static_cast<UInt64>(-value) : static_cast<UInt64>(value);
    String result = pad_zero(abs_value, width);
    if (negative && !result.empty()) {
        // COBOL uses overpunch for negative: last digit gets modified
        char last = result.back();
        if (last >= '0' && last <= '9') {
            // Overpunch encoding: 0->}, 1->J, 2->K, ..., 9->R
            static const char overpunch[] = "}JKLMNOPQR";
            result.back() = overpunch[last - '0'];
        }
    }
    return result;
}

/// Format as packed decimal (COMP-3)
inline ByteBuffer format_packed(Int64 value, Size bytes) {
    bool negative = value < 0;
    UInt64 abs_value = negative ? static_cast<UInt64>(-value) : static_cast<UInt64>(value);
    
    // Each byte holds 2 decimal digits, last nibble is sign
    Size digits = (bytes * 2) - 1;
    String num_str = pad_zero(abs_value, digits);
    
    ByteBuffer result(bytes, 0);
    Size digit_pos = 0;
    
    for (Size i = 0; i < bytes; ++i) {
        Byte high = 0, low = 0;
        
        if (digit_pos < num_str.length()) {
            high = static_cast<Byte>(num_str[digit_pos++] - '0');
        }
        
        if (i == bytes - 1) {
            // Last byte: low nibble is sign (C=positive, D=negative)
            low = negative ? 0x0D : 0x0C;
        } else if (digit_pos < num_str.length()) {
            low = static_cast<Byte>(num_str[digit_pos++] - '0');
        }
        
        result[i] = static_cast<Byte>((high << 4) | low);
    }
    
    return result;
}

/// Parse packed decimal (COMP-3) to Int64
inline Int64 parse_packed(const ByteBuffer& packed) {
    if (packed.empty()) return 0;
    
    Int64 result = 0;
    bool negative = false;
    
    for (Size i = 0; i < packed.size(); ++i) {
        Byte high = (packed[i] >> 4) & 0x0F;
        Byte low = packed[i] & 0x0F;
        
        result = result * 10 + high;
        
        if (i == packed.size() - 1) {
            // Last byte: low nibble is sign
            negative = (low == 0x0D || low == 0x0B);
        } else {
            result = result * 10 + low;
        }
    }
    
    return negative ? -result : result;
}

/// Format zoned decimal
inline String format_zoned(Int64 value, Size width) {
    bool negative = value < 0;
    UInt64 abs_value = negative ? static_cast<UInt64>(-value) : static_cast<UInt64>(value);
    String result = pad_zero(abs_value, width);
    
    // For zoned decimal, add zone bits (0xF) to each digit except last
    // Last digit gets sign zone (0xC for +, 0xD for -)
    // This returns printable ASCII representation for simplicity
    if (negative && !result.empty()) {
        char last = result.back();
        if (last >= '0' && last <= '9') {
            result.back() = static_cast<char>('p' + (last - '0'));  // 'p'-'y' for negative
        }
    }
    
    return result;
}

// =============================================================================
// Parsing Utilities
// =============================================================================

/// Parse integer from string (with error handling)
inline Optional<Int64> parse_int(StringView str) {
    Int64 result;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    if (ec == std::errc()) {
        return result;
    }
    return std::nullopt;
}

/// Parse unsigned integer from string
inline Optional<UInt64> parse_uint(StringView str) {
    UInt64 result;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    if (ec == std::errc()) {
        return result;
    }
    return std::nullopt;
}

/// Parse double from string
inline Optional<Float64> parse_double(StringView str) {
    try {
        Size pos;
        Float64 result = std::stod(String(str), &pos);
        if (pos == str.length()) {
            return result;
        }
    } catch (...) {}
    return std::nullopt;
}

// =============================================================================
// Formatting Utilities
// =============================================================================

/// Format bytes as human-readable size
inline String format_bytes(UInt64 bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        ++unit_index;
    }
    
    if (unit_index == 0) {
        return std::format("{} {}", bytes, units[unit_index]);
    }
    return std::format("{:.2f} {}", size, units[unit_index]);
}

/// Format duration as human-readable string
inline String format_duration(Milliseconds ms) {
    auto total_ms = ms.count();
    
    if (total_ms < 1000) {
        return std::format("{}ms", total_ms);
    }
    
    auto seconds = total_ms / 1000;
    auto remaining_ms = total_ms % 1000;
    
    if (seconds < 60) {
        return std::format("{}.{:03d}s", seconds, remaining_ms);
    }
    
    auto minutes = seconds / 60;
    auto remaining_sec = seconds % 60;
    
    if (minutes < 60) {
        return std::format("{}m {}s", minutes, remaining_sec);
    }
    
    auto hours = minutes / 60;
    auto remaining_min = minutes % 60;
    
    return std::format("{}h {}m {}s", hours, remaining_min, remaining_sec);
}

} // namespace ims::strings
