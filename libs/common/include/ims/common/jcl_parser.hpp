#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - JCL Parser Foundation
// Version: 3.6.2
// =============================================================================
//
// Basic JCL (Job Control Language) parsing for DD statement extraction and
// dataset allocation parameters. Supports common JCL constructs.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include "error.hpp"
#include "validation.hpp"

namespace ims::jcl {

// =============================================================================
// JCL Statement Types
// =============================================================================

enum class StatementType : UInt8 {
    UNKNOWN = 0,
    JOB = 1,
    EXEC = 2,
    DD = 3,
    PROC = 4,
    PEND = 5,
    SET = 6,
    IF = 7,
    ELSE = 8,
    ENDIF = 9,
    JCLLIB = 10,
    INCLUDE = 11,
    COMMENT = 12,
    NULL_STATEMENT = 13
};

inline String statement_type_to_string(StatementType type) {
    switch (type) {
        case StatementType::JOB:     return "JOB";
        case StatementType::EXEC:    return "EXEC";
        case StatementType::DD:      return "DD";
        case StatementType::PROC:    return "PROC";
        case StatementType::PEND:    return "PEND";
        case StatementType::SET:     return "SET";
        case StatementType::IF:      return "IF";
        case StatementType::ELSE:    return "ELSE";
        case StatementType::ENDIF:   return "ENDIF";
        case StatementType::JCLLIB:  return "JCLLIB";
        case StatementType::INCLUDE: return "INCLUDE";
        case StatementType::COMMENT: return "COMMENT";
        case StatementType::NULL_STATEMENT: return "NULL";
        default: return "UNKNOWN";
    }
}

// =============================================================================
// DD Statement Parameters
// =============================================================================

enum class Disposition : UInt8 {
    NEW = 1,
    OLD = 2,
    SHR = 3,
    MOD = 4
};

enum class NormalDisposition : UInt8 {
    DELETE = 1,
    KEEP = 2,
    PASS = 3,
    CATLG = 4,
    UNCATLG = 5
};

enum class AbnormalDisposition : UInt8 {
    DELETE = 1,
    KEEP = 2,
    CATLG = 3,
    UNCATLG = 4
};

struct DDParameters {
    String ddname;
    String dsname;
    Disposition disp{Disposition::NEW};
    NormalDisposition normal_disp{NormalDisposition::DELETE};
    AbnormalDisposition abnormal_disp{AbnormalDisposition::DELETE};
    String recfm;
    UInt32 lrecl{0};
    UInt32 blksize{0};
    String dsorg;
    String space_type;  // TRK, CYL, etc.
    UInt32 primary{0};
    UInt32 secondary{0};
    UInt32 directory{0};
    String unit;
    String volume;
    String dataclas;
    String storclas;
    String mgmtclas;
    bool is_sysout{false};
    String sysout_class;
    bool is_dummy{false};
    bool is_instream{false};
    String member;
    Vector<String> concatenated_datasets;
    
    DDParameters() = default;
};

// =============================================================================
// JOB Statement Parameters
// =============================================================================

struct JobParameters {
    String jobname;
    String account;
    String programmer;
    String class_param;
    String msgclass;
    String msglevel;
    String notify;
    String region;
    String time;
    String typrun;
    UInt32 priority{0};
    bool restart{false};
    String restart_step;
    
    JobParameters() = default;
};

// =============================================================================
// EXEC Statement Parameters
// =============================================================================

struct ExecParameters {
    String stepname;
    String program;       // PGM=
    String procedure;     // PROC=
    String region;
    String time;
    String cond;
    String parm;
    HashMap<String, String> overrides;
    
    ExecParameters() = default;
    
    bool is_program() const { return !program.empty(); }
    bool is_procedure() const { return !procedure.empty(); }
};

// =============================================================================
// Parsed JCL Statement
// =============================================================================

struct JclStatement {
    StatementType type{StatementType::UNKNOWN};
    String name;
    String operation;
    String operands;
    UInt32 line_number{0};
    bool is_continuation{false};
    
    // Type-specific parameters
    Optional<JobParameters> job_params;
    Optional<ExecParameters> exec_params;
    Optional<DDParameters> dd_params;
    
    JclStatement() = default;
};

// =============================================================================
// JCL Parser
// =============================================================================

class JclParser {
private:
    Vector<String> lines_;
    Size current_line_{0};
    Vector<JclStatement> statements_;
    Vector<String> errors_;
    
    static bool is_continuation_line(StringView line) {
        // Continuation line starts with "// " in columns 1-3
        return line.length() >= 3 && 
               line[0] == '/' && line[1] == '/' && line[2] == ' ';
    }
    
    static bool is_comment_line(StringView line) {
        // Comment: //*
        return line.length() >= 3 && 
               line[0] == '/' && line[1] == '/' && line[2] == '*';
    }
    
    static bool is_null_statement(StringView line) {
        // Null statement: // with nothing following
        return line == "//" || (line.length() == 2 && line[0] == '/' && line[1] == '/');
    }
    
    StatementType parse_statement_type(StringView operation) {
        if (operation == "JOB") return StatementType::JOB;
        if (operation == "EXEC") return StatementType::EXEC;
        if (operation == "DD") return StatementType::DD;
        if (operation == "PROC") return StatementType::PROC;
        if (operation == "PEND") return StatementType::PEND;
        if (operation == "SET") return StatementType::SET;
        if (operation == "IF") return StatementType::IF;
        if (operation == "ELSE") return StatementType::ELSE;
        if (operation == "ENDIF") return StatementType::ENDIF;
        if (operation == "JCLLIB") return StatementType::JCLLIB;
        if (operation == "INCLUDE") return StatementType::INCLUDE;
        return StatementType::UNKNOWN;
    }
    
    String extract_value(StringView operands, StringView key) {
        String search_key = String(key) + "=";
        auto pos = operands.find(search_key);
        if (pos == StringView::npos) return "";
        
        pos += search_key.length();
        Size end = pos;
        
        // Handle quoted values
        if (pos < operands.length() && operands[pos] == '\'') {
            ++pos;
            end = operands.find('\'', pos);
            if (end == StringView::npos) end = operands.length();
            return String(operands.substr(pos, end - pos));
        }
        
        // Handle parenthesized values
        if (pos < operands.length() && operands[pos] == '(') {
            int depth = 1;
            ++pos;
            end = pos;
            while (end < operands.length() && depth > 0) {
                if (operands[end] == '(') ++depth;
                else if (operands[end] == ')') --depth;
                ++end;
            }
            return String(operands.substr(pos, end - pos - 1));
        }
        
        // Find end of unquoted value
        while (end < operands.length() && 
               operands[end] != ',' && operands[end] != ' ') {
            ++end;
        }
        
        return String(operands.substr(pos, end - pos));
    }
    
    DDParameters parse_dd_operands(StringView operands) {
        DDParameters params;
        
        // Check for special DD types
        if (operands.find("DUMMY") != StringView::npos) {
            params.is_dummy = true;
            return params;
        }
        
        if (operands.find("SYSOUT=") != StringView::npos) {
            params.is_sysout = true;
            params.sysout_class = extract_value(operands, "SYSOUT");
            return params;
        }
        
        if (operands == "*" || operands.find("*") == 0) {
            params.is_instream = true;
            return params;
        }
        
        // Parse DSN
        String dsn = extract_value(operands, "DSN");
        if (dsn.empty()) dsn = extract_value(operands, "DSNAME");
        
        // Check for member name
        auto paren_pos = dsn.find('(');
        if (paren_pos != String::npos) {
            auto end_paren = dsn.find(')', paren_pos);
            if (end_paren != String::npos) {
                params.member = dsn.substr(paren_pos + 1, end_paren - paren_pos - 1);
                params.dsname = dsn.substr(0, paren_pos);
            }
        } else {
            params.dsname = dsn;
        }
        
        // Parse DISP
        String disp = extract_value(operands, "DISP");
        if (!disp.empty()) {
            // Parse (status,normal,abnormal)
            Vector<String> disp_parts;
            Size start = 0;
            for (Size i = 0; i <= disp.length(); ++i) {
                if (i == disp.length() || disp[i] == ',') {
                    disp_parts.push_back(disp.substr(start, i - start));
                    start = i + 1;
                }
            }
            
            if (!disp_parts.empty()) {
                if (disp_parts[0] == "NEW") params.disp = Disposition::NEW;
                else if (disp_parts[0] == "OLD") params.disp = Disposition::OLD;
                else if (disp_parts[0] == "SHR") params.disp = Disposition::SHR;
                else if (disp_parts[0] == "MOD") params.disp = Disposition::MOD;
            }
            
            if (disp_parts.size() > 1) {
                if (disp_parts[1] == "DELETE") params.normal_disp = NormalDisposition::DELETE;
                else if (disp_parts[1] == "KEEP") params.normal_disp = NormalDisposition::KEEP;
                else if (disp_parts[1] == "PASS") params.normal_disp = NormalDisposition::PASS;
                else if (disp_parts[1] == "CATLG") params.normal_disp = NormalDisposition::CATLG;
            }
            
            if (disp_parts.size() > 2) {
                if (disp_parts[2] == "DELETE") params.abnormal_disp = AbnormalDisposition::DELETE;
                else if (disp_parts[2] == "KEEP") params.abnormal_disp = AbnormalDisposition::KEEP;
                else if (disp_parts[2] == "CATLG") params.abnormal_disp = AbnormalDisposition::CATLG;
            }
        }
        
        // Parse DCB parameters
        params.recfm = extract_value(operands, "RECFM");
        String lrecl = extract_value(operands, "LRECL");
        if (!lrecl.empty()) params.lrecl = static_cast<UInt32>(std::stoul(lrecl));
        
        String blksize = extract_value(operands, "BLKSIZE");
        if (!blksize.empty()) params.blksize = static_cast<UInt32>(std::stoul(blksize));
        
        params.dsorg = extract_value(operands, "DSORG");
        
        // Parse SPACE
        String space = extract_value(operands, "SPACE");
        if (!space.empty()) {
            // Parse (type,(primary,secondary,directory))
            auto comma = space.find(',');
            if (comma != String::npos) {
                params.space_type = space.substr(0, comma);
                // Remove parentheses from space type if present
                if (!params.space_type.empty() && params.space_type[0] == '(') {
                    params.space_type = params.space_type.substr(1);
                }
            }
        }
        
        // Parse unit and volume
        params.unit = extract_value(operands, "UNIT");
        params.volume = extract_value(operands, "VOL");
        if (params.volume.empty()) {
            params.volume = extract_value(operands, "VOLUME");
        }
        
        // Parse SMS classes
        params.dataclas = extract_value(operands, "DATACLAS");
        params.storclas = extract_value(operands, "STORCLAS");
        params.mgmtclas = extract_value(operands, "MGMTCLAS");
        
        return params;
    }
    
public:
    JclParser() = default;
    
    /// Parse JCL from a string
    ErrorResult<void> parse(StringView jcl_text) {
        lines_.clear();
        statements_.clear();
        errors_.clear();
        current_line_ = 0;
        
        // Split into lines
        String current_line;
        for (char c : jcl_text) {
            if (c == '\n') {
                lines_.push_back(current_line);
                current_line.clear();
            } else if (c != '\r') {
                current_line += c;
            }
        }
        if (!current_line.empty()) {
            lines_.push_back(current_line);
        }
        
        // Parse each line
        while (current_line_ < lines_.size()) {
            const auto& line = lines_[current_line_];
            
            // Skip empty lines
            if (line.empty() || line.find_first_not_of(' ') == String::npos) {
                ++current_line_;
                continue;
            }
            
            // Check for comment
            if (is_comment_line(line)) {
                JclStatement stmt;
                stmt.type = StatementType::COMMENT;
                stmt.line_number = static_cast<UInt32>(current_line_ + 1);
                statements_.push_back(stmt);
                ++current_line_;
                continue;
            }
            
            // Check for null statement
            if (is_null_statement(line)) {
                JclStatement stmt;
                stmt.type = StatementType::NULL_STATEMENT;
                stmt.line_number = static_cast<UInt32>(current_line_ + 1);
                statements_.push_back(stmt);
                ++current_line_;
                continue;
            }
            
            // Parse JCL statement
            if (line.length() >= 2 && line[0] == '/' && line[1] == '/') {
                JclStatement stmt;
                stmt.line_number = static_cast<UInt32>(current_line_ + 1);
                
                // Extract name (columns 3-10)
                Size pos = 2;
                while (pos < line.length() && line[pos] == ' ') ++pos;
                
                Size name_start = pos;
                while (pos < line.length() && line[pos] != ' ') ++pos;
                stmt.name = line.substr(name_start, pos - name_start);
                
                // Extract operation
                while (pos < line.length() && line[pos] == ' ') ++pos;
                Size op_start = pos;
                while (pos < line.length() && line[pos] != ' ') ++pos;
                stmt.operation = line.substr(op_start, pos - op_start);
                
                // Extract operands
                while (pos < line.length() && line[pos] == ' ') ++pos;
                stmt.operands = line.substr(pos);
                
                // Handle continuation
                while (stmt.operands.length() > 0 && 
                       stmt.operands.back() == ',' &&
                       current_line_ + 1 < lines_.size()) {
                    ++current_line_;
                    const auto& cont_line = lines_[current_line_];
                    if (is_continuation_line(cont_line)) {
                        Size cont_start = 3;
                        while (cont_start < cont_line.length() && cont_line[cont_start] == ' ') {
                            ++cont_start;
                        }
                        stmt.operands += cont_line.substr(cont_start);
                    } else {
                        --current_line_;
                        break;
                    }
                }
                
                // Determine statement type
                stmt.type = parse_statement_type(stmt.operation);
                
                // Parse type-specific parameters
                if (stmt.type == StatementType::DD) {
                    stmt.dd_params = parse_dd_operands(stmt.operands);
                    stmt.dd_params->ddname = stmt.name;
                }
                
                statements_.push_back(stmt);
            }
            
            ++current_line_;
        }
        
        return make_success();
    }
    
    /// Get all parsed statements
    const Vector<JclStatement>& statements() const { return statements_; }
    
    /// Get DD statements only
    Vector<DDParameters> get_dd_statements() const {
        Vector<DDParameters> result;
        for (const auto& stmt : statements_) {
            if (stmt.type == StatementType::DD && stmt.dd_params) {
                result.push_back(*stmt.dd_params);
            }
        }
        return result;
    }
    
    /// Get DD statement by name
    Optional<DDParameters> get_dd(StringView ddname) const {
        for (const auto& stmt : statements_) {
            if (stmt.type == StatementType::DD && stmt.dd_params && 
                stmt.dd_params->ddname == ddname) {
                return stmt.dd_params;
            }
        }
        return std::nullopt;
    }
    
    /// Get parsing errors
    const Vector<String>& errors() const { return errors_; }
    
    /// Check if parsing succeeded
    bool has_errors() const { return !errors_.empty(); }
};

} // namespace ims::jcl
