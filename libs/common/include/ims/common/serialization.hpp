#pragma once

// =============================================================================
// IBM IMS (Information Management System) Emulation Enterprise - Serialization Framework
// Version: 3.6.3
// NEW in v3.6.3: Binary serialization support
// =============================================================================

#include "types.hpp"
#include "error.hpp"
#include <cstring>
#include <type_traits>

namespace ims {

// =============================================================================
// Byte Order Utilities
// =============================================================================

enum class ByteOrder {
    LITTLE_ENDIAN_ORDER,
    BIG_ENDIAN_ORDER,
    NATIVE_ORDER
};

inline ByteOrder native_byte_order() {
    union { UInt32 i; char c[4]; } bint = {0x01020304};
    return bint.c[0] == 1 ? ByteOrder::BIG_ENDIAN_ORDER : ByteOrder::LITTLE_ENDIAN_ORDER;
}

template<typename T>
T swap_bytes(T value) {
    static_assert(std::is_integral_v<T>, "swap_bytes requires integral type");
    
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>((value >> 8) | (value << 8));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(
            ((value >> 24) & 0x000000FF) |
            ((value >> 8)  & 0x0000FF00) |
            ((value << 8)  & 0x00FF0000) |
            ((value << 24) & 0xFF000000)
        );
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(
            ((value >> 56) & 0x00000000000000FFULL) |
            ((value >> 40) & 0x000000000000FF00ULL) |
            ((value >> 24) & 0x0000000000FF0000ULL) |
            ((value >> 8)  & 0x00000000FF000000ULL) |
            ((value << 8)  & 0x000000FF00000000ULL) |
            ((value << 24) & 0x0000FF0000000000ULL) |
            ((value << 40) & 0x00FF000000000000ULL) |
            ((value << 56) & 0xFF00000000000000ULL)
        );
    }
}

// =============================================================================
// Binary Writer
// =============================================================================

class BinaryWriter {
private:
    ByteBuffer& buffer_;
    ByteOrder byte_order_;
    Size position_{0};
    
    template<typename T>
    void write_raw(T value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        
        if constexpr (std::is_integral_v<T> && sizeof(T) > 1) {
            if (byte_order_ != ByteOrder::NATIVE_ORDER && byte_order_ != native_byte_order()) {
                value = swap_bytes(value);
            }
        }
        
        Size needed = position_ + sizeof(T);
        if (buffer_.size() < needed) {
            buffer_.resize(needed);
        }
        
        std::memcpy(buffer_.data() + position_, &value, sizeof(T));
        position_ += sizeof(T);
    }
    
public:
    explicit BinaryWriter(ByteBuffer& buffer, ByteOrder order = ByteOrder::LITTLE_ENDIAN_ORDER)
        : buffer_(buffer), byte_order_(order) {}
    
    // Primitives
    void write_int8(Int8 v) { write_raw(v); }
    void write_int16(Int16 v) { write_raw(v); }
    void write_int32(Int32 v) { write_raw(v); }
    void write_int64(Int64 v) { write_raw(v); }
    void write_uint8(UInt8 v) { write_raw(v); }
    void write_uint16(UInt16 v) { write_raw(v); }
    void write_uint32(UInt32 v) { write_raw(v); }
    void write_uint64(UInt64 v) { write_raw(v); }
    void write_float32(Float32 v) { write_raw(v); }
    void write_float64(Float64 v) { write_raw(v); }
    void write_bool(bool v) { write_raw(static_cast<UInt8>(v ? 1 : 0)); }
    
    // Byte array
    void write_bytes(const Byte* data, Size length) {
        Size needed = position_ + length;
        if (buffer_.size() < needed) {
            buffer_.resize(needed);
        }
        std::memcpy(buffer_.data() + position_, data, length);
        position_ += length;
    }
    
    void write_bytes(const ByteBuffer& data) {
        write_bytes(data.data(), data.size());
    }
    
    // String (length-prefixed)
    void write_string(const String& s) {
        write_uint32(static_cast<UInt32>(s.size()));
        write_bytes(reinterpret_cast<const Byte*>(s.data()), s.size());
    }
    
    // Fixed-length string (padded with zeros)
    void write_fixed_string(const String& s, Size length) {
        Size copy_len = std::min(s.size(), length);
        
        Size needed = position_ + length;
        if (buffer_.size() < needed) {
            buffer_.resize(needed);
        }
        
        std::memcpy(buffer_.data() + position_, s.data(), copy_len);
        if (copy_len < length) {
            std::memset(buffer_.data() + position_ + copy_len, 0, length - copy_len);
        }
        position_ += length;
    }
    
    // Vector of primitives
    template<typename T>
    void write_vector(const Vector<T>& vec) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        write_uint32(static_cast<UInt32>(vec.size()));
        for (const auto& item : vec) {
            write_raw(item);
        }
    }
    
    // Position control
    Size position() const { return position_; }
    void seek(Size pos) { position_ = pos; }
    void skip(Size bytes) { position_ += bytes; }
    Size size() const { return buffer_.size(); }
    
    // Reserve space
    void reserve(Size additional) {
        buffer_.reserve(buffer_.size() + additional);
    }
};

// =============================================================================
// Binary Reader
// =============================================================================

class BinaryReader {
private:
    const Byte* data_;
    Size size_;
    Size position_{0};
    ByteOrder byte_order_;
    
    template<typename T>
    T read_raw() {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        
        if (position_ + sizeof(T) > size_) {
            throw std::runtime_error("BinaryReader: read past end of buffer");
        }
        
        T value;
        std::memcpy(&value, data_ + position_, sizeof(T));
        position_ += sizeof(T);
        
        if constexpr (std::is_integral_v<T> && sizeof(T) > 1) {
            if (byte_order_ != ByteOrder::NATIVE_ORDER && byte_order_ != native_byte_order()) {
                value = swap_bytes(value);
            }
        }
        
        return value;
    }
    
public:
    BinaryReader(const Byte* data, Size size, ByteOrder order = ByteOrder::LITTLE_ENDIAN_ORDER)
        : data_(data), size_(size), byte_order_(order) {}
    
    BinaryReader(const ByteBuffer& buffer, ByteOrder order = ByteOrder::LITTLE_ENDIAN_ORDER)
        : data_(buffer.data()), size_(buffer.size()), byte_order_(order) {}
    
    // Primitives
    Int8 read_int8() { return read_raw<Int8>(); }
    Int16 read_int16() { return read_raw<Int16>(); }
    Int32 read_int32() { return read_raw<Int32>(); }
    Int64 read_int64() { return read_raw<Int64>(); }
    UInt8 read_uint8() { return read_raw<UInt8>(); }
    UInt16 read_uint16() { return read_raw<UInt16>(); }
    UInt32 read_uint32() { return read_raw<UInt32>(); }
    UInt64 read_uint64() { return read_raw<UInt64>(); }
    Float32 read_float32() { return read_raw<Float32>(); }
    Float64 read_float64() { return read_raw<Float64>(); }
    bool read_bool() { return read_raw<UInt8>() != 0; }
    
    // Byte array
    void read_bytes(Byte* dest, Size length) {
        if (position_ + length > size_) {
            throw std::runtime_error("BinaryReader: read past end of buffer");
        }
        std::memcpy(dest, data_ + position_, length);
        position_ += length;
    }
    
    ByteBuffer read_bytes(Size length) {
        ByteBuffer result(length);
        read_bytes(result.data(), length);
        return result;
    }
    
    // String (length-prefixed)
    String read_string() {
        UInt32 length = read_uint32();
        if (position_ + length > size_) {
            throw std::runtime_error("BinaryReader: string length exceeds buffer");
        }
        String result(reinterpret_cast<const char*>(data_ + position_), length);
        position_ += length;
        return result;
    }
    
    // Fixed-length string
    String read_fixed_string(Size length) {
        if (position_ + length > size_) {
            throw std::runtime_error("BinaryReader: read past end of buffer");
        }
        String result(reinterpret_cast<const char*>(data_ + position_), length);
        position_ += length;
        
        // Trim trailing nulls
        auto pos = result.find('\0');
        if (pos != String::npos) {
            result.resize(pos);
        }
        return result;
    }
    
    // Vector of primitives
    template<typename T>
    Vector<T> read_vector() {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        UInt32 count = read_uint32();
        Vector<T> result(count);
        for (UInt32 i = 0; i < count; ++i) {
            result[i] = read_raw<T>();
        }
        return result;
    }
    
    // Position control
    Size position() const { return position_; }
    void seek(Size pos) {
        if (pos > size_) {
            throw std::runtime_error("BinaryReader: seek past end of buffer");
        }
        position_ = pos;
    }
    void skip(Size bytes) {
        if (position_ + bytes > size_) {
            throw std::runtime_error("BinaryReader: skip past end of buffer");
        }
        position_ += bytes;
    }
    Size remaining() const { return size_ - position_; }
    Size size() const { return size_; }
    bool eof() const { return position_ >= size_; }
    
    // Peek without advancing
    template<typename T>
    T peek() {
        Size saved = position_;
        T result = read_raw<T>();
        position_ = saved;
        return result;
    }
};

// =============================================================================
// Serializable Interface
// =============================================================================

class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void serialize(BinaryWriter& writer) const = 0;
    virtual void deserialize(BinaryReader& reader) = 0;
};

// =============================================================================
// Convenience Functions
// =============================================================================

template<typename T>
ByteBuffer serialize(const T& obj) requires requires { obj.serialize(std::declval<BinaryWriter&>()); } {
    ByteBuffer buffer;
    BinaryWriter writer(buffer);
    obj.serialize(writer);
    return buffer;
}

template<typename T>
T deserialize(const ByteBuffer& buffer) requires std::is_default_constructible_v<T> {
    T obj;
    BinaryReader reader(buffer);
    obj.deserialize(reader);
    return obj;
}

} // namespace ims
