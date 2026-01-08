#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Diagnostic Utilities
// Version: 3.6.2
// =============================================================================
//
// Provides hex dumps, memory dumps, diagnostic information collection,
// and debugging utilities for mainframe emulation.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include "platform.hpp"
#include <iomanip>
#include <sstream>
#include <cstring>

namespace ims::diagnostics {

// =============================================================================
// Hex Dump Utility
// =============================================================================

struct HexDumpOptions {
    Size bytes_per_line = 16;
    Size group_size = 4;
    bool show_offset = true;
    bool show_ascii = true;
    bool uppercase = true;
    UInt64 base_offset = 0;
};

class HexDump {
public:
    static constexpr Size DEFAULT_BYTES_PER_LINE = 16;
    static constexpr Size DEFAULT_GROUP_SIZE = 4;
    
    using Options = HexDumpOptions;
    
    static String format(const void* data, Size size, const Options& opts = Options{}) {
        if (!data || size == 0) {
            return "(empty)";
        }
        
        std::ostringstream oss;
        const Byte* bytes = static_cast<const Byte*>(data);
        const char* hex_chars = opts.uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
        
        for (Size i = 0; i < size; i += opts.bytes_per_line) {
            // Offset
            if (opts.show_offset) {
                oss << std::setw(8) << std::setfill('0') << std::hex 
                    << (opts.base_offset + i) << "  ";
            }
            
            // Hex bytes
            String hex_part;
            String ascii_part;
            
            for (Size j = 0; j < opts.bytes_per_line; ++j) {
                if (j > 0 && opts.group_size > 0 && j % opts.group_size == 0) {
                    hex_part += ' ';
                }
                
                if (i + j < size) {
                    Byte b = bytes[i + j];
                    hex_part += hex_chars[(b >> 4) & 0x0F];
                    hex_part += hex_chars[b & 0x0F];
                    hex_part += ' ';
                    
                    // ASCII representation
                    if (b >= 0x20 && b < 0x7F) {
                        ascii_part += static_cast<char>(b);
                    } else {
                        ascii_part += '.';
                    }
                } else {
                    hex_part += "   ";
                    ascii_part += ' ';
                }
            }
            
            oss << hex_part;
            
            if (opts.show_ascii) {
                oss << " |" << ascii_part << "|";
            }
            
            if (i + opts.bytes_per_line < size) {
                oss << '\n';
            }
        }
        
        return oss.str();
    }
    
    static String format(const ByteBuffer& buffer, Options opts = Options{}) {
        return format(buffer.data(), buffer.size(), opts);
    }
    
    static String format(StringView str, Options opts = Options{}) {
        return format(str.data(), str.size(), opts);
    }
};

// =============================================================================
// EBCDIC/ASCII Translation Tables
// =============================================================================

class EbcdicTranslator {
public:
    // ASCII to EBCDIC translation table
    static constexpr Byte ASCII_TO_EBCDIC[256] = {
        0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
        0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
        0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
        0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
        0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
        0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xAD,0xE0,0xBD,0x5F,0x6D,
        0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
        0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0xA1,0x07,
        0x20,0x21,0x22,0x23,0x24,0x15,0x06,0x17,0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B,
        0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xFF,
        0x41,0xAA,0x4A,0xB1,0x9F,0xB2,0x6A,0xB5,0xBB,0xB4,0x9A,0x8A,0xB0,0xCA,0xAF,0xBC,
        0x90,0x8F,0xEA,0xFA,0xBE,0xA0,0xB6,0xB3,0x9D,0xDA,0x9B,0x8B,0xB7,0xB8,0xB9,0xAB,
        0x64,0x65,0x62,0x66,0x63,0x67,0x9E,0x68,0x74,0x71,0x72,0x73,0x78,0x75,0x76,0x77,
        0xAC,0x69,0xED,0xEE,0xEB,0xEF,0xEC,0xBF,0x80,0xFD,0xFE,0xFB,0xFC,0xBA,0xAE,0x59,
        0x44,0x45,0x42,0x46,0x43,0x47,0x9C,0x48,0x54,0x51,0x52,0x53,0x58,0x55,0x56,0x57,
        0x8C,0x49,0xCD,0xCE,0xCB,0xCF,0xCC,0xE1,0x70,0xDD,0xDE,0xDB,0xDC,0x8D,0x8E,0xDF
    };
    
    // EBCDIC to ASCII translation table
    static constexpr Byte EBCDIC_TO_ASCII[256] = {
        0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
        0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
        0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
        0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
        0x20,0xA0,0xE2,0xE4,0xE0,0xE1,0xE3,0xE5,0xE7,0xF1,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
        0x26,0xE9,0xEA,0xEB,0xE8,0xED,0xEE,0xEF,0xEC,0xDF,0x21,0x24,0x2A,0x29,0x3B,0x5E,
        0x2D,0x2F,0xC2,0xC4,0xC0,0xC1,0xC3,0xC5,0xC7,0xD1,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
        0xF8,0xC9,0xCA,0xCB,0xC8,0xCD,0xCE,0xCF,0xCC,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
        0xD8,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xAB,0xBB,0xF0,0xFD,0xFE,0xB1,
        0xB0,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xAA,0xBA,0xE6,0xB8,0xC6,0xA4,
        0xB5,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xA1,0xBF,0xD0,0x5B,0xDE,0xAE,
        0xAC,0xA3,0xA5,0xB7,0xA9,0xA7,0xB6,0xBC,0xBD,0xBE,0xDD,0xA8,0xAF,0x5D,0xB4,0xD7,
        0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xAD,0xF4,0xF6,0xF2,0xF3,0xF5,
        0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xB9,0xFB,0xFC,0xF9,0xFA,0xFF,
        0x5C,0xF7,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xB2,0xD4,0xD6,0xD2,0xD3,0xD5,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xB3,0xDB,0xDC,0xD9,0xDA,0x9F
    };
    
    static Byte to_ebcdic(Byte ascii) {
        return ASCII_TO_EBCDIC[ascii];
    }
    
    static Byte to_ascii(Byte ebcdic) {
        return EBCDIC_TO_ASCII[ebcdic];
    }
    
    static ByteBuffer ascii_to_ebcdic(const ByteBuffer& ascii) {
        ByteBuffer result;
        result.reserve(ascii.size());
        for (Byte b : ascii) {
            result.push_back(ASCII_TO_EBCDIC[b]);
        }
        return result;
    }
    
    static ByteBuffer ebcdic_to_ascii(const ByteBuffer& ebcdic) {
        ByteBuffer result;
        result.reserve(ebcdic.size());
        for (Byte b : ebcdic) {
            result.push_back(EBCDIC_TO_ASCII[b]);
        }
        return result;
    }
    
    static String ascii_string_to_ebcdic(StringView ascii) {
        String result;
        result.reserve(ascii.size());
        for (char c : ascii) {
            result += static_cast<char>(ASCII_TO_EBCDIC[static_cast<Byte>(c)]);
        }
        return result;
    }
    
    static String ebcdic_string_to_ascii(StringView ebcdic) {
        String result;
        result.reserve(ebcdic.size());
        for (char c : ebcdic) {
            result += static_cast<char>(EBCDIC_TO_ASCII[static_cast<Byte>(c)]);
        }
        return result;
    }
};

// =============================================================================
// Diagnostic Information Collector
// =============================================================================

struct DiagnosticInfo {
    String component;
    String version;
    String build_date;
    String platform;
    String compiler;
    UInt64 memory_used{0};
    UInt64 memory_available{0};
    UInt64 uptime_seconds{0};
    Map<String, String> properties;
    Vector<String> warnings;
    Vector<String> errors;
    SystemTimePoint collected_at;
    
    DiagnosticInfo() : collected_at(SystemClock::now()) {}
    
    String to_string() const {
        std::ostringstream oss;
        oss << "=== Diagnostic Information ===\n";
        oss << "Component: " << component << "\n";
        oss << "Version: " << version << "\n";
        oss << "Build Date: " << build_date << "\n";
        oss << "Platform: " << platform << "\n";
        oss << "Compiler: " << compiler << "\n";
        oss << "Memory Used: " << memory_used << " bytes\n";
        oss << "Memory Available: " << memory_available << " bytes\n";
        oss << "Uptime: " << uptime_seconds << " seconds\n";
        
        if (!properties.empty()) {
            oss << "\nProperties:\n";
            for (const auto& [key, value] : properties) {
                oss << "  " << key << ": " << value << "\n";
            }
        }
        
        if (!warnings.empty()) {
            oss << "\nWarnings:\n";
            for (const auto& w : warnings) {
                oss << "  - " << w << "\n";
            }
        }
        
        if (!errors.empty()) {
            oss << "\nErrors:\n";
            for (const auto& e : errors) {
                oss << "  - " << e << "\n";
            }
        }
        
        return oss.str();
    }
};

class DiagnosticCollector {
private:
    SystemTimePoint start_time_;
    Vector<String> log_entries_;
    mutable Mutex mutex_;
    
public:
    DiagnosticCollector() : start_time_(SystemClock::now()) {}
    
    void log(const String& entry) {
        LockGuard<Mutex> lock(mutex_);
        auto now = SystemClock::now();
        auto elapsed = std::chrono::duration_cast<Milliseconds>(now - start_time_).count();
        log_entries_.push_back(std::format("[{:>8}ms] {}", elapsed, entry));
    }
    
    DiagnosticInfo collect(const String& component) const {
        DiagnosticInfo info;
        info.component = component;
        info.version = IMS_VERSION;
        info.build_date = __DATE__ " " __TIME__;
        
#ifdef _WIN32
        info.platform = "Windows";
#elif defined(__APPLE__)
        info.platform = "macOS";
#else
        info.platform = "Linux";
#endif

#ifdef __GNUC__
        info.compiler = std::format("GCC {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
        info.compiler = std::format("MSVC {}", _MSC_VER);
#elif defined(__clang__)
        info.compiler = std::format("Clang {}.{}.{}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#else
        info.compiler = "Unknown";
#endif
        
        auto elapsed = std::chrono::duration_cast<Seconds>(SystemClock::now() - start_time_);
        info.uptime_seconds = static_cast<UInt64>(elapsed.count());
        
        return info;
    }
    
    Vector<String> get_log() const {
        LockGuard<Mutex> lock(mutex_);
        return log_entries_;
    }
    
    void clear_log() {
        LockGuard<Mutex> lock(mutex_);
        log_entries_.clear();
    }
};

// Global diagnostic collector
inline DiagnosticCollector& global_diagnostics() {
    static DiagnosticCollector collector;
    return collector;
}

// =============================================================================
// Memory Dump Utility
// =============================================================================

class MemoryDump {
public:
    struct Region {
        String name;
        const void* address;
        Size size;
        String description;
    };
    
    static String dump_regions(const Vector<Region>& regions) {
        std::ostringstream oss;
        oss << "=== Memory Dump ===\n\n";
        
        for (const auto& region : regions) {
            oss << "Region: " << region.name << "\n";
            oss << "Address: " << region.address << "\n";
            oss << "Size: " << region.size << " bytes\n";
            if (!region.description.empty()) {
                oss << "Description: " << region.description << "\n";
            }
            oss << "\n" << HexDump::format(region.address, region.size) << "\n\n";
        }
        
        return oss.str();
    }
};

// =============================================================================
// Debug Trace Utility
// =============================================================================

class DebugTrace {
private:
    static inline AtomicBool enabled_{false};
    static inline AtomicUInt32 trace_level_{0};
    
public:
    enum Level : UInt32 {
        NONE = 0,
        ERROR = 1,
        WARNING = 2,
        INFO = 3,
        DEBUG = 4,
        VERBOSE = 5
    };
    
    static void enable(UInt32 level = INFO) {
        enabled_.store(true, std::memory_order_relaxed);
        trace_level_.store(level, std::memory_order_relaxed);
    }
    
    static void disable() {
        enabled_.store(false, std::memory_order_relaxed);
    }
    
    static bool is_enabled(UInt32 level = INFO) {
        return enabled_.load(std::memory_order_relaxed) && 
               level <= trace_level_.load(std::memory_order_relaxed);
    }
    
    template<typename... Args>
    static void trace(UInt32 level, const String& format_str, Args&&... args) {
        if (!is_enabled(level)) return;
        
        String prefix;
        switch (level) {
            case ERROR:   prefix = "[ERROR]   "; break;
            case WARNING: prefix = "[WARNING] "; break;
            case INFO:    prefix = "[INFO]    "; break;
            case DEBUG:   prefix = "[DEBUG]   "; break;
            case VERBOSE: prefix = "[VERBOSE] "; break;
            default:      prefix = "[TRACE]   "; break;
        }
        
        String message = std::vformat(format_str, std::make_format_args(args...));
        global_diagnostics().log(prefix + message);
    }
};

// Convenience macros
#define IMS_TRACE_ERROR(...)   ims::diagnostics::DebugTrace::trace(ims::diagnostics::DebugTrace::ERROR, __VA_ARGS__)
#define IMS_TRACE_WARNING(...) ims::diagnostics::DebugTrace::trace(ims::diagnostics::DebugTrace::WARNING, __VA_ARGS__)
#define IMS_TRACE_INFO(...)    ims::diagnostics::DebugTrace::trace(ims::diagnostics::DebugTrace::INFO, __VA_ARGS__)
#define IMS_TRACE_DEBUG(...)   ims::diagnostics::DebugTrace::trace(ims::diagnostics::DebugTrace::DEBUG, __VA_ARGS__)
#define IMS_TRACE_VERBOSE(...) ims::diagnostics::DebugTrace::trace(ims::diagnostics::DebugTrace::VERBOSE, __VA_ARGS__)

} // namespace ims::diagnostics
