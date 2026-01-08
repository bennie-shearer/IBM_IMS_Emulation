/**
 * @file uuid.hpp
 * @brief UUID generation without external dependencies
 * @version 3.6.2
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_UUID_HPP
#define IMS_COMMON_UUID_HPP

#include "types.hpp"
#include <array>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>

namespace ims::common {

/**
 * @brief 128-bit Universally Unique Identifier (UUID)
 * 
 * Implements Version 4 (random) UUIDs per RFC 4122.
 */
class UUID {
public:
    static constexpr Size SIZE = 16;
    
    /**
     * @brief Create a nil (all zeros) UUID
     */
    UUID() {
        data_.fill(0);
    }
    
    /**
     * @brief Create UUID from raw bytes
     */
    explicit UUID(const std::array<UInt8, SIZE>& data) : data_(data) {}
    
    /**
     * @brief Create UUID from raw byte pointer
     */
    explicit UUID(const UInt8* data) {
        std::memcpy(data_.data(), data, SIZE);
    }
    
    /**
     * @brief Check if UUID is nil (all zeros)
     */
    bool is_nil() const {
        for (const auto& byte : data_) {
            if (byte != 0) return false;
        }
        return true;
    }
    
    /**
     * @brief Get UUID version (4 for random UUIDs)
     */
    int version() const {
        return (data_[6] >> 4) & 0x0F;
    }
    
    /**
     * @brief Get UUID variant
     */
    int variant() const {
        UInt8 v = data_[8];
        if ((v & 0x80) == 0x00) return 0;      // NCS backward compatibility
        if ((v & 0xC0) == 0x80) return 1;      // RFC 4122 (standard)
        if ((v & 0xE0) == 0xC0) return 2;      // Microsoft
        return 3;                               // Reserved
    }
    
    /**
     * @brief Convert to string format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
     */
    String to_string() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        
        for (Size i = 0; i < SIZE; ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                oss << '-';
            }
            oss << std::setw(2) << static_cast<int>(data_[i]);
        }
        
        return oss.str();
    }
    
    /**
     * @brief Convert to compact string (no dashes)
     */
    String to_compact_string() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        
        for (Size i = 0; i < SIZE; ++i) {
            oss << std::setw(2) << static_cast<int>(data_[i]);
        }
        
        return oss.str();
    }
    
    /**
     * @brief Parse UUID from string
     */
    static std::optional<UUID> parse(const String& str) {
        String clean;
        clean.reserve(32);
        
        for (char c : str) {
            if (c == '-' || c == '{' || c == '}') continue;
            if (!std::isxdigit(static_cast<unsigned char>(c))) {
                return std::nullopt;
            }
            clean += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        
        if (clean.length() != 32) {
            return std::nullopt;
        }
        
        std::array<UInt8, SIZE> data;
        for (Size i = 0; i < SIZE; ++i) {
            int high = hex_to_int(clean[i * 2]);
            int low = hex_to_int(clean[i * 2 + 1]);
            if (high < 0 || low < 0) return std::nullopt;
            data[i] = static_cast<UInt8>((high << 4) | low);
        }
        
        return UUID(data);
    }
    
    /**
     * @brief Get raw bytes
     */
    const std::array<UInt8, SIZE>& bytes() const { return data_; }
    std::array<UInt8, SIZE>& bytes() { return data_; }
    
    const UInt8* data() const { return data_.data(); }
    UInt8* data() { return data_.data(); }
    
    // Comparison operators
    bool operator==(const UUID& other) const { return data_ == other.data_; }
    bool operator!=(const UUID& other) const { return data_ != other.data_; }
    bool operator<(const UUID& other) const { return data_ < other.data_; }
    bool operator<=(const UUID& other) const { return data_ <= other.data_; }
    bool operator>(const UUID& other) const { return data_ > other.data_; }
    bool operator>=(const UUID& other) const { return data_ >= other.data_; }
    
    /**
     * @brief Hash value for use in containers
     */
    Size hash() const {
        // FNV-1a hash
        Size h = 14695981039346656037ULL;
        for (const auto& byte : data_) {
            h ^= static_cast<Size>(byte);
            h *= 1099511628211ULL;
        }
        return h;
    }

private:
    std::array<UInt8, SIZE> data_;
    
    static int hex_to_int(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
};

/**
 * @brief Thread-safe UUID generator
 * 
 * Generates Version 4 (random) UUIDs.
 */
class UUIDGenerator {
public:
    /**
     * @brief Generate a new random UUID (Version 4)
     */
    static UUID generate() {
        static UUIDGenerator generator;
        return generator.next();
    }
    
    /**
     * @brief Generate next UUID from this generator instance
     */
    UUID next() {
        std::array<UInt8, UUID::SIZE> data;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Generate random bytes
            std::uniform_int_distribution<int> dist(0, 255);
            for (auto& byte : data) {
                byte = static_cast<UInt8>(dist(rng_));
            }
        }
        
        // Set version 4
        data[6] = (data[6] & 0x0F) | 0x40;
        
        // Set variant (RFC 4122)
        data[8] = (data[8] & 0x3F) | 0x80;
        
        return UUID(data);
    }
    
    /**
     * @brief Generate multiple UUIDs efficiently
     */
    std::vector<UUID> generate_batch(Size count) {
        std::vector<UUID> result;
        result.reserve(count);
        
        for (Size i = 0; i < count; ++i) {
            result.push_back(next());
        }
        
        return result;
    }
    
    /**
     * @brief Reseed the random number generator
     */
    void reseed() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::random_device rd;
        rng_.seed(rd());
    }

private:
    UUIDGenerator() {
        std::random_device rd;
        rng_.seed(rd());
    }
    
    std::mutex mutex_;
    std::mt19937_64 rng_;
};

/**
 * @brief Create a nil UUID
 */
inline UUID nil_uuid() {
    return UUID();
}

/**
 * @brief Create a UUID from string (throws on invalid)
 */
inline UUID uuid_from_string(const String& str) {
    auto result = UUID::parse(str);
    if (!result) {
        throw std::invalid_argument("Invalid UUID string: " + str);
    }
    return *result;
}

}  // namespace ims::common

// Hash specialization for std::unordered_map/set
namespace std {
    template<>
    struct hash<ims::common::UUID> {
        std::size_t operator()(const ims::common::UUID& uuid) const noexcept {
            return uuid.hash();
        }
    };
}

#endif  // IMS_COMMON_UUID_HPP
