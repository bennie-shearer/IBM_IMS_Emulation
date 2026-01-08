#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Configuration Management
// Version: 3.6.3
// NEW in v3.6.3: Runtime configuration support
// =============================================================================

#include "types.hpp"
#include "error.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace ims {

// =============================================================================
// Configuration Value (type-safe variant)
// =============================================================================

class ConfigValue {
public:
    using ValueType = Variant<String, Int64, Float64, bool>;
    
private:
    ValueType value_;
    
public:
    ConfigValue() : value_(String{}) {}
    ConfigValue(const String& s) : value_(s) {}
    ConfigValue(const char* s) : value_(String(s)) {}
    ConfigValue(Int64 i) : value_(i) {}
    ConfigValue(int i) : value_(static_cast<Int64>(i)) {}
    ConfigValue(Float64 f) : value_(f) {}
    ConfigValue(bool b) : value_(b) {}
    
    // Type checking
    bool is_string() const { return std::holds_alternative<String>(value_); }
    bool is_int() const { return std::holds_alternative<Int64>(value_); }
    bool is_float() const { return std::holds_alternative<Float64>(value_); }
    bool is_bool() const { return std::holds_alternative<bool>(value_); }
    
    // Getters with defaults
    String as_string(const String& def = "") const {
        if (auto* v = std::get_if<String>(&value_)) return *v;
        return def;
    }
    
    Int64 as_int(Int64 def = 0) const {
        if (auto* v = std::get_if<Int64>(&value_)) return *v;
        if (auto* v = std::get_if<Float64>(&value_)) return static_cast<Int64>(*v);
        return def;
    }
    
    Float64 as_float(Float64 def = 0.0) const {
        if (auto* v = std::get_if<Float64>(&value_)) return *v;
        if (auto* v = std::get_if<Int64>(&value_)) return static_cast<Float64>(*v);
        return def;
    }
    
    bool as_bool(bool def = false) const {
        if (auto* v = std::get_if<bool>(&value_)) return *v;
        if (auto* v = std::get_if<String>(&value_)) {
            String s = *v;
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s == "true" || s == "yes" || s == "1" || s == "on";
        }
        if (auto* v = std::get_if<Int64>(&value_)) return *v != 0;
        return def;
    }
    
    // Convert to string representation
    String to_string() const {
        if (auto* v = std::get_if<String>(&value_)) return *v;
        if (auto* v = std::get_if<Int64>(&value_)) return std::to_string(*v);
        if (auto* v = std::get_if<Float64>(&value_)) return std::to_string(*v);
        if (auto* v = std::get_if<bool>(&value_)) return *v ? "true" : "false";
        return "";
    }
};

// =============================================================================
// Configuration Section
// =============================================================================

class ConfigSection {
private:
    String name_;
    HashMap<String, ConfigValue> values_;
    
public:
    explicit ConfigSection(const String& name = "") : name_(name) {}
    
    const String& name() const { return name_; }
    
    void set(const String& key, const ConfigValue& value) {
        values_[key] = value;
    }
    
    ConfigValue get(const String& key, const ConfigValue& def = ConfigValue{}) const {
        auto it = values_.find(key);
        return it != values_.end() ? it->second : def;
    }
    
    bool has(const String& key) const {
        return values_.find(key) != values_.end();
    }
    
    void remove(const String& key) {
        values_.erase(key);
    }
    
    Vector<String> keys() const {
        Vector<String> result;
        result.reserve(values_.size());
        for (const auto& [k, v] : values_) {
            result.push_back(k);
        }
        return result;
    }
    
    Size size() const { return values_.size(); }
    bool empty() const { return values_.empty(); }
    
    // Convenience accessors
    String get_string(const String& key, const String& def = "") const {
        return get(key).as_string(def);
    }
    
    Int64 get_int(const String& key, Int64 def = 0) const {
        return get(key).as_int(def);
    }
    
    Float64 get_float(const String& key, Float64 def = 0.0) const {
        return get(key).as_float(def);
    }
    
    bool get_bool(const String& key, bool def = false) const {
        return get(key).as_bool(def);
    }
};

// =============================================================================
// Configuration Manager (INI-style parser)
// =============================================================================

class ConfigManager {
private:
    HashMap<String, ConfigSection> sections_;
    String default_section_{"default"};
    Path config_path_;
    mutable SharedMutex mutex_;
    
    static String trim(const String& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == String::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
    
    static bool is_comment(const String& line) {
        String trimmed = trim(line);
        return trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';';
    }
    
    static Optional<String> parse_section(const String& line) {
        String trimmed = trim(line);
        if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
            return trim(trimmed.substr(1, trimmed.size() - 2));
        }
        return std::nullopt;
    }
    
    static Optional<std::pair<String, String>> parse_key_value(const String& line) {
        auto eq_pos = line.find('=');
        if (eq_pos == String::npos) return std::nullopt;
        
        String key = trim(line.substr(0, eq_pos));
        String value = trim(line.substr(eq_pos + 1));
        
        // Remove quotes from value
        if (value.size() >= 2) {
            if ((value.front() == '"' && value.back() == '"') ||
                (value.front() == '\'' && value.back() == '\'')) {
                value = value.substr(1, value.size() - 2);
            }
        }
        
        if (key.empty()) return std::nullopt;
        return std::make_pair(key, value);
    }
    
    static ConfigValue parse_value(const String& str) {
        // Try bool
        String lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "yes" || lower == "on") return ConfigValue(true);
        if (lower == "false" || lower == "no" || lower == "off") return ConfigValue(false);
        
        // Try integer
        try {
            size_t pos;
            Int64 i = std::stoll(str, &pos);
            if (pos == str.size()) return ConfigValue(i);
        } catch (...) {}
        
        // Try float
        try {
            size_t pos;
            Float64 f = std::stod(str, &pos);
            if (pos == str.size()) return ConfigValue(f);
        } catch (...) {}
        
        // Default to string
        return ConfigValue(str);
    }
    
public:
    ConfigManager() {
        sections_[default_section_] = ConfigSection(default_section_);
    }
    
    // Load from INI file
    ErrorResult<void> load(const Path& path) {
        std::unique_lock lock(mutex_);
        
        std::ifstream file(path);
        if (!file.is_open()) {
            return make_error<void>("Failed to open config file: " + path.string());
        }
        
        config_path_ = path;
        sections_.clear();
        sections_[default_section_] = ConfigSection(default_section_);
        
        String current_section = default_section_;
        String line;
        int line_num = 0;
        
        while (std::getline(file, line)) {
            ++line_num;
            
            if (is_comment(line)) continue;
            
            // Try section header
            if (auto section = parse_section(line)) {
                current_section = *section;
                if (sections_.find(current_section) == sections_.end()) {
                    sections_[current_section] = ConfigSection(current_section);
                }
                continue;
            }
            
            // Try key=value
            if (auto kv = parse_key_value(line)) {
                auto& [key, value] = *kv;
                sections_[current_section].set(key, parse_value(value));
            }
        }
        
        return ErrorResult<void>();
    }
    
    // Save to INI file
    ErrorResult<void> save(const Path& path) const {
        std::shared_lock lock(mutex_);
        
        std::ofstream file(path);
        if (!file.is_open()) {
            return make_error<void>("Failed to open config file for writing: " + path.string());
        }
        
        file << "# IBM IMS (Information Management System) Emulation Enterprise Configuration\n";
        file << "# Generated automatically\n\n";
        
        for (const auto& [section_name, section] : sections_) {
            if (section.empty()) continue;
            
            if (section_name != default_section_) {
                file << "[" << section_name << "]\n";
            }
            
            for (const auto& key : section.keys()) {
                file << key << " = " << section.get(key).to_string() << "\n";
            }
            file << "\n";
        }
        
        return ErrorResult<void>();
    }
    
    ErrorResult<void> save() const {
        if (config_path_.empty()) {
            return make_error<void>("No config path set");
        }
        return save(config_path_);
    }
    
    // Section access
    ConfigSection& section(const String& name) {
        std::unique_lock lock(mutex_);
        if (sections_.find(name) == sections_.end()) {
            sections_[name] = ConfigSection(name);
        }
        return sections_[name];
    }
    
    const ConfigSection* section_ptr(const String& name) const {
        std::shared_lock lock(mutex_);
        auto it = sections_.find(name);
        return it != sections_.end() ? &it->second : nullptr;
    }
    
    ConfigSection& operator[](const String& name) { return section(name); }
    
    // Direct value access (default section)
    void set(const String& key, const ConfigValue& value) {
        section(default_section_).set(key, value);
    }
    
    ConfigValue get(const String& key, const ConfigValue& def = ConfigValue{}) const {
        std::shared_lock lock(mutex_);
        auto it = sections_.find(default_section_);
        return it != sections_.end() ? it->second.get(key, def) : def;
    }
    
    // Convenience accessors
    String get_string(const String& key, const String& def = "") const {
        return get(key).as_string(def);
    }
    
    Int64 get_int(const String& key, Int64 def = 0) const {
        return get(key).as_int(def);
    }
    
    Float64 get_float(const String& key, Float64 def = 0.0) const {
        return get(key).as_float(def);
    }
    
    bool get_bool(const String& key, bool def = false) const {
        return get(key).as_bool(def);
    }
    
    // Qualified access (section.key)
    ConfigValue get_qualified(const String& qualified_key, const ConfigValue& def = ConfigValue{}) const {
        auto dot_pos = qualified_key.find('.');
        if (dot_pos == String::npos) {
            return get(qualified_key, def);
        }
        
        String section_name = qualified_key.substr(0, dot_pos);
        String key = qualified_key.substr(dot_pos + 1);
        
        if (auto* sec = section_ptr(section_name)) {
            return sec->get(key, def);
        }
        return def;
    }
    
    Vector<String> section_names() const {
        std::shared_lock lock(mutex_);
        Vector<String> result;
        result.reserve(sections_.size());
        for (const auto& [name, section] : sections_) {
            result.push_back(name);
        }
        return result;
    }
    
    void clear() {
        std::unique_lock lock(mutex_);
        sections_.clear();
        sections_[default_section_] = ConfigSection(default_section_);
    }
};

// =============================================================================
// Global Configuration Instance
// =============================================================================

inline ConfigManager& global_config() {
    static ConfigManager instance;
    return instance;
}

} // namespace ims
