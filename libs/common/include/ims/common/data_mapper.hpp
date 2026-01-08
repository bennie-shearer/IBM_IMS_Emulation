/**
 * @file data_mapper.hpp
 * @brief Data mapping and transformation utilities
 * @version 3.6.3
 *
 * Provides data mapping including:
 * - Field-to-field mapping
 * - Type conversions
 * - Record transformations
 * - Mainframe data format handling
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_DATA_MAPPER_HPP
#define IMS_COMMON_DATA_MAPPER_HPP

#include "types.hpp"
#include "result.hpp"

namespace ims::common {

// =============================================================================
// Field Definition
// =============================================================================

/**
 * @brief Field data types
 */
enum class FieldType {
    String,
    Integer,
    Decimal,
    Date,
    Time,
    Timestamp,
    Binary,
    Packed,    // Packed decimal
    Zoned      // Zoned decimal
};

inline String field_type_to_string(FieldType type) {
    switch (type) {
        case FieldType::String: return "String";
        case FieldType::Integer: return "Integer";
        case FieldType::Decimal: return "Decimal";
        case FieldType::Date: return "Date";
        case FieldType::Time: return "Time";
        case FieldType::Timestamp: return "Timestamp";
        case FieldType::Binary: return "Binary";
        case FieldType::Packed: return "Packed";
        case FieldType::Zoned: return "Zoned";
        default: return "Unknown";
    }
}

/**
 * @brief Field definition for mapping
 */
struct FieldDef {
    String name;
    FieldType type{FieldType::String};
    Size offset{0};
    Size length{0};
    Size precision{0};     // For decimal types
    Size scale{0};         // For decimal types
    bool nullable{false};
    String default_value;
    
    FieldDef() = default;
    FieldDef(String n, FieldType t, Size off, Size len)
        : name(std::move(n)), type(t), offset(off), length(len) {}
};

// =============================================================================
// Field Value
// =============================================================================

/**
 * @brief Universal field value holder
 */
class FieldValue {
private:
    FieldType type_{FieldType::String};
    String string_value_;
    Int64 int_value_{0};
    double double_value_{0.0};
    ByteBuffer binary_value_;
    bool is_null_{true};
    
public:
    FieldValue() = default;
    
    explicit FieldValue(const String& s)
        : type_(FieldType::String), string_value_(s), is_null_(false) {}
    
    explicit FieldValue(Int64 i)
        : type_(FieldType::Integer), int_value_(i), is_null_(false) {}
    
    explicit FieldValue(double d)
        : type_(FieldType::Decimal), double_value_(d), is_null_(false) {}
    
    explicit FieldValue(const ByteBuffer& b)
        : type_(FieldType::Binary), binary_value_(b), is_null_(false) {}
    
    static FieldValue null() { return FieldValue(); }
    
    // Type checks
    FieldType type() const { return type_; }
    bool is_null() const { return is_null_; }
    bool is_string() const { return type_ == FieldType::String; }
    bool is_integer() const { return type_ == FieldType::Integer; }
    bool is_decimal() const { return type_ == FieldType::Decimal; }
    bool is_binary() const { return type_ == FieldType::Binary; }
    
    // Getters
    String as_string() const {
        if (is_null_) return "";
        switch (type_) {
            case FieldType::String: return string_value_;
            case FieldType::Integer: return std::to_string(int_value_);
            case FieldType::Decimal: return std::to_string(double_value_);
            default: return "";
        }
    }
    
    Int64 as_integer() const {
        if (is_null_) return 0;
        switch (type_) {
            case FieldType::Integer: return int_value_;
            case FieldType::Decimal: return static_cast<Int64>(double_value_);
            case FieldType::String:
                try { return std::stoll(string_value_); }
                catch (...) { return 0; }
            default: return 0;
        }
    }
    
    double as_decimal() const {
        if (is_null_) return 0.0;
        switch (type_) {
            case FieldType::Decimal: return double_value_;
            case FieldType::Integer: return static_cast<double>(int_value_);
            case FieldType::String:
                try { return std::stod(string_value_); }
                catch (...) { return 0.0; }
            default: return 0.0;
        }
    }
    
    const ByteBuffer& as_binary() const { return binary_value_; }
    
    // Setters
    void set_string(const String& s) {
        type_ = FieldType::String;
        string_value_ = s;
        is_null_ = false;
    }
    
    void set_integer(Int64 i) {
        type_ = FieldType::Integer;
        int_value_ = i;
        is_null_ = false;
    }
    
    void set_decimal(double d) {
        type_ = FieldType::Decimal;
        double_value_ = d;
        is_null_ = false;
    }
    
    void set_null() { is_null_ = true; }
};

// =============================================================================
// Record Definition
// =============================================================================

/**
 * @brief Record layout definition
 */
class RecordDef {
private:
    String name_;
    Vector<FieldDef> fields_;
    HashMap<String, Size> field_index_;
    Size record_length_{0};
    
public:
    RecordDef() = default;
    explicit RecordDef(String name) : name_(std::move(name)) {}
    
    /**
     * @brief Adds a field definition
     */
    RecordDef& add_field(const FieldDef& field) {
        field_index_[field.name] = fields_.size();
        fields_.push_back(field);
        
        Size field_end = field.offset + field.length;
        if (field_end > record_length_) {
            record_length_ = field_end;
        }
        
        return *this;
    }
    
    /**
     * @brief Adds a field with basic parameters
     */
    RecordDef& add_field(const String& name, FieldType type,
                        Size offset, Size length) {
        return add_field(FieldDef(name, type, offset, length));
    }
    
    /**
     * @brief Gets field by name
     */
    const FieldDef* field(const String& name) const {
        auto it = field_index_.find(name);
        return it != field_index_.end() ? &fields_[it->second] : nullptr;
    }
    
    /**
     * @brief Gets field by index
     */
    const FieldDef* field(Size index) const {
        return index < fields_.size() ? &fields_[index] : nullptr;
    }
    
    const String& name() const { return name_; }
    const Vector<FieldDef>& fields() const { return fields_; }
    Size field_count() const { return fields_.size(); }
    Size record_length() const { return record_length_; }
    
    void set_record_length(Size len) { record_length_ = len; }
};

// =============================================================================
// Data Mapper
// =============================================================================

/**
 * @brief Maps data between record formats
 */
class DataMapper {
public:
    using TransformFn = Function<FieldValue(const FieldValue&)>;
    
private:
    struct FieldMapping {
        String source_field;
        String target_field;
        TransformFn transform;
        FieldValue default_value;
    };
    
    RecordDef source_def_;
    RecordDef target_def_;
    Vector<FieldMapping> mappings_;
    
public:
    DataMapper() = default;
    DataMapper(RecordDef source, RecordDef target)
        : source_def_(std::move(source))
        , target_def_(std::move(target)) {}
    
    /**
     * @brief Maps source field to target field
     */
    DataMapper& map(const String& source_field, const String& target_field) {
        mappings_.push_back({source_field, target_field, nullptr, FieldValue()});
        return *this;
    }
    
    /**
     * @brief Maps with transformation function
     */
    DataMapper& map(const String& source_field, const String& target_field,
                   TransformFn transform) {
        mappings_.push_back({source_field, target_field, std::move(transform),
                           FieldValue()});
        return *this;
    }
    
    /**
     * @brief Sets default value for target field
     */
    DataMapper& with_default(const String& target_field, FieldValue value) {
        for (auto& m : mappings_) {
            if (m.target_field == target_field) {
                m.default_value = std::move(value);
                break;
            }
        }
        return *this;
    }
    
    /**
     * @brief Extracts field value from raw record
     */
    FieldValue extract_field(const ByteBuffer& record, const FieldDef& field) const {
        if (field.offset + field.length > record.size()) {
            return FieldValue::null();
        }
        
        switch (field.type) {
            case FieldType::String: {
                String s(record.begin() + field.offset,
                        record.begin() + field.offset + field.length);
                // Trim trailing spaces (common in mainframe)
                while (!s.empty() && s.back() == ' ') s.pop_back();
                return FieldValue(s);
            }
            
            case FieldType::Integer: {
                Int64 value = 0;
                for (Size i = 0; i < field.length && i < 8; ++i) {
                    value = (value << 8) | record[field.offset + i];
                }
                return FieldValue(value);
            }
            
            case FieldType::Packed: {
                // Packed decimal: each byte has 2 digits, last nibble is sign
                Int64 value = 0;
                bool negative = false;
                
                for (Size i = 0; i < field.length; ++i) {
                    Byte b = record[field.offset + i];
                    if (i == field.length - 1) {
                        value = value * 10 + ((b >> 4) & 0x0F);
                        negative = (b & 0x0F) == 0x0D;
                    } else {
                        value = value * 10 + ((b >> 4) & 0x0F);
                        value = value * 10 + (b & 0x0F);
                    }
                }
                
                if (negative) value = -value;
                
                // Apply scale
                double result = static_cast<double>(value);
                for (Size i = 0; i < field.scale; ++i) result /= 10.0;
                
                return FieldValue(result);
            }
            
            case FieldType::Zoned: {
                // Zoned decimal: character digits with sign in zone of last byte
                String digits;
                bool negative = false;
                
                for (Size i = 0; i < field.length; ++i) {
                    Byte b = record[field.offset + i];
                    Byte zone = (b >> 4) & 0x0F;
                    Byte digit = b & 0x0F;
                    
                    if (i == field.length - 1) {
                        negative = (zone == 0x0D);
                    }
                    digits += static_cast<char>('0' + digit);
                }
                
                Int64 value = std::stoll(digits);
                if (negative) value = -value;
                
                double result = static_cast<double>(value);
                for (Size i = 0; i < field.scale; ++i) result /= 10.0;
                
                return FieldValue(result);
            }
            
            case FieldType::Binary: {
                ByteBuffer data(record.begin() + field.offset,
                              record.begin() + field.offset + field.length);
                return FieldValue(data);
            }
            
            default:
                return FieldValue::null();
        }
    }
    
    /**
     * @brief Sets field value in raw record
     */
    void set_field(ByteBuffer& record, const FieldDef& field,
                  const FieldValue& value) const {
        // Ensure record is large enough
        if (record.size() < field.offset + field.length) {
            record.resize(field.offset + field.length, ' ');
        }
        
        switch (field.type) {
            case FieldType::String: {
                String s = value.as_string();
                // Pad or truncate to field length
                s.resize(field.length, ' ');
                std::copy(s.begin(), s.end(), record.begin() + field.offset);
                break;
            }
            
            case FieldType::Integer: {
                Int64 v = value.as_integer();
                for (Size i = field.length; i > 0; --i) {
                    record[field.offset + i - 1] = static_cast<Byte>(v & 0xFF);
                    v >>= 8;
                }
                break;
            }
            
            case FieldType::Packed: {
                double d = value.as_decimal();
                bool negative = d < 0;
                if (negative) d = -d;
                
                // Scale up
                for (Size i = 0; i < field.scale; ++i) d *= 10.0;
                Int64 v = static_cast<Int64>(d + 0.5);  // Round
                
                // Convert to packed
                Vector<Byte> digits;
                while (v > 0) {
                    digits.push_back(static_cast<Byte>(v % 10));
                    v /= 10;
                }
                
                // Build packed bytes from right
                Size byte_idx = field.offset + field.length - 1;
                Size digit_idx = 0;
                
                // Sign nibble
                record[byte_idx] = static_cast<Byte>(
                    (digits.empty() ? 0 : digits[digit_idx++]) << 4 |
                    (negative ? 0x0D : 0x0C));
                
                while (byte_idx > field.offset && digit_idx < digits.size()) {
                    --byte_idx;
                    Byte low = digit_idx < digits.size() ? digits[digit_idx++] : 0;
                    Byte high = digit_idx < digits.size() ? digits[digit_idx++] : 0;
                    record[byte_idx] = static_cast<Byte>((high << 4) | low);
                }
                break;
            }
            
            default:
                break;
        }
    }
    
    /**
     * @brief Transforms a source record to target format
     */
    Result<ByteBuffer> transform(const ByteBuffer& source) const {
        ByteBuffer target(target_def_.record_length(), ' ');
        
        for (const auto& mapping : mappings_) {
            const FieldDef* src_field = source_def_.field(mapping.source_field);
            const FieldDef* tgt_field = target_def_.field(mapping.target_field);
            
            if (!tgt_field) continue;
            
            FieldValue value;
            
            if (src_field) {
                value = extract_field(source, *src_field);
                if (mapping.transform) {
                    value = mapping.transform(value);
                }
            } else {
                value = mapping.default_value;
            }
            
            if (!value.is_null()) {
                set_field(target, *tgt_field, value);
            }
        }
        
        return Result<ByteBuffer>::ok(target);
    }
    
    const RecordDef& source_def() const { return source_def_; }
    const RecordDef& target_def() const { return target_def_; }
};

// =============================================================================
// Common Transformations
// =============================================================================

namespace transforms {

/**
 * @brief Converts string to uppercase
 */
inline FieldValue to_upper(const FieldValue& v) {
    if (v.is_null()) return v;
    String s = v.as_string();
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return FieldValue(s);
}

/**
 * @brief Converts string to lowercase
 */
inline FieldValue to_lower(const FieldValue& v) {
    if (v.is_null()) return v;
    String s = v.as_string();
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return FieldValue(s);
}

/**
 * @brief Trims whitespace
 */
inline FieldValue trim(const FieldValue& v) {
    if (v.is_null()) return v;
    String s = v.as_string();
    Size start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end-1]))) --end;
    return FieldValue(s.substr(start, end - start));
}

/**
 * @brief Scales numeric value
 */
inline auto scale(double factor) {
    return [factor](const FieldValue& v) -> FieldValue {
        if (v.is_null()) return v;
        return FieldValue(v.as_decimal() * factor);
    };
}

/**
 * @brief Formats date (YYYYMMDD to YYYY-MM-DD)
 */
inline FieldValue format_date(const FieldValue& v) {
    if (v.is_null()) return v;
    String s = v.as_string();
    if (s.size() == 8) {
        return FieldValue(s.substr(0,4) + "-" + s.substr(4,2) + "-" + s.substr(6,2));
    }
    return v;
}

} // namespace transforms

} // namespace ims::common

#endif // IMS_COMMON_DATA_MAPPER_HPP
