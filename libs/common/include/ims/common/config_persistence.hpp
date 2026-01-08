/**
 * @file config_persistence.hpp
 * @brief Configuration persistence with JSON support and validation
 * @version 3.6.3
 *
 * Provides configuration file management including:
 * - JSON-based configuration storage
 * - Automatic backup and restore
 * - Schema validation
 * - Environment variable interpolation
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_CONFIG_PERSISTENCE_HPP
#define IMS_COMMON_CONFIG_PERSISTENCE_HPP

#include "types.hpp"
#include "file_utils.hpp"
#include <fstream>
#include <regex>
#include <cstdlib>

namespace ims::common {

// =============================================================================
// Configuration Value Types
// =============================================================================

/**
 * @brief Represents a configuration value that can be string, int, double, or bool
 */
class ConfigValue {
public:
    enum class Type { Null, String, Integer, Double, Boolean, Array, Object };
    
private:
    Type type_{Type::Null};
    String string_value_;
    Int64 int_value_{0};
    double double_value_{0.0};
    bool bool_value_{false};
    Vector<ConfigValue> array_value_;
    Map<String, ConfigValue> object_value_;
    
public:
    ConfigValue() : type_(Type::Null) {}
    explicit ConfigValue(const String& s) : type_(Type::String), string_value_(s) {}
    explicit ConfigValue(const char* s) : type_(Type::String), string_value_(s) {}
    explicit ConfigValue(Int64 i) : type_(Type::Integer), int_value_(i) {}
    explicit ConfigValue(int i) : type_(Type::Integer), int_value_(static_cast<Int64>(i)) {}
    explicit ConfigValue(double d) : type_(Type::Double), double_value_(d) {}
    
    // Static factory for bool to avoid constructor ambiguity with integral types
    static ConfigValue from_bool(bool b) {
        ConfigValue v;
        v.type_ = Type::Boolean;
        v.bool_value_ = b;
        return v;
    }
    
    Type type() const { return type_; }
    bool is_null() const { return type_ == Type::Null; }
    bool is_string() const { return type_ == Type::String; }
    bool is_integer() const { return type_ == Type::Integer; }
    bool is_double() const { return type_ == Type::Double; }
    bool is_boolean() const { return type_ == Type::Boolean; }
    bool is_array() const { return type_ == Type::Array; }
    bool is_object() const { return type_ == Type::Object; }
    
    String as_string(const String& default_val = "") const {
        if (type_ == Type::String) return string_value_;
        if (type_ == Type::Integer) return std::to_string(int_value_);
        if (type_ == Type::Double) return std::to_string(double_value_);
        if (type_ == Type::Boolean) return bool_value_ ? "true" : "false";
        return default_val;
    }
    
    Int64 as_integer(Int64 default_val = 0) const {
        if (type_ == Type::Integer) return int_value_;
        if (type_ == Type::Double) return static_cast<Int64>(double_value_);
        if (type_ == Type::String) {
            try { return std::stoll(string_value_); } catch (...) {}
        }
        return default_val;
    }
    
    double as_double(double default_val = 0.0) const {
        if (type_ == Type::Double) return double_value_;
        if (type_ == Type::Integer) return static_cast<double>(int_value_);
        if (type_ == Type::String) {
            try { return std::stod(string_value_); } catch (...) {}
        }
        return default_val;
    }
    
    bool as_boolean(bool default_val = false) const {
        if (type_ == Type::Boolean) return bool_value_;
        if (type_ == Type::Integer) return int_value_ != 0;
        if (type_ == Type::String) {
            return string_value_ == "true" || string_value_ == "1" || 
                   string_value_ == "yes" || string_value_ == "on";
        }
        return default_val;
    }
    
    void set_array(const Vector<ConfigValue>& arr) {
        type_ = Type::Array;
        array_value_ = arr;
    }
    
    void set_object(const Map<String, ConfigValue>& obj) {
        type_ = Type::Object;
        object_value_ = obj;
    }
    
    const Vector<ConfigValue>& as_array() const { return array_value_; }
    const Map<String, ConfigValue>& as_object() const { return object_value_; }
    
    void add_to_array(const ConfigValue& val) {
        if (type_ != Type::Array) {
            type_ = Type::Array;
            array_value_.clear();
        }
        array_value_.push_back(val);
    }
    
    void set_property(const String& key, const ConfigValue& val) {
        if (type_ != Type::Object) {
            type_ = Type::Object;
            object_value_.clear();
        }
        object_value_[key] = val;
    }
    
    ConfigValue& operator[](const String& key) {
        if (type_ != Type::Object) {
            type_ = Type::Object;
        }
        return object_value_[key];
    }
    
    const ConfigValue* get(const String& key) const {
        if (type_ != Type::Object) return nullptr;
        auto it = object_value_.find(key);
        return it != object_value_.end() ? &it->second : nullptr;
    }
};

// =============================================================================
// Simple JSON Parser (no external dependencies)
// =============================================================================

/**
 * @brief Simple JSON parser for configuration files
 */
class JsonParser {
private:
    StringView input_;
    Size pos_{0};
    
    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }
    
    char peek() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }
    
    char consume() {
        return pos_ < input_.size() ? input_[pos_++] : '\0';
    }
    
    bool match(char c) {
        skip_whitespace();
        if (peek() == c) {
            ++pos_;
            return true;
        }
        return false;
    }
    
    String parse_string() {
        String result;
        consume(); // Opening quote
        while (pos_ < input_.size()) {
            char c = consume();
            if (c == '"') break;
            if (c == '\\' && pos_ < input_.size()) {
                char next = consume();
                switch (next) {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    default: result += next; break;
                }
            } else {
                result += c;
            }
        }
        return result;
    }
    
    ConfigValue parse_number() {
        Size start = pos_;
        bool is_double = false;
        
        if (peek() == '-') ++pos_;
        while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) ++pos_;
        
        if (peek() == '.') {
            is_double = true;
            ++pos_;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) ++pos_;
        }
        
        if (peek() == 'e' || peek() == 'E') {
            is_double = true;
            ++pos_;
            if (peek() == '+' || peek() == '-') ++pos_;
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) ++pos_;
        }
        
        String num_str(input_.substr(start, pos_ - start));
        if (is_double) {
            return ConfigValue(std::stod(num_str));
        } else {
            return ConfigValue(static_cast<Int64>(std::stoll(num_str)));
        }
    }
    
    ConfigValue parse_value() {
        skip_whitespace();
        char c = peek();
        
        if (c == '"') {
            return ConfigValue(parse_string());
        } else if (c == '{') {
            return parse_object();
        } else if (c == '[') {
            return parse_array();
        } else if (c == 't') {
            pos_ += 4; // true
            return ConfigValue::from_bool(true);
        } else if (c == 'f') {
            pos_ += 5; // false
            return ConfigValue::from_bool(false);
        } else if (c == 'n') {
            pos_ += 4; // null
            return ConfigValue();
        } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return parse_number();
        }
        
        return ConfigValue();
    }
    
    ConfigValue parse_array() {
        ConfigValue result;
        result.set_array({});
        consume(); // Opening bracket
        skip_whitespace();
        
        if (peek() != ']') {
            do {
                result.add_to_array(parse_value());
                skip_whitespace();
            } while (match(','));
        }
        
        match(']');
        return result;
    }
    
    ConfigValue parse_object() {
        ConfigValue result;
        result.set_object({});
        consume(); // Opening brace
        skip_whitespace();
        
        if (peek() != '}') {
            do {
                skip_whitespace();
                String key = parse_string();
                match(':');
                result.set_property(key, parse_value());
                skip_whitespace();
            } while (match(','));
        }
        
        match('}');
        return result;
    }
    
public:
    ConfigValue parse(StringView json) {
        input_ = json;
        pos_ = 0;
        return parse_value();
    }
};

// =============================================================================
// JSON Writer
// =============================================================================

/**
 * @brief Writes configuration to JSON format
 */
class JsonWriter {
private:
    int indent_level_{0};
    int indent_size_{2};
    bool pretty_{true};
    
    String indent() const {
        if (!pretty_) return "";
        return String(indent_level_ * indent_size_, ' ');
    }
    
    String newline() const {
        return pretty_ ? "\n" : "";
    }
    
    String escape_string(const String& s) const {
        String result;
        result.reserve(s.size() + 2);
        result += '"';
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 32) {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        result += buf;
                    } else {
                        result += c;
                    }
            }
        }
        result += '"';
        return result;
    }
    
public:
    explicit JsonWriter(bool pretty = true, int indent_size = 2) 
        : indent_size_(indent_size), pretty_(pretty) {}
    
    String write(const ConfigValue& value) {
        switch (value.type()) {
            case ConfigValue::Type::Null:
                return "null";
            case ConfigValue::Type::String:
                return escape_string(value.as_string());
            case ConfigValue::Type::Integer:
                return std::to_string(value.as_integer());
            case ConfigValue::Type::Double: {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.15g", value.as_double());
                return buf;
            }
            case ConfigValue::Type::Boolean:
                return value.as_boolean() ? "true" : "false";
            case ConfigValue::Type::Array: {
                const auto& arr = value.as_array();
                if (arr.empty()) return "[]";
                
                String result = "[" + newline();
                ++indent_level_;
                for (Size i = 0; i < arr.size(); ++i) {
                    result += indent() + write(arr[i]);
                    if (i + 1 < arr.size()) result += ",";
                    result += newline();
                }
                --indent_level_;
                result += indent() + "]";
                return result;
            }
            case ConfigValue::Type::Object: {
                const auto& obj = value.as_object();
                if (obj.empty()) return "{}";
                
                String result = "{" + newline();
                ++indent_level_;
                Size count = 0;
                for (const auto& [key, val] : obj) {
                    result += indent() + escape_string(key) + ": " + write(val);
                    if (++count < obj.size()) result += ",";
                    result += newline();
                }
                --indent_level_;
                result += indent() + "}";
                return result;
            }
        }
        return "null";
    }
};

// =============================================================================
// Environment Variable Interpolation
// =============================================================================

/**
 * @brief Interpolates environment variables in configuration strings
 */
class EnvInterpolator {
public:
    /**
     * @brief Replaces ${VAR} or $VAR patterns with environment variable values
     */
    static String interpolate(const String& input) {
        static const std::regex env_pattern(R"(\$\{([^}]+)\}|\$([A-Za-z_][A-Za-z0-9_]*))");
        String result;
        std::sregex_iterator it(input.begin(), input.end(), env_pattern);
        std::sregex_iterator end;
        Size last_pos = 0;
        
        while (it != end) {
            result += input.substr(last_pos, it->position() - last_pos);
            String var_name = (*it)[1].matched ? (*it)[1].str() : (*it)[2].str();
            
            const char* env_val = std::getenv(var_name.c_str());
            result += env_val ? env_val : "";
            
            last_pos = it->position() + it->length();
            ++it;
        }
        
        result += input.substr(last_pos);
        return result;
    }
    
    /**
     * @brief Recursively interpolates environment variables in a ConfigValue
     */
    static void interpolate_config(ConfigValue& config) {
        switch (config.type()) {
            case ConfigValue::Type::String: {
                String interpolated = interpolate(config.as_string());
                config = ConfigValue(interpolated);
                break;
            }
            case ConfigValue::Type::Array: {
                // Arrays require special handling - we need to work with a copy
                Vector<ConfigValue> new_array;
                for (const auto& item : config.as_array()) {
                    ConfigValue copy = item;
                    interpolate_config(copy);
                    new_array.push_back(copy);
                }
                config.set_array(new_array);
                break;
            }
            case ConfigValue::Type::Object: {
                Map<String, ConfigValue> new_obj;
                for (const auto& [key, val] : config.as_object()) {
                    ConfigValue copy = val;
                    interpolate_config(copy);
                    new_obj[key] = copy;
                }
                config.set_object(new_obj);
                break;
            }
            default:
                break;
        }
    }
};

// =============================================================================
// Configuration Persistence Manager
// =============================================================================

/**
 * @brief Manages configuration persistence with backup and validation
 */
class ConfigPersistence {
private:
    Path config_path_;
    Path backup_path_;
    ConfigValue config_;
    bool auto_backup_{true};
    Size max_backups_{5};
    
    void rotate_backups() {
        if (!auto_backup_ || backup_path_.empty()) return;
        
        // Rotate existing backups
        for (Size i = max_backups_ - 1; i > 0; --i) {
            Path old_backup = backup_path_.string() + "." + std::to_string(i);
            Path new_backup = backup_path_.string() + "." + std::to_string(i + 1);
            if (std::filesystem::exists(old_backup)) {
                if (i + 1 >= max_backups_) {
                    std::filesystem::remove(old_backup);
                } else {
                    std::filesystem::rename(old_backup, new_backup);
                }
            }
        }
        
        // Create new backup
        if (std::filesystem::exists(config_path_)) {
            Path first_backup = backup_path_.string() + ".1";
            std::filesystem::copy_file(config_path_, first_backup, 
                std::filesystem::copy_options::overwrite_existing);
        }
    }
    
public:
    explicit ConfigPersistence(const Path& config_path) 
        : config_path_(config_path)
        , backup_path_(config_path.string() + ".backup") {}
    
    void set_backup_path(const Path& path) { backup_path_ = path; }
    void set_auto_backup(bool enabled) { auto_backup_ = enabled; }
    void set_max_backups(Size count) { max_backups_ = count; }
    
    /**
     * @brief Loads configuration from file
     */
    bool load() {
        if (!std::filesystem::exists(config_path_)) {
            return false;
        }
        
        std::ifstream file(config_path_);
        if (!file) return false;
        
        String content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
        
        JsonParser parser;
        config_ = parser.parse(content);
        
        return !config_.is_null();
    }
    
    /**
     * @brief Loads configuration with environment variable interpolation
     */
    bool load_with_env() {
        if (!load()) return false;
        EnvInterpolator::interpolate_config(config_);
        return true;
    }
    
    /**
     * @brief Saves configuration to file
     */
    bool save() {
        rotate_backups();
        
        std::ofstream file(config_path_);
        if (!file) return false;
        
        JsonWriter writer(true, 2);
        file << writer.write(config_);
        
        return file.good();
    }
    
    /**
     * @brief Restores configuration from backup
     */
    bool restore_from_backup(Size backup_number = 1) {
        Path backup_file = backup_path_.string() + "." + std::to_string(backup_number);
        if (!std::filesystem::exists(backup_file)) {
            return false;
        }
        
        std::filesystem::copy_file(backup_file, config_path_,
            std::filesystem::copy_options::overwrite_existing);
        
        return load();
    }
    
    /**
     * @brief Gets a configuration value by dot-separated path
     */
    ConfigValue get(const String& path, const ConfigValue& default_val = ConfigValue()) const {
        Vector<String> parts;
        Size start = 0;
        for (Size i = 0; i <= path.size(); ++i) {
            if (i == path.size() || path[i] == '.') {
                parts.push_back(path.substr(start, i - start));
                start = i + 1;
            }
        }
        
        const ConfigValue* current = &config_;
        for (const auto& part : parts) {
            if (!current->is_object()) return default_val;
            current = current->get(part);
            if (!current) return default_val;
        }
        
        return *current;
    }
    
    /**
     * @brief Sets a configuration value by dot-separated path
     */
    void set(const String& path, const ConfigValue& value) {
        Vector<String> parts;
        Size start = 0;
        for (Size i = 0; i <= path.size(); ++i) {
            if (i == path.size() || path[i] == '.') {
                parts.push_back(path.substr(start, i - start));
                start = i + 1;
            }
        }
        
        if (parts.empty()) return;
        
        ConfigValue* current = &config_;
        for (Size i = 0; i < parts.size() - 1; ++i) {
            if (!current->is_object()) {
                current->set_object({});
            }
            current = &(*current)[parts[i]];
        }
        
        current->set_property(parts.back(), value);
    }
    
    ConfigValue& root() { return config_; }
    const ConfigValue& root() const { return config_; }
    
    const Path& path() const { return config_path_; }
    
    /**
     * @brief Checks if configuration file exists
     */
    bool exists() const {
        return std::filesystem::exists(config_path_);
    }
    
    /**
     * @brief Gets list of available backups
     */
    Vector<Path> list_backups() const {
        Vector<Path> backups;
        for (Size i = 1; i <= max_backups_; ++i) {
            Path backup_file = backup_path_.string() + "." + std::to_string(i);
            if (std::filesystem::exists(backup_file)) {
                backups.push_back(backup_file);
            }
        }
        return backups;
    }
};

// =============================================================================
// Configuration Schema Validation
// =============================================================================

/**
 * @brief Validates configuration against a schema
 */
class ConfigValidator {
public:
    struct ValidationError {
        String path;
        String message;
    };
    
private:
    Vector<ValidationError> errors_;
    
    void add_error(const String& path, const String& message) {
        errors_.push_back({path, message});
    }
    
    void validate_value(const String& path, const ConfigValue& value, 
                        const ConfigValue& schema) {
        if (!schema.is_object()) return;
        
        const auto* type_spec = schema.get("type");
        if (type_spec && type_spec->is_string()) {
            String expected_type = type_spec->as_string();
            bool valid = false;
            
            if (expected_type == "string") valid = value.is_string();
            else if (expected_type == "integer") valid = value.is_integer();
            else if (expected_type == "number") valid = value.is_integer() || value.is_double();
            else if (expected_type == "boolean") valid = value.is_boolean();
            else if (expected_type == "array") valid = value.is_array();
            else if (expected_type == "object") valid = value.is_object();
            else if (expected_type == "null") valid = value.is_null();
            
            if (!valid) {
                add_error(path, "Expected type '" + expected_type + "'");
            }
        }
        
        // Check required properties for objects
        const auto* required = schema.get("required");
        if (required && required->is_array() && value.is_object()) {
            for (const auto& req : required->as_array()) {
                if (req.is_string()) {
                    String prop_name = req.as_string();
                    if (!value.get(prop_name)) {
                        add_error(path, "Missing required property: " + prop_name);
                    }
                }
            }
        }
        
        // Validate object properties
        const auto* properties = schema.get("properties");
        if (properties && properties->is_object() && value.is_object()) {
            for (const auto& [key, prop_schema] : properties->as_object()) {
                const auto* prop_value = value.get(key);
                if (prop_value) {
                    validate_value(path.empty() ? key : path + "." + key, 
                                  *prop_value, prop_schema);
                }
            }
        }
        
        // Validate array items
        const auto* items = schema.get("items");
        if (items && value.is_array()) {
            Size index = 0;
            for (const auto& item : value.as_array()) {
                validate_value(path + "[" + std::to_string(index) + "]", item, *items);
                ++index;
            }
        }
        
        // Validate numeric ranges
        if (value.is_integer() || value.is_double()) {
            double num_val = value.as_double();
            
            const auto* minimum = schema.get("minimum");
            if (minimum && num_val < minimum->as_double()) {
                add_error(path, "Value below minimum: " + std::to_string(minimum->as_double()));
            }
            
            const auto* maximum = schema.get("maximum");
            if (maximum && num_val > maximum->as_double()) {
                add_error(path, "Value above maximum: " + std::to_string(maximum->as_double()));
            }
        }
        
        // Validate string patterns
        if (value.is_string()) {
            const auto* min_length = schema.get("minLength");
            if (min_length && value.as_string().size() < static_cast<Size>(min_length->as_integer())) {
                add_error(path, "String too short");
            }
            
            const auto* max_length = schema.get("maxLength");
            if (max_length && value.as_string().size() > static_cast<Size>(max_length->as_integer())) {
                add_error(path, "String too long");
            }
            
            const auto* pattern = schema.get("pattern");
            if (pattern && pattern->is_string()) {
                std::regex re(pattern->as_string());
                if (!std::regex_match(value.as_string(), re)) {
                    add_error(path, "String does not match pattern");
                }
            }
        }
    }
    
public:
    /**
     * @brief Validates configuration against schema
     * @return true if valid, false otherwise
     */
    bool validate(const ConfigValue& config, const ConfigValue& schema) {
        errors_.clear();
        validate_value("", config, schema);
        return errors_.empty();
    }
    
    const Vector<ValidationError>& errors() const { return errors_; }
    
    String error_report() const {
        String report;
        for (const auto& error : errors_) {
            report += "- ";
            if (!error.path.empty()) {
                report += error.path + ": ";
            }
            report += error.message + "\n";
        }
        return report;
    }
};

} // namespace ims::common

#endif // IMS_COMMON_CONFIG_PERSISTENCE_HPP
