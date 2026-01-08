/**
 * @file file_utils.hpp
 * @brief Cross-platform file system utilities
 * @version 3.6.2
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_FILE_UTILS_HPP
#define IMS_COMMON_FILE_UTILS_HPP

#include "types.hpp"
#include "result.hpp"
#include <fstream>
#include <optional>
#include <sstream>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <direct.h>
    #define IMS_PATH_SEPARATOR '\\'
    #define IMS_PATH_SEPARATOR_STR "\\"
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/types.h>
    #define IMS_PATH_SEPARATOR '/'
    #define IMS_PATH_SEPARATOR_STR "/"
#endif

namespace ims::common::file_utils {

/**
 * @brief File information structure
 */
struct FileInfo {
    String path;
    UInt64 size;
    time_t modified_time;
    bool is_directory;
    bool is_regular_file;
    bool is_readable;
    bool is_writable;
};

/**
 * @brief Check if a file exists
 */
inline bool file_exists(const String& path) {
#if defined(_WIN32) || defined(_WIN64)
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (stat(path.c_str(), &st) == 0) && S_ISREG(st.st_mode);
#endif
}

/**
 * @brief Check if a directory exists
 */
inline bool directory_exists(const String& path) {
#if defined(_WIN32) || defined(_WIN64)
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
#endif
}

/**
 * @brief Check if a path exists (file or directory)
 */
inline bool path_exists(const String& path) {
#if defined(_WIN32) || defined(_WIN64)
    DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#endif
}

/**
 * @brief Get file size in bytes
 */
inline std::optional<UInt64> file_size(const String& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return std::nullopt;
    return static_cast<UInt64>(st.st_size);
}

/**
 * @brief Get file modification time
 */
inline std::optional<time_t> file_modified_time(const String& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return std::nullopt;
    return st.st_mtime;
}

/**
 * @brief Get file information
 */
inline std::optional<FileInfo> get_file_info(const String& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return std::nullopt;
    
    FileInfo info;
    info.path = path;
    info.size = static_cast<UInt64>(st.st_size);
    info.modified_time = st.st_mtime;
    info.is_directory = S_ISDIR(st.st_mode);
    info.is_regular_file = S_ISREG(st.st_mode);
    info.is_readable = (access(path.c_str(), R_OK) == 0);
    info.is_writable = (access(path.c_str(), W_OK) == 0);
    
    return info;
}

/**
 * @brief Join path components
 */
inline String path_join(const String& base, const String& append) {
    if (base.empty()) return append;
    if (append.empty()) return base;
    
    char last = base.back();
    if (last == '/' || last == '\\') {
        return base + append;
    }
    return base + IMS_PATH_SEPARATOR_STR + append;
}

/**
 * @brief Variadic path join
 */
template<typename... Args>
String path_join(const String& first, const String& second, Args... rest) {
    return path_join(path_join(first, second), rest...);
}

/**
 * @brief Get parent directory
 */
inline String parent_path(const String& path) {
    Size pos = path.find_last_of("/\\");
    if (pos == String::npos) return ".";
    if (pos == 0) return path.substr(0, 1);
    return path.substr(0, pos);
}

/**
 * @brief Get filename from path
 */
inline String filename(const String& path) {
    Size pos = path.find_last_of("/\\");
    if (pos == String::npos) return path;
    return path.substr(pos + 1);
}

/**
 * @brief Get file extension (including dot)
 */
inline String extension(const String& path) {
    String name = filename(path);
    Size pos = name.rfind('.');
    if (pos == String::npos || pos == 0) return "";
    return name.substr(pos);
}

/**
 * @brief Get filename without extension
 */
inline String stem(const String& path) {
    String name = filename(path);
    Size pos = name.rfind('.');
    if (pos == String::npos || pos == 0) return name;
    return name.substr(0, pos);
}

/**
 * @brief Normalize path separators
 */
inline String normalize_path(const String& path) {
    String result = path;
    for (char& c : result) {
        if (c == '/' || c == '\\') {
            c = IMS_PATH_SEPARATOR;
        }
    }
    return result;
}

/**
 * @brief Create a directory
 */
inline bool create_directory(const String& path) {
#if defined(_WIN32) || defined(_WIN64)
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

/**
 * @brief Create directories recursively
 */
inline bool create_directories(const String& path) {
    if (directory_exists(path)) return true;
    
    // Create parent first
    String parent = parent_path(path);
    if (!parent.empty() && parent != path) {
        if (!create_directories(parent)) return false;
    }
    
    return create_directory(path);
}

/**
 * @brief Remove a file
 */
inline bool remove_file(const String& path) {
    return std::remove(path.c_str()) == 0;
}

/**
 * @brief Remove an empty directory
 */
inline bool remove_directory(const String& path) {
#if defined(_WIN32) || defined(_WIN64)
    return RemoveDirectoryA(path.c_str()) != 0;
#else
    return rmdir(path.c_str()) == 0;
#endif
}

/**
 * @brief Rename/move a file
 */
inline bool rename_file(const String& old_path, const String& new_path) {
    return std::rename(old_path.c_str(), new_path.c_str()) == 0;
}

/**
 * @brief Copy a file
 */
inline ResultE<void> copy_file(const String& source, const String& dest) {
    std::ifstream src(source, std::ios::binary);
    if (!src) {
        return VoidResult::err("Failed to open source file: " + source);
    }
    
    std::ofstream dst(dest, std::ios::binary);
    if (!dst) {
        return VoidResult::err("Failed to create destination file: " + dest);
    }
    
    dst << src.rdbuf();
    
    if (!dst) {
        return VoidResult::err("Failed to write to destination file: " + dest);
    }
    
    return VoidResult::ok();
}

/**
 * @brief Read entire file as string
 */
inline ResultE<String> read_file(const String& path) {
    std::ifstream file(path, std::ios::in);
    if (!file) {
        return ResultE<String>::err("Failed to open file: " + path);
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    
    if (!file && !file.eof()) {
        return ResultE<String>::err("Failed to read file: " + path);
    }
    
    return ResultE<String>::ok(ss.str());
}

/**
 * @brief Read entire file as binary
 */
inline ResultE<std::vector<Byte>> read_binary_file(const String& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return ResultE<std::vector<Byte>>::err("Failed to open file: " + path);
    }
    
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<Byte> buffer(static_cast<Size>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return ResultE<std::vector<Byte>>::err("Failed to read file: " + path);
    }
    
    return ResultE<std::vector<Byte>>::ok(std::move(buffer));
}

/**
 * @brief Write string to file
 */
inline ResultE<void> write_file(const String& path, const String& content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file) {
        return VoidResult::err("Failed to create file: " + path);
    }
    
    file << content;
    
    if (!file) {
        return VoidResult::err("Failed to write file: " + path);
    }
    
    return VoidResult::ok();
}

/**
 * @brief Write binary data to file
 */
inline ResultE<void> write_binary_file(const String& path, const std::vector<Byte>& data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return VoidResult::err("Failed to create file: " + path);
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    if (!file) {
        return VoidResult::err("Failed to write file: " + path);
    }
    
    return VoidResult::ok();
}

/**
 * @brief Append string to file
 */
inline ResultE<void> append_file(const String& path, const String& content) {
    std::ofstream file(path, std::ios::out | std::ios::app);
    if (!file) {
        return VoidResult::err("Failed to open file for append: " + path);
    }
    
    file << content;
    
    if (!file) {
        return VoidResult::err("Failed to append to file: " + path);
    }
    
    return VoidResult::ok();
}

/**
 * @brief Atomic file write (write to temp, then rename)
 */
inline ResultE<void> atomic_write_file(const String& path, const String& content) {
    // Generate random suffix for temp file
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);
    
    String temp_path = path + ".tmp." + std::to_string(dis(gen));
    
    auto result = write_file(temp_path, content);
    if (!result) {
        return result;
    }
    
    if (!rename_file(temp_path, path)) {
        remove_file(temp_path);
        return VoidResult::err("Failed to rename temp file to: " + path);
    }
    
    return VoidResult::ok();
}

/**
 * @brief Get temporary directory path
 */
inline String get_temp_directory() {
#if defined(_WIN32) || defined(_WIN64)
    char buffer[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, buffer);
    if (len > 0 && len < MAX_PATH) {
        return String(buffer);
    }
    return "C:\\Temp";
#else
    const char* tmp = std::getenv("TMPDIR");
    if (tmp && *tmp) return tmp;
    tmp = std::getenv("TMP");
    if (tmp && *tmp) return tmp;
    tmp = std::getenv("TEMP");
    if (tmp && *tmp) return tmp;
    return "/tmp";
#endif
}

/**
 * @brief Create a unique temporary file
 */
inline ResultE<String> create_temp_file(const String& prefix = "ims_", const String& suffix = "") {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999999);
    
    String temp_dir = get_temp_directory();
    
    for (int attempt = 0; attempt < 100; ++attempt) {
        String filename = prefix + std::to_string(dis(gen)) + suffix;
        String path = path_join(temp_dir, filename);
        
        if (!path_exists(path)) {
            // Create the file
            std::ofstream file(path);
            if (file) {
                return ResultE<String>::ok(path);
            }
        }
    }
    
    return ResultE<String>::err("Failed to create temporary file");
}

/**
 * @brief Create a unique temporary directory
 */
inline ResultE<String> create_temp_directory(const String& prefix = "ims_") {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999999);
    
    String temp_dir = get_temp_directory();
    
    for (int attempt = 0; attempt < 100; ++attempt) {
        String dirname = prefix + std::to_string(dis(gen));
        String path = path_join(temp_dir, dirname);
        
        if (!path_exists(path)) {
            if (create_directory(path)) {
                return ResultE<String>::ok(path);
            }
        }
    }
    
    return ResultE<String>::err("Failed to create temporary directory");
}

/**
 * @brief Get current working directory
 */
inline ResultE<String> get_current_directory() {
#if defined(_WIN32) || defined(_WIN64)
    char buffer[MAX_PATH];
    if (_getcwd(buffer, MAX_PATH) != nullptr) {
        return ResultE<String>::ok(String(buffer));
    }
#else
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX) != nullptr) {
        return ResultE<String>::ok(String(buffer));
    }
#endif
    return ResultE<String>::err("Failed to get current directory");
}

/**
 * @brief Set current working directory
 */
inline bool set_current_directory(const String& path) {
#if defined(_WIN32) || defined(_WIN64)
    return _chdir(path.c_str()) == 0;
#else
    return chdir(path.c_str()) == 0;
#endif
}

/**
 * @brief Get home directory
 */
inline String get_home_directory() {
#if defined(_WIN32) || defined(_WIN64)
    const char* home = std::getenv("USERPROFILE");
    if (home && *home) return home;
    
    const char* drive = std::getenv("HOMEDRIVE");
    const char* path = std::getenv("HOMEPATH");
    if (drive && path) {
        return String(drive) + path;
    }
    return "C:\\Users";
#else
    const char* home = std::getenv("HOME");
    if (home && *home) return home;
    return "/home";
#endif
}

/**
 * @brief List directory contents
 */
inline ResultE<std::vector<String>> list_directory(const String& path) {
    std::vector<String> entries;
    
#if defined(_WIN32) || defined(_WIN64)
    String search_path = path_join(path, "*");
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &fd);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return ResultE<std::vector<String>>::err("Failed to open directory: " + path);
    }
    
    do {
        String name = fd.cFileName;
        if (name != "." && name != "..") {
            entries.push_back(name);
        }
    } while (FindNextFileA(hFind, &fd));
    
    FindClose(hFind);
#else
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return ResultE<std::vector<String>>::err("Failed to open directory: " + path);
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        String name = entry->d_name;
        if (name != "." && name != "..") {
            entries.push_back(name);
        }
    }
    
    closedir(dir);
#endif
    
    return ResultE<std::vector<String>>::ok(std::move(entries));
}

/**
 * @brief RAII temporary file that is deleted on destruction
 */
class TempFile {
public:
    explicit TempFile(const String& prefix = "ims_", const String& suffix = "") {
        auto result = create_temp_file(prefix, suffix);
        if (result) {
            path_ = std::move(*result);
        }
    }
    
    ~TempFile() {
        if (!path_.empty()) {
            remove_file(path_);
        }
    }
    
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    
    TempFile(TempFile&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }
    
    TempFile& operator=(TempFile&& other) noexcept {
        if (this != &other) {
            if (!path_.empty()) remove_file(path_);
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }
    
    bool valid() const { return !path_.empty(); }
    const String& path() const { return path_; }
    
    void release() { path_.clear(); }

private:
    String path_;
};

/**
 * @brief RAII temporary directory that is deleted on destruction
 */
class TempDirectory {
public:
    explicit TempDirectory(const String& prefix = "ims_") {
        auto result = create_temp_directory(prefix);
        if (result) {
            path_ = std::move(*result);
        }
    }
    
    ~TempDirectory() {
        if (!path_.empty()) {
            // Note: Only removes if empty
            remove_directory(path_);
        }
    }
    
    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
    
    TempDirectory(TempDirectory&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }
    
    TempDirectory& operator=(TempDirectory&& other) noexcept {
        if (this != &other) {
            if (!path_.empty()) remove_directory(path_);
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }
    
    bool valid() const { return !path_.empty(); }
    const String& path() const { return path_; }
    
    void release() { path_.clear(); }

private:
    String path_;
};

}  // namespace ims::common::file_utils

#endif  // IMS_COMMON_FILE_UTILS_HPP
