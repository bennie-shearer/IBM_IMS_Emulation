#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Time Utilities
// Version: 3.6.2
// =============================================================================
//
// Provides mainframe-compatible time formats including STCK (Store Clock),
// Julian dates, TOD clock, and various timestamp formats.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ims::time {

// =============================================================================
// Constants
// =============================================================================

// STCK epoch: January 1, 1900 00:00:00 UTC
// Unix epoch: January 1, 1970 00:00:00 UTC
// Difference: 2208988800 seconds (70 years, accounting for leap years)
constexpr Int64 STCK_UNIX_EPOCH_DIFF_SECONDS = 2208988800LL;
constexpr Int64 STCK_UNITS_PER_MICROSECOND = 4096LL;
constexpr Int64 STCK_UNITS_PER_SECOND = 4096LL * 1000000LL;

// =============================================================================
// STCK (Store Clock) - IBM Mainframe Time Format
// =============================================================================

class Stck {
private:
    UInt64 value_;  // 64-bit STCK value
    
public:
    Stck() : value_(0) {}
    explicit Stck(UInt64 value) : value_(value) {}
    
    // Create from system time point
    static Stck from_system_time(SystemTimePoint tp) {
        auto duration = tp.time_since_epoch();
        auto micros = std::chrono::duration_cast<Microseconds>(duration).count();
        
        // Add epoch difference and convert to STCK units
        Int64 total_micros = micros + (STCK_UNIX_EPOCH_DIFF_SECONDS * 1000000LL);
        return Stck(static_cast<UInt64>(total_micros * STCK_UNITS_PER_MICROSECOND));
    }
    
    // Create from current time
    static Stck now() {
        return from_system_time(SystemClock::now());
    }
    
    // Convert to system time point
    SystemTimePoint to_system_time() const {
        Int64 total_micros = static_cast<Int64>(value_ / STCK_UNITS_PER_MICROSECOND);
        total_micros -= (STCK_UNIX_EPOCH_DIFF_SECONDS * 1000000LL);
        return SystemTimePoint(Microseconds(total_micros));
    }
    
    // Get raw value
    UInt64 value() const { return value_; }
    
    // Arithmetic operators
    Stck operator+(const Stck& other) const { return Stck(value_ + other.value_); }
    Stck operator-(const Stck& other) const { return Stck(value_ - other.value_); }
    
    // Comparison operators
    bool operator==(const Stck& other) const { return value_ == other.value_; }
    bool operator!=(const Stck& other) const { return value_ != other.value_; }
    bool operator<(const Stck& other) const { return value_ < other.value_; }
    bool operator<=(const Stck& other) const { return value_ <= other.value_; }
    bool operator>(const Stck& other) const { return value_ > other.value_; }
    bool operator>=(const Stck& other) const { return value_ >= other.value_; }
    
    // Format as hex string
    String to_hex() const {
        return std::format("{:016X}", value_);
    }
    
    // Parse from hex string
    static Optional<Stck> from_hex(StringView hex) {
        if (hex.length() != 16) return std::nullopt;
        try {
            UInt64 value = 0;
            for (char c : hex) {
                value <<= 4;
                if (c >= '0' && c <= '9') {
                    value |= static_cast<UInt64>(c - '0');
                } else if (c >= 'A' && c <= 'F') {
                    value |= static_cast<UInt64>(c - 'A' + 10);
                } else if (c >= 'a' && c <= 'f') {
                    value |= static_cast<UInt64>(c - 'a' + 10);
                } else {
                    return std::nullopt;
                }
            }
            return Stck(value);
        } catch (...) {
            return std::nullopt;
        }
    }
};

// =============================================================================
// Julian Date (IBM Format: YYDDD or YYYYDDD)
// =============================================================================

struct JulianDate {
    UInt16 year;      // Full year (e.g., 2025)
    UInt16 day;       // Day of year (1-366)
    
    JulianDate() : year(1900), day(1) {}
    JulianDate(UInt16 y, UInt16 d) : year(y), day(d) {}
    
    // Create from system time point
    static JulianDate from_system_time(SystemTimePoint tp) {
        std::time_t time = SystemClock::to_time_t(tp);
        std::tm* tm = std::gmtime(&time);
        if (!tm) return JulianDate();
        
        return JulianDate(
            static_cast<UInt16>(tm->tm_year + 1900),
            static_cast<UInt16>(tm->tm_yday + 1)
        );
    }
    
    // Create from current time
    static JulianDate today() {
        return from_system_time(SystemClock::now());
    }
    
    // Convert to Gregorian date
    struct GregorianDate {
        UInt16 year, month, day;
    };
    
    GregorianDate to_gregorian() const {
        // Days in each month (non-leap year)
        static const UInt16 days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        
        bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        
        UInt16 remaining = day;
        UInt16 month = 1;
        
        for (int m = 0; m < 12; ++m) {
            UInt16 days = days_in_month[m];
            if (m == 1 && is_leap) ++days;  // February in leap year
            
            if (remaining <= days) {
                return GregorianDate{year, static_cast<UInt16>(m + 1), remaining};
            }
            remaining -= days;
            ++month;
        }
        
        return GregorianDate{year, 12, 31};
    }
    
    // Format as YYDDD (5 characters)
    String to_yyddd() const {
        return std::format("{:02d}{:03d}", year % 100, day);
    }
    
    // Format as YYYYDDD (7 characters)
    String to_yyyyddd() const {
        return std::format("{:04d}{:03d}", year, day);
    }
    
    // Parse from YYDDD (assumes 2000s for YY < 50, 1900s otherwise)
    static Optional<JulianDate> from_yyddd(StringView str) {
        if (str.length() != 5) return std::nullopt;
        try {
            int yy = std::stoi(String(str.substr(0, 2)));
            int ddd = std::stoi(String(str.substr(2, 3)));
            
            UInt16 year = static_cast<UInt16>(yy < 50 ? 2000 + yy : 1900 + yy);
            if (ddd < 1 || ddd > 366) return std::nullopt;
            
            return JulianDate(year, static_cast<UInt16>(ddd));
        } catch (...) {
            return std::nullopt;
        }
    }
    
    // Parse from YYYYDDD
    static Optional<JulianDate> from_yyyyddd(StringView str) {
        if (str.length() != 7) return std::nullopt;
        try {
            int yyyy = std::stoi(String(str.substr(0, 4)));
            int ddd = std::stoi(String(str.substr(4, 3)));
            
            if (ddd < 1 || ddd > 366) return std::nullopt;
            
            return JulianDate(static_cast<UInt16>(yyyy), static_cast<UInt16>(ddd));
        } catch (...) {
            return std::nullopt;
        }
    }
    
    // Comparison
    bool operator==(const JulianDate& other) const {
        return year == other.year && day == other.day;
    }
    
    bool operator<(const JulianDate& other) const {
        if (year != other.year) return year < other.year;
        return day < other.day;
    }
};

// =============================================================================
// Timestamp Formatting
// =============================================================================

class TimestampFormatter {
public:
    // ISO 8601 format: 2025-12-22T14:30:45.123Z
    static String format_iso8601(SystemTimePoint tp) {
        auto time = SystemClock::to_time_t(tp);
        auto ms = std::chrono::duration_cast<Milliseconds>(
            tp.time_since_epoch()).count() % 1000;
        
        std::tm* tm = std::gmtime(&time);
        if (!tm) return "";
        
        return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}Z",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            static_cast<int>(ms));
    }
    
    // Mainframe format: YYYY-MM-DD-HH.MM.SS.NNNNNN
    static String format_mainframe(SystemTimePoint tp) {
        auto time = SystemClock::to_time_t(tp);
        auto micros = std::chrono::duration_cast<Microseconds>(
            tp.time_since_epoch()).count() % 1000000;
        
        std::tm* tm = std::gmtime(&time);
        if (!tm) return "";
        
        return std::format("{:04d}-{:02d}-{:02d}-{:02d}.{:02d}.{:02d}.{:06d}",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            static_cast<int>(micros));
    }
    
    // SMF format: HHMMSS00 (with hundredths)
    static String format_smf_time(SystemTimePoint tp) {
        auto time = SystemClock::to_time_t(tp);
        auto cs = std::chrono::duration_cast<std::chrono::duration<int, std::centi>>(
            tp.time_since_epoch()).count() % 100;
        
        std::tm* tm = std::gmtime(&time);
        if (!tm) return "";
        
        return std::format("{:02d}{:02d}{:02d}{:02d}",
            tm->tm_hour, tm->tm_min, tm->tm_sec, cs);
    }
    
    // SMF date format: YYYYDDD (Julian)
    static String format_smf_date(SystemTimePoint tp) {
        return JulianDate::from_system_time(tp).to_yyyyddd();
    }
    
    // DB2 timestamp format
    static String format_db2(SystemTimePoint tp) {
        auto time = SystemClock::to_time_t(tp);
        auto micros = std::chrono::duration_cast<Microseconds>(
            tp.time_since_epoch()).count() % 1000000;
        
        std::tm* tm = std::gmtime(&time);
        if (!tm) return "";
        
        return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:06d}",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            static_cast<int>(micros));
    }
    
    // Format as current time
    static String now_iso8601() { return format_iso8601(SystemClock::now()); }
    static String now_mainframe() { return format_mainframe(SystemClock::now()); }
    static String now_db2() { return format_db2(SystemClock::now()); }
};

// =============================================================================
// Timestamp Parsing
// =============================================================================

class TimestampParser {
public:
    // Parse ISO 8601 format
    static Optional<SystemTimePoint> parse_iso8601(StringView str) {
        // Simple parser for YYYY-MM-DDTHH:MM:SS format
        if (str.length() < 19) return std::nullopt;
        
        try {
            std::tm tm{};
            tm.tm_year = std::stoi(String(str.substr(0, 4))) - 1900;
            tm.tm_mon = std::stoi(String(str.substr(5, 2))) - 1;
            tm.tm_mday = std::stoi(String(str.substr(8, 2)));
            tm.tm_hour = std::stoi(String(str.substr(11, 2)));
            tm.tm_min = std::stoi(String(str.substr(14, 2)));
            tm.tm_sec = std::stoi(String(str.substr(17, 2)));
            
            std::time_t time = std::mktime(&tm);
            if (time == -1) return std::nullopt;
            
            // Parse milliseconds if present
            Milliseconds ms(0);
            if (str.length() > 20 && str[19] == '.') {
                Size end = str.find_first_of("Z+-", 20);
                if (end == StringView::npos) end = str.length();
                String ms_str(str.substr(20, end - 20));
                while (ms_str.length() < 3) ms_str += '0';
                ms = Milliseconds(std::stoi(ms_str.substr(0, 3)));
            }
            
            return SystemClock::from_time_t(time) + ms;
        } catch (...) {
            return std::nullopt;
        }
    }
};

// =============================================================================
// Duration Utilities
// =============================================================================

class DurationUtils {
public:
    // Parse duration string like "5m", "2h30m", "100ms"
    static Optional<Milliseconds> parse(StringView str) {
        if (str.empty()) return std::nullopt;
        
        Int64 total_ms = 0;
        Size pos = 0;
        
        while (pos < str.length()) {
            // Skip whitespace
            while (pos < str.length() && std::isspace(str[pos])) ++pos;
            if (pos >= str.length()) break;
            
            // Parse number
            Size num_start = pos;
            while (pos < str.length() && (std::isdigit(str[pos]) || str[pos] == '.')) ++pos;
            if (pos == num_start) return std::nullopt;
            
            double value;
            try {
                value = std::stod(String(str.substr(num_start, pos - num_start)));
            } catch (...) {
                return std::nullopt;
            }
            
            // Parse unit
            Size unit_start = pos;
            while (pos < str.length() && std::isalpha(str[pos])) ++pos;
            StringView unit = str.substr(unit_start, pos - unit_start);
            
            // Convert to milliseconds
            if (unit == "ns" || unit == "nanosecond" || unit == "nanoseconds") {
                total_ms += static_cast<Int64>(value / 1000000.0);
            } else if (unit == "us" || unit == "microsecond" || unit == "microseconds") {
                total_ms += static_cast<Int64>(value / 1000.0);
            } else if (unit == "ms" || unit == "millisecond" || unit == "milliseconds" || unit.empty()) {
                total_ms += static_cast<Int64>(value);
            } else if (unit == "s" || unit == "sec" || unit == "second" || unit == "seconds") {
                total_ms += static_cast<Int64>(value * 1000);
            } else if (unit == "m" || unit == "min" || unit == "minute" || unit == "minutes") {
                total_ms += static_cast<Int64>(value * 60000);
            } else if (unit == "h" || unit == "hr" || unit == "hour" || unit == "hours") {
                total_ms += static_cast<Int64>(value * 3600000);
            } else if (unit == "d" || unit == "day" || unit == "days") {
                total_ms += static_cast<Int64>(value * 86400000);
            } else {
                return std::nullopt;
            }
        }
        
        return Milliseconds(total_ms);
    }
    
    // Format duration as human-readable string
    static String format(Milliseconds ms) {
        Int64 total = ms.count();
        if (total < 1000) {
            return std::format("{}ms", total);
        }
        
        Int64 days = total / 86400000;
        total %= 86400000;
        Int64 hours = total / 3600000;
        total %= 3600000;
        Int64 minutes = total / 60000;
        total %= 60000;
        Int64 seconds = total / 1000;
        Int64 millis = total % 1000;
        
        std::ostringstream oss;
        if (days > 0) oss << days << "d ";
        if (hours > 0) oss << hours << "h ";
        if (minutes > 0) oss << minutes << "m ";
        if (seconds > 0 || millis > 0) {
            if (millis > 0) {
                oss << seconds << "." << std::setw(3) << std::setfill('0') << millis << "s";
            } else {
                oss << seconds << "s";
            }
        }
        
        String result = oss.str();
        // Trim trailing space
        while (!result.empty() && result.back() == ' ') {
            result.pop_back();
        }
        return result.empty() ? "0ms" : result;
    }
};

// =============================================================================
// Convenience Functions
// =============================================================================

inline String current_timestamp_iso() {
    return TimestampFormatter::format_iso8601(SystemClock::now());
}

inline String current_timestamp_mainframe() {
    return TimestampFormatter::format_mainframe(SystemClock::now());
}

inline JulianDate today() {
    return JulianDate::today();
}

inline Stck current_stck() {
    return Stck::now();
}

} // namespace ims::time
