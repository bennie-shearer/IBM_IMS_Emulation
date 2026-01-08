/**
 * @file byte_buffer.hpp
 * @brief Enhanced byte buffer utilities for binary data manipulation
 * @version 3.6.3
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_BYTE_BUFFER_HPP
#define IMS_COMMON_BYTE_BUFFER_HPP

#include "types.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ims::common {

/**
 * @brief Non-owning view into byte data
 */
class ByteBufferView {
public:
    ByteBufferView() : data_(nullptr), size_(0) {}
    ByteBufferView(const Byte* data, Size size) : data_(data), size_(size) {}
    ByteBufferView(const std::vector<Byte>& vec) : data_(vec.data()), size_(vec.size()) {}
    
    const Byte* data() const { return data_; }
    Size size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    Byte operator[](Size index) const {
        if (index >= size_) throw std::out_of_range("ByteBufferView index out of range");
        return data_[index];
    }
    
    ByteBufferView subview(Size offset, Size length) const {
        if (offset + length > size_) throw std::out_of_range("Subview out of range");
        return ByteBufferView(data_ + offset, length);
    }
    
    const Byte* begin() const { return data_; }
    const Byte* end() const { return data_ + size_; }
    
    bool operator==(const ByteBufferView& other) const {
        return size_ == other.size_ && std::memcmp(data_, other.data_, size_) == 0;
    }

private:
    const Byte* data_;
    Size size_;
};

/**
 * @brief Dynamic byte buffer with read/write operations
 */
class ByteBuffer {
public:
    explicit ByteBuffer(Size initial_capacity = 256)
        : position_(0), capacity_(initial_capacity) {
        data_.reserve(initial_capacity);
    }
    
    explicit ByteBuffer(const std::vector<Byte>& data)
        : data_(data), position_(0), capacity_(data.size()) {}
    
    ByteBuffer(const Byte* data, Size size)
        : data_(data, data + size), position_(0), capacity_(size) {}

    // Position management
    Size position() const { return position_; }
    Size size() const { return data_.size(); }
    Size remaining() const { return size() > position_ ? size() - position_ : 0; }
    bool empty() const { return data_.empty(); }
    
    void seek(Size pos) {
        if (pos > data_.size()) throw std::out_of_range("Seek position out of range");
        position_ = pos;
    }
    
    void rewind() { position_ = 0; }
    void skip(Size bytes) { seek(position_ + bytes); }
    
    // Data access
    const Byte* data() const { return data_.data(); }
    Byte* data() { return data_.data(); }
    std::vector<Byte>& raw() { return data_; }
    const std::vector<Byte>& raw() const { return data_; }
    
    ByteBufferView view() const { return ByteBufferView(data_.data(), data_.size()); }
    ByteBufferView view(Size offset, Size length) const {
        return ByteBufferView(data_.data() + offset, length);
    }
    
    void clear() {
        data_.clear();
        position_ = 0;
    }
    
    void resize(Size new_size) {
        data_.resize(new_size);
        if (position_ > new_size) position_ = new_size;
    }

    // Write operations - Big Endian
    void write_uint8(UInt8 value) {
        ensure_capacity(1);
        data_.push_back(value);
    }
    
    void write_uint16_be(UInt16 value) {
        ensure_capacity(2);
        data_.push_back(static_cast<Byte>((value >> 8) & 0xFF));
        data_.push_back(static_cast<Byte>(value & 0xFF));
    }
    
    void write_uint32_be(UInt32 value) {
        ensure_capacity(4);
        data_.push_back(static_cast<Byte>((value >> 24) & 0xFF));
        data_.push_back(static_cast<Byte>((value >> 16) & 0xFF));
        data_.push_back(static_cast<Byte>((value >> 8) & 0xFF));
        data_.push_back(static_cast<Byte>(value & 0xFF));
    }
    
    void write_uint64_be(UInt64 value) {
        ensure_capacity(8);
        for (int i = 7; i >= 0; --i) {
            data_.push_back(static_cast<Byte>((value >> (i * 8)) & 0xFF));
        }
    }

    // Write operations - Little Endian
    void write_uint16_le(UInt16 value) {
        ensure_capacity(2);
        data_.push_back(static_cast<Byte>(value & 0xFF));
        data_.push_back(static_cast<Byte>((value >> 8) & 0xFF));
    }
    
    void write_uint32_le(UInt32 value) {
        ensure_capacity(4);
        data_.push_back(static_cast<Byte>(value & 0xFF));
        data_.push_back(static_cast<Byte>((value >> 8) & 0xFF));
        data_.push_back(static_cast<Byte>((value >> 16) & 0xFF));
        data_.push_back(static_cast<Byte>((value >> 24) & 0xFF));
    }
    
    void write_uint64_le(UInt64 value) {
        ensure_capacity(8);
        for (int i = 0; i < 8; ++i) {
            data_.push_back(static_cast<Byte>((value >> (i * 8)) & 0xFF));
        }
    }

    // String write operations
    void write_bytes(const Byte* bytes, Size length) {
        ensure_capacity(length);
        data_.insert(data_.end(), bytes, bytes + length);
    }
    
    void write_bytes(const std::vector<Byte>& bytes) {
        write_bytes(bytes.data(), bytes.size());
    }
    
    void write_string(const String& str) {
        write_bytes(reinterpret_cast<const Byte*>(str.data()), str.size());
    }
    
    void write_string_padded(const String& str, Size length, char pad = ' ') {
        ensure_capacity(length);
        Size copy_len = std::min(str.size(), length);
        for (Size i = 0; i < copy_len; ++i) {
            data_.push_back(static_cast<Byte>(str[i]));
        }
        for (Size i = copy_len; i < length; ++i) {
            data_.push_back(static_cast<Byte>(pad));
        }
    }
    
    void write_string_null_terminated(const String& str) {
        write_string(str);
        write_uint8(0);
    }

    // Read operations - Big Endian
    UInt8 read_uint8() {
        check_remaining(1);
        return data_[position_++];
    }
    
    UInt16 read_uint16_be() {
        check_remaining(2);
        UInt16 value = (static_cast<UInt16>(data_[position_]) << 8) |
                       static_cast<UInt16>(data_[position_ + 1]);
        position_ += 2;
        return value;
    }
    
    UInt32 read_uint32_be() {
        check_remaining(4);
        UInt32 value = (static_cast<UInt32>(data_[position_]) << 24) |
                       (static_cast<UInt32>(data_[position_ + 1]) << 16) |
                       (static_cast<UInt32>(data_[position_ + 2]) << 8) |
                       static_cast<UInt32>(data_[position_ + 3]);
        position_ += 4;
        return value;
    }
    
    UInt64 read_uint64_be() {
        check_remaining(8);
        UInt64 value = 0;
        for (int i = 0; i < 8; ++i) {
            value = (value << 8) | static_cast<UInt64>(data_[position_ + i]);
        }
        position_ += 8;
        return value;
    }

    // Read operations - Little Endian
    UInt16 read_uint16_le() {
        check_remaining(2);
        UInt16 value = static_cast<UInt16>(data_[position_]) |
                       (static_cast<UInt16>(data_[position_ + 1]) << 8);
        position_ += 2;
        return value;
    }
    
    UInt32 read_uint32_le() {
        check_remaining(4);
        UInt32 value = static_cast<UInt32>(data_[position_]) |
                       (static_cast<UInt32>(data_[position_ + 1]) << 8) |
                       (static_cast<UInt32>(data_[position_ + 2]) << 16) |
                       (static_cast<UInt32>(data_[position_ + 3]) << 24);
        position_ += 4;
        return value;
    }
    
    UInt64 read_uint64_le() {
        check_remaining(8);
        UInt64 value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<UInt64>(data_[position_ + i]) << (i * 8);
        }
        position_ += 8;
        return value;
    }

    // String read operations
    std::vector<Byte> read_bytes(Size length) {
        check_remaining(length);
        std::vector<Byte> result(data_.begin() + position_,
                                  data_.begin() + position_ + length);
        position_ += length;
        return result;
    }
    
    String read_string(Size length) {
        check_remaining(length);
        String result(reinterpret_cast<const char*>(data_.data() + position_), length);
        position_ += length;
        return result;
    }
    
    String read_string_trimmed(Size length, char trim_char = ' ') {
        String result = read_string(length);
        // Trim trailing characters
        Size end = result.size();
        while (end > 0 && result[end - 1] == trim_char) --end;
        return result.substr(0, end);
    }
    
    String read_string_null_terminated() {
        Size start = position_;
        while (position_ < data_.size() && data_[position_] != 0) {
            ++position_;
        }
        String result(reinterpret_cast<const char*>(data_.data() + start),
                      position_ - start);
        if (position_ < data_.size()) ++position_;  // Skip null
        return result;
    }

    // Pattern search
    std::optional<Size> find(const Byte* pattern, Size pattern_len, Size start_pos = 0) const {
        if (pattern_len == 0 || pattern_len > data_.size()) return std::nullopt;
        
        for (Size i = start_pos; i <= data_.size() - pattern_len; ++i) {
            if (std::memcmp(data_.data() + i, pattern, pattern_len) == 0) {
                return i;
            }
        }
        return std::nullopt;
    }
    
    std::optional<Size> find(const std::vector<Byte>& pattern, Size start_pos = 0) const {
        return find(pattern.data(), pattern.size(), start_pos);
    }

    // Hexdump formatting
    String hexdump(Size bytes_per_line = 16) const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        
        for (Size i = 0; i < data_.size(); i += bytes_per_line) {
            // Address
            oss << std::setw(8) << i << "  ";
            
            // Hex bytes
            for (Size j = 0; j < bytes_per_line; ++j) {
                if (i + j < data_.size()) {
                    oss << std::setw(2) << static_cast<int>(data_[i + j]) << " ";
                } else {
                    oss << "   ";
                }
                if (j == 7) oss << " ";
            }
            
            oss << " |";
            
            // ASCII representation
            for (Size j = 0; j < bytes_per_line && i + j < data_.size(); ++j) {
                char c = static_cast<char>(data_[i + j]);
                oss << (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
            }
            
            oss << "|\n";
        }
        
        return oss.str();
    }

private:
    std::vector<Byte> data_;
    Size position_;
    Size capacity_;
    
    void ensure_capacity(Size additional) {
        Size needed = data_.size() + additional;
        if (needed > capacity_) {
            capacity_ = std::max(capacity_ * 2, needed);
            data_.reserve(capacity_);
        }
    }
    
    void check_remaining(Size needed) const {
        if (position_ + needed > data_.size()) {
            throw std::out_of_range("ByteBuffer read past end");
        }
    }
};

/**
 * @brief Fixed-size stack-allocated byte buffer
 */
template<Size N>
class FixedByteBuffer {
public:
    FixedByteBuffer() : size_(0), position_(0) {
        data_.fill(0);
    }
    
    static constexpr Size capacity() { return N; }
    Size size() const { return size_; }
    Size position() const { return position_; }
    Size remaining() const { return size_ > position_ ? size_ - position_ : 0; }
    
    void seek(Size pos) {
        if (pos > size_) throw std::out_of_range("Seek position out of range");
        position_ = pos;
    }
    
    void rewind() { position_ = 0; }
    
    void clear() {
        size_ = 0;
        position_ = 0;
    }
    
    const Byte* data() const { return data_.data(); }
    Byte* data() { return data_.data(); }
    
    // Write operations
    bool write_uint8(UInt8 value) {
        if (size_ >= N) return false;
        data_[size_++] = value;
        return true;
    }
    
    bool write_bytes(const Byte* bytes, Size length) {
        if (size_ + length > N) return false;
        std::memcpy(data_.data() + size_, bytes, length);
        size_ += length;
        return true;
    }
    
    // Read operations
    UInt8 read_uint8() {
        if (position_ >= size_) throw std::out_of_range("Read past end");
        return data_[position_++];
    }
    
    ByteBufferView view() const {
        return ByteBufferView(data_.data(), size_);
    }

private:
    std::array<Byte, N> data_;
    Size size_;
    Size position_;
};

}  // namespace ims::common

#endif  // IMS_COMMON_BYTE_BUFFER_HPP
