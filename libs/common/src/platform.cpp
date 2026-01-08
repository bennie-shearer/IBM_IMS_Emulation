// =============================================================================
// IBM IMS Emulation Enterprise - Platform Implementation
// Version: 3.6.2
// =============================================================================

#include "ims/common/platform.hpp"
#include <cstdlib>
#include <thread>

#ifdef IMS_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <processthreadsapi.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #include <sys/stat.h>
    #include <time.h>
    #ifdef IMS_LINUX
        #include <sys/sysinfo.h>
    #endif
    #ifdef IMS_MACOS
        #include <sys/sysctl.h>
        #include <mach/mach.h>
    #endif
#endif

namespace ims {

// =============================================================================
// PlatformInfo Implementation
// =============================================================================

PlatformInfo PlatformInfo::get_current() {
    PlatformInfo info;
    info.os_name = IMS_PLATFORM_NAME;
    info.arch_name = IMS_ARCH_NAME;
    info.compiler_name = IMS_COMPILER_NAME;
    info.compiler_version = IMS_COMPILER_VERSION;
    info.page_size = platform::get_page_size();
    info.cache_line_size = 64; // Common default
    info.cpu_count = platform::get_cpu_count();
    info.total_memory = platform::get_total_memory();
    return info;
}

String PlatformInfo::to_string() const {
    return std::format(
        "OS: {}, Arch: {}, Compiler: {} ({}), CPUs: {}, Memory: {} MB",
        os_name, arch_name, compiler_name, compiler_version,
        cpu_count, total_memory / (1024 * 1024));
}

// =============================================================================
// Platform Utilities Implementation
// =============================================================================

namespace platform {

UInt64 get_timestamp_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    return static_cast<UInt64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count());
}

UInt32 get_process_id() {
#ifdef IMS_WINDOWS
    return static_cast<UInt32>(GetCurrentProcessId());
#else
    return static_cast<UInt32>(getpid());
#endif
}

UInt64 get_thread_id() {
#ifdef IMS_WINDOWS
    return static_cast<UInt64>(GetCurrentThreadId());
#else
    return static_cast<UInt64>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}

Size get_page_size() {
#ifdef IMS_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return static_cast<Size>(si.dwPageSize);
#else
    return static_cast<Size>(sysconf(_SC_PAGESIZE));
#endif
}

UInt32 get_cpu_count() {
    return std::thread::hardware_concurrency();
}

UInt64 get_total_memory() {
#ifdef IMS_WINDOWS
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
#elif defined(IMS_LINUX)
    struct sysinfo si;
    sysinfo(&si);
    return static_cast<UInt64>(si.totalram) * si.mem_unit;
#elif defined(IMS_MACOS)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    UInt64 memory = 0;
    size_t len = sizeof(memory);
    sysctl(mib, 2, &memory, &len, nullptr, 0);
    return memory;
#else
    return 0;
#endif
}

UInt64 get_available_memory() {
#ifdef IMS_WINDOWS
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullAvailPhys;
#elif defined(IMS_LINUX)
    struct sysinfo si;
    sysinfo(&si);
    return static_cast<UInt64>(si.freeram) * si.mem_unit;
#elif defined(IMS_MACOS)
    vm_size_t page_size;
    mach_port_t mach_port = mach_host_self();
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);
    host_page_size(mach_port, &page_size);
    if (host_statistics64(mach_port, HOST_VM_INFO64, 
                          reinterpret_cast<host_info64_t>(&vm_stats), &count) == KERN_SUCCESS) {
        return static_cast<UInt64>(vm_stats.free_count) * page_size;
    }
    return 0;
#else
    return 0;
#endif
}

void sleep_ms(UInt32 milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

Optional<String> get_env(const String& name) {
#ifdef IMS_WINDOWS
    char buffer[32767];
    DWORD result = GetEnvironmentVariableA(name.c_str(), buffer, sizeof(buffer));
    if (result > 0 && result < sizeof(buffer)) {
        return String(buffer);
    }
    return std::nullopt;
#else
    const char* value = std::getenv(name.c_str());
    if (value) {
        return String(value);
    }
    return std::nullopt;
#endif
}

bool set_env(const String& name, const String& value) {
#ifdef IMS_WINDOWS
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

Path get_home_directory() {
#ifdef IMS_WINDOWS
    auto home = get_env("USERPROFILE");
    if (home) return Path(*home);
    auto drive = get_env("HOMEDRIVE");
    auto path = get_env("HOMEPATH");
    if (drive && path) return Path(*drive + *path);
    return Path("C:\\");
#else
    auto home = get_env("HOME");
    if (home) return Path(*home);
    struct passwd* pw = getpwuid(getuid());
    if (pw) return Path(pw->pw_dir);
    return Path("/tmp");
#endif
}

Path get_temp_directory() {
#ifdef IMS_WINDOWS
    char buffer[MAX_PATH];
    GetTempPathA(MAX_PATH, buffer);
    return Path(buffer);
#else
    auto tmp = get_env("TMPDIR");
    if (tmp) return Path(*tmp);
    return Path("/tmp");
#endif
}

Path get_current_directory() {
    return std::filesystem::current_path();
}

bool create_directories(const Path& path) {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec);
}

bool file_exists(const Path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

bool directory_exists(const Path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
}

UInt64 get_file_size(const Path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    return ec ? 0 : static_cast<UInt64>(size);
}

} // namespace platform

} // namespace ims
