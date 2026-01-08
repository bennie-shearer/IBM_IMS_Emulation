#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Platform Abstraction
// Version: 3.6.2
// =============================================================================

#include "types.hpp"

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define IMS_WINDOWS 1
    #define IMS_PLATFORM_NAME "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
    #define IMS_MACOS 1
    #define IMS_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define IMS_LINUX 1
    #define IMS_PLATFORM_NAME "Linux"
#else
    #define IMS_UNKNOWN_PLATFORM 1
    #define IMS_PLATFORM_NAME "Unknown"
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define IMS_ARCH_X64 1
    #define IMS_ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
    #define IMS_ARCH_X86 1
    #define IMS_ARCH_NAME "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define IMS_ARCH_ARM64 1
    #define IMS_ARCH_NAME "ARM64"
#else
    #define IMS_ARCH_UNKNOWN 1
    #define IMS_ARCH_NAME "Unknown"
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define IMS_COMPILER_MSVC 1
    #define IMS_COMPILER_NAME "MSVC"
    #define IMS_COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
    #define IMS_COMPILER_CLANG 1
    #define IMS_COMPILER_NAME "Clang"
    #define IMS_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
    #define IMS_COMPILER_GCC 1
    #define IMS_COMPILER_NAME "GCC"
    #define IMS_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
    #define IMS_COMPILER_UNKNOWN 1
    #define IMS_COMPILER_NAME "Unknown"
    #define IMS_COMPILER_VERSION 0
#endif

namespace ims {

// =============================================================================
// Platform Information
// =============================================================================

struct PlatformInfo {
    String os_name;
    String arch_name;
    String compiler_name;
    Int32 compiler_version;
    Size page_size;
    Size cache_line_size;
    UInt32 cpu_count;
    UInt64 total_memory;
    
    static PlatformInfo get_current();
    String to_string() const;
};

// =============================================================================
// Platform-specific utilities
// =============================================================================

namespace platform {

// Get current timestamp in nanoseconds
UInt64 get_timestamp_ns();

// Get process ID
UInt32 get_process_id();

// Get thread ID  
UInt64 get_thread_id();

// Get page size
Size get_page_size();

// Get CPU count
UInt32 get_cpu_count();

// Get total system memory
UInt64 get_total_memory();

// Get available memory
UInt64 get_available_memory();

// Sleep for milliseconds
void sleep_ms(UInt32 milliseconds);

// Get environment variable
Optional<String> get_env(const String& name);

// Set environment variable
bool set_env(const String& name, const String& value);

// Get home directory
Path get_home_directory();

// Get temp directory
Path get_temp_directory();

// Get current working directory
Path get_current_directory();

// Create directory recursively
bool create_directories(const Path& path);

// File exists check
bool file_exists(const Path& path);

// Directory exists check
bool directory_exists(const Path& path);

// Get file size
UInt64 get_file_size(const Path& path);

} // namespace platform

} // namespace ims
