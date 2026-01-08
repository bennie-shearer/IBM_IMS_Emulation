/**
 * @file argparse.hpp
 * @brief Command-line argument parser without external dependencies
 * @version 3.6.2
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_ARGPARSE_HPP
#define IMS_COMMON_ARGPARSE_HPP

#include "types.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <vector>

namespace ims::common {

/**
 * @brief Parsed argument value
 */
class ArgumentValue {
public:
    ArgumentValue() : value_(false), count_(0) {}
    
    explicit ArgumentValue(bool b) : value_(b), count_(b ? 1 : 0) {}
    explicit ArgumentValue(String s) : value_(std::move(s)), count_(1) {}
    explicit ArgumentValue(std::vector<String> v) : value_(std::move(v)), count_(v.size()) {}
    
    bool is_set() const { return count_ > 0; }
    Size count() const { return count_; }
    
    bool as_bool() const {
        if (auto* b = std::get_if<bool>(&value_)) {
            return *b;
        }
        if (auto* s = std::get_if<String>(&value_)) {
            return !s->empty() && *s != "0" && *s != "false" && *s != "no";
        }
        return count_ > 0;
    }
    
    String as_string() const {
        if (auto* s = std::get_if<String>(&value_)) {
            return *s;
        }
        if (auto* v = std::get_if<std::vector<String>>(&value_)) {
            return v->empty() ? "" : v->front();
        }
        return std::get_if<bool>(&value_) && std::get<bool>(value_) ? "true" : "";
    }
    
    int as_int(int default_val = 0) const {
        String s = as_string();
        if (s.empty()) return default_val;
        try {
            return std::stoi(s);
        } catch (...) {
            return default_val;
        }
    }
    
    double as_double(double default_val = 0.0) const {
        String s = as_string();
        if (s.empty()) return default_val;
        try {
            return std::stod(s);
        } catch (...) {
            return default_val;
        }
    }
    
    std::vector<String> as_list() const {
        if (auto* v = std::get_if<std::vector<String>>(&value_)) {
            return *v;
        }
        if (auto* s = std::get_if<String>(&value_)) {
            return s->empty() ? std::vector<String>{} : std::vector<String>{*s};
        }
        return {};
    }
    
    operator bool() const { return is_set(); }
    operator String() const { return as_string(); }
    operator int() const { return as_int(); }

private:
    std::variant<bool, String, std::vector<String>> value_;
    Size count_;
};

/**
 * @brief Argument definition builder
 */
class Argument {
public:
    Argument(String short_name, String long_name)
        : short_name_(std::move(short_name))
        , long_name_(std::move(long_name)) {}
    
    Argument& help(String text) {
        help_text_ = std::move(text);
        return *this;
    }
    
    Argument& required() {
        required_ = true;
        return *this;
    }
    
    Argument& default_value(String value) {
        default_value_ = std::move(value);
        return *this;
    }
    
    Argument& flag() {
        is_flag_ = true;
        return *this;
    }
    
    Argument& nargs(int n) {
        nargs_ = n;
        return *this;
    }
    
    Argument& choices(std::vector<String> choices) {
        choices_ = std::move(choices);
        return *this;
    }
    
    Argument& metavar(String name) {
        metavar_ = std::move(name);
        return *this;
    }
    
    const String& short_name() const { return short_name_; }
    const String& long_name() const { return long_name_; }
    const String& help_text() const { return help_text_; }
    const std::optional<String>& default_value() const { return default_value_; }
    bool is_required() const { return required_; }
    bool is_flag() const { return is_flag_; }
    int get_nargs() const { return nargs_; }
    const std::vector<String>& get_choices() const { return choices_; }
    const String& get_metavar() const { return metavar_; }

private:
    String short_name_;
    String long_name_;
    String help_text_;
    std::optional<String> default_value_;
    bool required_ = false;
    bool is_flag_ = false;
    int nargs_ = 1;
    std::vector<String> choices_;
    String metavar_;
};

/**
 * @brief Parsed arguments container
 */
class ParsedArguments {
public:
    ArgumentValue operator[](const String& name) const {
        auto it = values_.find(name);
        return it != values_.end() ? it->second : ArgumentValue();
    }
    
    bool has(const String& name) const {
        auto it = values_.find(name);
        return it != values_.end() && it->second.is_set();
    }
    
    const std::vector<String>& positional() const { return positional_; }
    
    void set(const String& name, ArgumentValue value) {
        values_[name] = std::move(value);
    }
    
    void add_positional(String value) {
        positional_.push_back(std::move(value));
    }

private:
    std::map<String, ArgumentValue> values_;
    std::vector<String> positional_;
};

/**
 * @brief Command-line argument parser
 */
class ArgumentParser {
public:
    ArgumentParser(String program_name, String description = "")
        : program_name_(std::move(program_name))
        , description_(std::move(description)) {}
    
    /**
     * @brief Add an argument definition
     */
    Argument& add_argument(const String& short_name, const String& long_name = "") {
        arguments_.emplace_back(short_name, long_name);
        return arguments_.back();
    }
    
    /**
     * @brief Add a positional argument
     */
    void add_positional(const String& name, const String& help_text = "", bool required = true) {
        positional_names_.push_back(name);
        positional_help_.push_back(help_text);
        positional_required_.push_back(required);
    }
    
    /**
     * @brief Set epilog text shown after help
     */
    void set_epilog(String epilog) {
        epilog_ = std::move(epilog);
    }
    
    /**
     * @brief Parse command line arguments
     */
    ParsedArguments parse(int argc, char* argv[]) {
        std::vector<String> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        return parse(args);
    }
    
    /**
     * @brief Parse argument vector
     */
    ParsedArguments parse(const std::vector<String>& args) {
        ParsedArguments result;
        
        // Initialize defaults
        for (const auto& arg : arguments_) {
            String key = arg.long_name().empty() ? arg.short_name() : arg.long_name();
            // Remove leading dashes for key
            while (!key.empty() && key[0] == '-') key = key.substr(1);
            
            if (arg.default_value()) {
                result.set(key, ArgumentValue(*arg.default_value()));
            } else if (arg.is_flag()) {
                result.set(key, ArgumentValue(false));
            }
        }
        
        Size positional_index = 0;
        
        for (Size i = 0; i < args.size(); ++i) {
            const String& current = args[i];
            
            // Check for help
            if (current == "-h" || current == "--help") {
                print_help();
                std::exit(0);
            }
            
            // Check if it's an option
            if (current.length() > 1 && current[0] == '-') {
                const Argument* found_arg = nullptr;
                
                for (const auto& arg : arguments_) {
                    if (current == arg.short_name() || current == arg.long_name()) {
                        found_arg = &arg;
                        break;
                    }
                }
                
                if (!found_arg) {
                    throw std::runtime_error("Unknown argument: " + current);
                }
                
                String key = found_arg->long_name().empty() ? 
                    found_arg->short_name() : found_arg->long_name();
                while (!key.empty() && key[0] == '-') key = key.substr(1);
                
                if (found_arg->is_flag()) {
                    result.set(key, ArgumentValue(true));
                } else {
                    // Get value(s)
                    int nargs = found_arg->get_nargs();
                    
                    if (nargs == 1) {
                        if (i + 1 >= args.size()) {
                            throw std::runtime_error("Missing value for: " + current);
                        }
                        String value = args[++i];
                        validate_choice(*found_arg, value);
                        result.set(key, ArgumentValue(value));
                    } else {
                        std::vector<String> values;
                        for (int n = 0; n < nargs && i + 1 < args.size(); ++n) {
                            String value = args[++i];
                            if (value[0] == '-') {
                                --i;
                                break;
                            }
                            validate_choice(*found_arg, value);
                            values.push_back(value);
                        }
                        result.set(key, ArgumentValue(values));
                    }
                }
            } else {
                // Positional argument
                result.add_positional(current);
                
                if (positional_index < positional_names_.size()) {
                    result.set(positional_names_[positional_index], ArgumentValue(current));
                }
                ++positional_index;
            }
        }
        
        // Check required arguments
        for (const auto& arg : arguments_) {
            if (arg.is_required()) {
                String key = arg.long_name().empty() ? arg.short_name() : arg.long_name();
                while (!key.empty() && key[0] == '-') key = key.substr(1);
                
                if (!result.has(key)) {
                    throw std::runtime_error("Missing required argument: " + 
                        (arg.long_name().empty() ? arg.short_name() : arg.long_name()));
                }
            }
        }
        
        // Check required positional arguments
        for (Size j = 0; j < positional_required_.size(); ++j) {
            if (positional_required_[j] && j >= positional_index) {
                throw std::runtime_error("Missing required positional argument: " + 
                    positional_names_[j]);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Print help message
     */
    void print_help(std::ostream& out = std::cout) const {
        out << "Usage: " << program_name_;
        
        for (const auto& arg : arguments_) {
            if (arg.is_required()) {
                out << " " << arg.short_name();
                if (!arg.is_flag()) {
                    out << " " << (arg.get_metavar().empty() ? "VALUE" : arg.get_metavar());
                }
            }
        }
        
        for (Size i = 0; i < positional_names_.size(); ++i) {
            if (positional_required_[i]) {
                out << " " << positional_names_[i];
            } else {
                out << " [" << positional_names_[i] << "]";
            }
        }
        
        out << " [options]\n\n";
        
        if (!description_.empty()) {
            out << description_ << "\n\n";
        }
        
        if (!positional_names_.empty()) {
            out << "Positional arguments:\n";
            for (Size i = 0; i < positional_names_.size(); ++i) {
                out << "  " << std::left << std::setw(20) << positional_names_[i];
                if (i < positional_help_.size() && !positional_help_[i].empty()) {
                    out << positional_help_[i];
                }
                out << "\n";
            }
            out << "\n";
        }
        
        out << "Options:\n";
        out << "  " << std::left << std::setw(20) << "-h, --help" << "Show this help message\n";
        
        for (const auto& arg : arguments_) {
            std::ostringstream opt;
            opt << arg.short_name();
            if (!arg.long_name().empty()) {
                opt << ", " << arg.long_name();
            }
            if (!arg.is_flag()) {
                opt << " " << (arg.get_metavar().empty() ? "VALUE" : arg.get_metavar());
            }
            
            out << "  " << std::left << std::setw(20) << opt.str();
            out << arg.help_text();
            
            if (arg.default_value()) {
                out << " (default: " << *arg.default_value() << ")";
            }
            if (arg.is_required()) {
                out << " [required]";
            }
            if (!arg.get_choices().empty()) {
                out << " {";
                bool first = true;
                for (const auto& c : arg.get_choices()) {
                    if (!first) out << ", ";
                    out << c;
                    first = false;
                }
                out << "}";
            }
            out << "\n";
        }
        
        if (!epilog_.empty()) {
            out << "\n" << epilog_ << "\n";
        }
    }

private:
    String program_name_;
    String description_;
    String epilog_;
    std::vector<Argument> arguments_;
    std::vector<String> positional_names_;
    std::vector<String> positional_help_;
    std::vector<bool> positional_required_;
    
    void validate_choice(const Argument& arg, const String& value) const {
        const auto& choices = arg.get_choices();
        if (!choices.empty()) {
            if (std::find(choices.begin(), choices.end(), value) == choices.end()) {
                std::ostringstream oss;
                oss << "Invalid choice '" << value << "' for ";
                oss << (arg.long_name().empty() ? arg.short_name() : arg.long_name());
                oss << ". Valid choices: ";
                bool first = true;
                for (const auto& c : choices) {
                    if (!first) oss << ", ";
                    oss << c;
                    first = false;
                }
                throw std::runtime_error(oss.str());
            }
        }
    }
};

}  // namespace ims::common

#endif  // IMS_COMMON_ARGPARSE_HPP
