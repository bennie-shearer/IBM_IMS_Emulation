/**
 * @file command_processor.hpp
 * @brief Interactive command-line interface framework
 * @version 3.6.3
 *
 * Provides command processing including:
 * - Interactive command-line interface
 * - Command history and auto-completion
 * - Built-in help system
 * - Extensible command registration
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_COMMAND_PROCESSOR_HPP
#define IMS_COMMON_COMMAND_PROCESSOR_HPP

#include "types.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <deque>

namespace ims::common {

// =============================================================================
// Command Result
// =============================================================================

/**
 * @brief Result of command execution
 */
struct CommandResult {
    bool success{true};
    String output;
    String error;
    int exit_code{0};
    
    static CommandResult ok(const String& output = "") {
        return {true, output, "", 0};
    }
    
    static CommandResult fail(const String& error, int code = 1) {
        return {false, "", error, code};
    }
};

// =============================================================================
// Command Context
// =============================================================================

/**
 * @brief Context passed to command handlers
 */
struct CommandContext {
    Vector<String> args;           ///< Command arguments (excluding command name)
    Map<String, String> options;   ///< Parsed options (--key=value)
    Set<String> flags;             ///< Parsed flags (--flag)
    std::ostream& out;             ///< Output stream
    std::ostream& err;             ///< Error stream
    void* user_data{nullptr};      ///< Custom user data
    
    CommandContext(std::ostream& out_stream = std::cout, 
                   std::ostream& err_stream = std::cerr)
        : out(out_stream), err(err_stream) {}
    
    bool has_flag(const String& name) const {
        return flags.find(name) != flags.end();
    }
    
    String option(const String& name, const String& default_val = "") const {
        auto it = options.find(name);
        return it != options.end() ? it->second : default_val;
    }
    
    Optional<String> arg(Size index) const {
        return index < args.size() ? Optional<String>(args[index]) : std::nullopt;
    }
};

// =============================================================================
// Command Definition
// =============================================================================

/**
 * @brief Definition of a command
 */
struct CommandDefinition {
    using Handler = Function<CommandResult(CommandContext&)>;
    using Completer = Function<Vector<String>(const String&)>;
    
    String name;
    String description;
    String usage;
    String category;
    Handler handler;
    Completer completer;
    Vector<String> aliases;
    bool hidden{false};
    
    CommandDefinition() = default;
    
    CommandDefinition(String cmd_name, String cmd_desc, Handler cmd_handler)
        : name(std::move(cmd_name))
        , description(std::move(cmd_desc))
        , handler(std::move(cmd_handler)) {}
    
    // Full constructor for all common fields
    CommandDefinition(String cmd_name, String cmd_desc, String cmd_usage,
                     String cmd_category, Handler cmd_handler)
        : name(std::move(cmd_name))
        , description(std::move(cmd_desc))
        , usage(std::move(cmd_usage))
        , category(std::move(cmd_category))
        , handler(std::move(cmd_handler)) {}
};

// =============================================================================
// Command History
// =============================================================================

/**
 * @brief Manages command history
 */
class CommandHistory {
private:
    std::deque<String> history_;
    Size max_size_{100};
    Size current_index_{0};
    
public:
    explicit CommandHistory(Size max_size = 100) : max_size_(max_size) {}
    
    void add(const String& command) {
        // Don't add empty or duplicate consecutive commands
        if (command.empty()) return;
        if (!history_.empty() && history_.back() == command) return;
        
        history_.push_back(command);
        if (history_.size() > max_size_) {
            history_.pop_front();
        }
        current_index_ = history_.size();
    }
    
    Optional<String> previous() {
        if (history_.empty() || current_index_ == 0) return std::nullopt;
        --current_index_;
        return history_[current_index_];
    }
    
    Optional<String> next() {
        if (current_index_ >= history_.size()) return std::nullopt;
        ++current_index_;
        if (current_index_ >= history_.size()) return std::nullopt;
        return history_[current_index_];
    }
    
    void reset_navigation() {
        current_index_ = history_.size();
    }
    
    Vector<String> get_all() const {
        return Vector<String>(history_.begin(), history_.end());
    }
    
    Vector<String> search(const String& prefix) const {
        Vector<String> matches;
        for (const auto& cmd : history_) {
            if (cmd.size() >= prefix.size() && 
                cmd.substr(0, prefix.size()) == prefix) {
                matches.push_back(cmd);
            }
        }
        return matches;
    }
    
    Size size() const { return history_.size(); }
    void clear() { history_.clear(); current_index_ = 0; }
};

// =============================================================================
// Command Parser
// =============================================================================

/**
 * @brief Parses command line into tokens
 */
class CommandParser {
public:
    struct ParsedCommand {
        String name;
        Vector<String> args;
        Map<String, String> options;
        Set<String> flags;
    };
    
    static Vector<String> tokenize(const String& input) {
        Vector<String> tokens;
        String current;
        bool in_quotes = false;
        bool in_single_quotes = false;
        bool escaped = false;
        
        for (char c : input) {
            if (escaped) {
                current += c;
                escaped = false;
                continue;
            }
            
            if (c == '\\') {
                escaped = true;
                continue;
            }
            
            if (c == '"' && !in_single_quotes) {
                in_quotes = !in_quotes;
                continue;
            }
            
            if (c == '\'' && !in_quotes) {
                in_single_quotes = !in_single_quotes;
                continue;
            }
            
            if (std::isspace(static_cast<unsigned char>(c)) && 
                !in_quotes && !in_single_quotes) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                continue;
            }
            
            current += c;
        }
        
        if (!current.empty()) {
            tokens.push_back(current);
        }
        
        return tokens;
    }
    
    static ParsedCommand parse(const String& input) {
        ParsedCommand result;
        auto tokens = tokenize(input);
        
        if (tokens.empty()) return result;
        
        result.name = tokens[0];
        
        for (Size i = 1; i < tokens.size(); ++i) {
            const String& token = tokens[i];
            
            if (token.size() >= 2 && token[0] == '-' && token[1] == '-') {
                // Long option
                String opt = token.substr(2);
                auto eq_pos = opt.find('=');
                
                if (eq_pos != String::npos) {
                    String key = opt.substr(0, eq_pos);
                    String value = opt.substr(eq_pos + 1);
                    result.options[key] = value;
                } else {
                    result.flags.insert(opt);
                }
            } else if (token.size() >= 1 && token[0] == '-') {
                // Short option(s)
                for (Size j = 1; j < token.size(); ++j) {
                    result.flags.insert(String(1, token[j]));
                }
            } else {
                // Regular argument
                result.args.push_back(token);
            }
        }
        
        return result;
    }
};

// =============================================================================
// Command Processor
// =============================================================================

/**
 * @brief Main command processor with registration and execution
 */
class CommandProcessor {
private:
    Map<String, CommandDefinition> commands_;
    Map<String, String> aliases_;  // alias -> command name
    CommandHistory history_;
    String prompt_{"$ "};
    void* user_data_{nullptr};
    bool running_{false};
    std::ostream& out_;
    std::ostream& err_;
    
    // Built-in commands
    void register_builtin_commands() {
        // Help command
        register_command(CommandDefinition(
            "help",
            "Display help information",
            "help [command]",
            "System",
            [this](CommandContext& ctx) -> CommandResult {
                if (ctx.args.empty()) {
                    return show_all_help(ctx);
                } else {
                    return show_command_help(ctx.args[0], ctx);
                }
            }
        ));
        
        // History command
        register_command(CommandDefinition(
            "history",
            "Display command history",
            "history [count]",
            "System",
            [this](CommandContext& ctx) -> CommandResult {
                Size count = 10;
                if (!ctx.args.empty()) {
                    try {
                        count = static_cast<Size>(std::stoul(ctx.args[0]));
                    } catch (...) {}
                }
                
                auto all = history_.get_all();
                Size start = all.size() > count ? all.size() - count : 0;
                
                for (Size i = start; i < all.size(); ++i) {
                    ctx.out << std::setw(4) << (i + 1) << "  " << all[i] << "\n";
                }
                
                return CommandResult::ok();
            }
        ));
        
        // Clear command
        register_command(CommandDefinition(
            "clear",
            "Clear the screen",
            "clear",
            "System",
            [](CommandContext& ctx) -> CommandResult {
                // ANSI escape sequence to clear screen
                ctx.out << "\033[2J\033[H";
                return CommandResult::ok();
            }
        ));
        
        // Exit command
        register_command(CommandDefinition(
            "exit",
            "Exit the command processor",
            "exit [code]",
            "System",
            [this](CommandContext& ctx) -> CommandResult {
                running_ = false;
                int code = 0;
                if (!ctx.args.empty()) {
                    try {
                        code = std::stoi(ctx.args[0]);
                    } catch (...) {}
                }
                return CommandResult{true, "", "", code};
            }
        ));
        add_alias("quit", "exit");
        
        // Echo command
        register_command(CommandDefinition(
            "echo",
            "Echo arguments to output",
            "echo [text...]",
            "System",
            [](CommandContext& ctx) -> CommandResult {
                for (Size i = 0; i < ctx.args.size(); ++i) {
                    if (i > 0) ctx.out << " ";
                    ctx.out << ctx.args[i];
                }
                ctx.out << "\n";
                return CommandResult::ok();
            }
        ));
    }
    
    CommandResult show_all_help(CommandContext& ctx) {
        // Group commands by category
        Map<String, Vector<const CommandDefinition*>> categories;
        
        for (const auto& [name, cmd] : commands_) {
            if (cmd.hidden) continue;
            String cat = cmd.category.empty() ? "General" : cmd.category;
            categories[cat].push_back(&cmd);
        }
        
        ctx.out << "Available commands:\n\n";
        
        for (const auto& [category, cmds] : categories) {
            ctx.out << category << ":\n";
            
            for (const auto* cmd : cmds) {
                ctx.out << "  " << std::left << std::setw(15) << cmd->name
                        << " " << cmd->description << "\n";
            }
            ctx.out << "\n";
        }
        
        ctx.out << "Type 'help <command>' for more information.\n";
        
        return CommandResult::ok();
    }
    
    CommandResult show_command_help(const String& name, CommandContext& ctx) {
        auto it = commands_.find(resolve_alias(name));
        if (it == commands_.end()) {
            return CommandResult::fail("Unknown command: " + name);
        }
        
        const auto& cmd = it->second;
        
        ctx.out << cmd.name << " - " << cmd.description << "\n\n";
        
        if (!cmd.usage.empty()) {
            ctx.out << "Usage: " << cmd.usage << "\n\n";
        }
        
        if (!cmd.aliases.empty()) {
            ctx.out << "Aliases: ";
            for (Size i = 0; i < cmd.aliases.size(); ++i) {
                if (i > 0) ctx.out << ", ";
                ctx.out << cmd.aliases[i];
            }
            ctx.out << "\n";
        }
        
        return CommandResult::ok();
    }
    
    String resolve_alias(const String& name) const {
        auto it = aliases_.find(name);
        return it != aliases_.end() ? it->second : name;
    }
    
public:
    CommandProcessor(std::ostream& out = std::cout, std::ostream& err = std::cerr)
        : out_(out), err_(err) {
        register_builtin_commands();
    }
    
    /**
     * @brief Registers a command
     */
    void register_command(CommandDefinition cmd) {
        String name = cmd.name;
        
        // Register aliases
        for (const auto& alias : cmd.aliases) {
            aliases_[alias] = name;
        }
        
        commands_[name] = std::move(cmd);
    }
    
    /**
     * @brief Adds an alias for an existing command
     */
    void add_alias(const String& alias, const String& command) {
        aliases_[alias] = command;
        
        auto it = commands_.find(command);
        if (it != commands_.end()) {
            it->second.aliases.push_back(alias);
        }
    }
    
    /**
     * @brief Unregisters a command
     */
    bool unregister_command(const String& name) {
        auto it = commands_.find(name);
        if (it == commands_.end()) return false;
        
        // Remove aliases
        for (const auto& alias : it->second.aliases) {
            aliases_.erase(alias);
        }
        
        commands_.erase(it);
        return true;
    }
    
    /**
     * @brief Executes a command string
     */
    CommandResult execute(const String& input) {
        auto parsed = CommandParser::parse(input);
        
        if (parsed.name.empty()) {
            return CommandResult::ok();
        }
        
        // Add to history
        history_.add(input);
        
        // Resolve alias
        String cmd_name = resolve_alias(parsed.name);
        
        // Find command
        auto it = commands_.find(cmd_name);
        if (it == commands_.end()) {
            return CommandResult::fail("Unknown command: " + parsed.name);
        }
        
        // Build context
        CommandContext ctx(out_, err_);
        ctx.args = std::move(parsed.args);
        ctx.options = std::move(parsed.options);
        ctx.flags = std::move(parsed.flags);
        ctx.user_data = user_data_;
        
        // Execute
        try {
            return it->second.handler(ctx);
        } catch (const std::exception& e) {
            return CommandResult::fail(String("Exception: ") + e.what());
        } catch (...) {
            return CommandResult::fail("Unknown exception");
        }
    }
    
    /**
     * @brief Gets completions for partial input
     */
    Vector<String> complete(const String& partial) {
        Vector<String> completions;
        
        auto parsed = CommandParser::parse(partial);
        
        if (parsed.args.empty() && partial.find(' ') == String::npos) {
            // Complete command name
            for (const auto& [name, cmd] : commands_) {
                if (cmd.hidden) continue;
                if (name.size() >= partial.size() && 
                    name.substr(0, partial.size()) == partial) {
                    completions.push_back(name);
                }
            }
            
            for (const auto& [alias, _] : aliases_) {
                if (alias.size() >= partial.size() && 
                    alias.substr(0, partial.size()) == partial) {
                    completions.push_back(alias);
                }
            }
        } else {
            // Complete arguments using command's completer
            String cmd_name = resolve_alias(parsed.name);
            auto it = commands_.find(cmd_name);
            
            if (it != commands_.end() && it->second.completer) {
                String current_arg = parsed.args.empty() ? "" : parsed.args.back();
                completions = it->second.completer(current_arg);
            }
        }
        
        std::sort(completions.begin(), completions.end());
        completions.erase(std::unique(completions.begin(), completions.end()), 
                         completions.end());
        
        return completions;
    }
    
    /**
     * @brief Runs interactive command loop
     */
    int run() {
        running_ = true;
        int last_exit_code = 0;
        
        while (running_) {
            out_ << prompt_ << std::flush;
            
            String line;
            if (!std::getline(std::cin, line)) {
                break;
            }
            
            // Trim whitespace
            Size start = line.find_first_not_of(" \t\r\n");
            Size end = line.find_last_not_of(" \t\r\n");
            if (start == String::npos) continue;
            line = line.substr(start, end - start + 1);
            
            if (line.empty()) continue;
            
            auto result = execute(line);
            last_exit_code = result.exit_code;
            
            if (!result.success && !result.error.empty()) {
                err_ << "Error: " << result.error << "\n";
            }
            
            if (!result.output.empty()) {
                out_ << result.output;
                if (result.output.back() != '\n') {
                    out_ << "\n";
                }
            }
        }
        
        return last_exit_code;
    }
    
    /**
     * @brief Stops the command loop
     */
    void stop() {
        running_ = false;
    }
    
    /**
     * @brief Sets the prompt string
     */
    void set_prompt(const String& prompt) {
        prompt_ = prompt;
    }
    
    /**
     * @brief Sets user data available to all commands
     */
    void set_user_data(void* data) {
        user_data_ = data;
    }
    
    /**
     * @brief Gets command history
     */
    CommandHistory& history() { return history_; }
    const CommandHistory& history() const { return history_; }
    
    /**
     * @brief Gets list of registered commands
     */
    Vector<String> command_names() const {
        Vector<String> names;
        for (const auto& [name, cmd] : commands_) {
            if (!cmd.hidden) {
                names.push_back(name);
            }
        }
        std::sort(names.begin(), names.end());
        return names;
    }
    
    /**
     * @brief Checks if a command exists
     */
    bool has_command(const String& name) const {
        return commands_.find(resolve_alias(name)) != commands_.end();
    }
};

// =============================================================================
// Command Builder
// =============================================================================

/**
 * @brief Builder pattern for command definition
 */
class CommandBuilder {
private:
    CommandDefinition cmd_;
    
public:
    explicit CommandBuilder(const String& name) {
        cmd_.name = name;
    }
    
    CommandBuilder& description(const String& desc) {
        cmd_.description = desc;
        return *this;
    }
    
    CommandBuilder& usage(const String& usage_str) {
        cmd_.usage = usage_str;
        return *this;
    }
    
    CommandBuilder& category(const String& cat) {
        cmd_.category = cat;
        return *this;
    }
    
    CommandBuilder& alias(const String& alias_name) {
        cmd_.aliases.push_back(alias_name);
        return *this;
    }
    
    CommandBuilder& hidden(bool is_hidden = true) {
        cmd_.hidden = is_hidden;
        return *this;
    }
    
    CommandBuilder& handler(CommandDefinition::Handler h) {
        cmd_.handler = std::move(h);
        return *this;
    }
    
    CommandBuilder& completer(CommandDefinition::Completer c) {
        cmd_.completer = std::move(c);
        return *this;
    }
    
    CommandDefinition build() {
        return std::move(cmd_);
    }
    
    void register_to(CommandProcessor& processor) {
        processor.register_command(std::move(cmd_));
    }
};

} // namespace ims::common

#endif // IMS_COMMON_COMMAND_PROCESSOR_HPP
