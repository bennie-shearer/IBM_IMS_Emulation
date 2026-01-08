/**
 * @file compression.hpp
 * @brief Data compression utilities with zero external dependencies
 * @version 3.6.2
 *
 * Provides compression functionality including:
 * - Run-length encoding (RLE)
 * - Dictionary-based compression
 * - Optimized for mainframe record formats
 * - Zero external dependencies
 *
 * Copyright (c) 2025 Bennie Shearer
 * MIT License - See LICENSE file for details
 */

#ifndef IMS_COMMON_COMPRESSION_HPP
#define IMS_COMMON_COMPRESSION_HPP

#include "types.hpp"
#include <algorithm>
#include <cstring>
#include <numeric>

namespace ims::common {

// =============================================================================
// Compression Result
// =============================================================================

/**
 * @brief Result of compression/decompression operation
 */
struct CompressionResult {
    bool success{true};
    ByteBuffer data;
    Size original_size{0};
    Size compressed_size{0};
    String error;
    
    double ratio() const {
        if (original_size == 0) return 0.0;
        return 1.0 - (static_cast<double>(compressed_size) / 
                      static_cast<double>(original_size));
    }
    
    double expansion() const {
        if (original_size == 0) return 0.0;
        return static_cast<double>(compressed_size) / 
               static_cast<double>(original_size);
    }
    
    static CompressionResult ok(ByteBuffer data, Size orig, Size comp) {
        return {true, std::move(data), orig, comp, ""};
    }
    
    static CompressionResult fail(const String& error) {
        return {false, {}, 0, 0, error};
    }
};

// =============================================================================
// Run-Length Encoding (RLE)
// =============================================================================

/**
 * @brief Run-length encoding for repetitive data
 * 
 * Format: For runs of 3 or more identical bytes:
 *   [escape_byte] [count] [value]
 * For other data:
 *   [value] (if value != escape_byte)
 *   [escape_byte] [0] [escape_byte] (for literal escape byte)
 */
class RleCompressor {
private:
    Byte escape_byte_{0xFF};
    Size min_run_length_{3};
    
public:
    RleCompressor() = default;
    
    /**
     * @brief Sets the escape byte used for encoding
     */
    void set_escape_byte(Byte escape) { escape_byte_ = escape; }
    
    /**
     * @brief Sets minimum run length for compression
     */
    void set_min_run_length(Size length) { min_run_length_ = length; }
    
    /**
     * @brief Compresses data using RLE
     */
    CompressionResult compress(const ByteBuffer& input) {
        if (input.empty()) {
            return CompressionResult::ok({}, 0, 0);
        }
        
        ByteBuffer output;
        output.reserve(input.size());
        
        Size i = 0;
        while (i < input.size()) {
            Byte current = input[i];
            Size run_length = 1;
            
            // Count consecutive identical bytes
            while (i + run_length < input.size() && 
                   input[i + run_length] == current && 
                   run_length < 255) {
                ++run_length;
            }
            
            if (run_length >= min_run_length_) {
                // Encode as run
                output.push_back(escape_byte_);
                output.push_back(static_cast<Byte>(run_length));
                output.push_back(current);
                i += run_length;
            } else {
                // Encode literally
                if (current == escape_byte_) {
                    // Escape the escape byte
                    output.push_back(escape_byte_);
                    output.push_back(0);
                    output.push_back(escape_byte_);
                } else {
                    output.push_back(current);
                }
                ++i;
            }
        }
        
        return CompressionResult::ok(std::move(output), input.size(), output.size());
    }
    
    /**
     * @brief Decompresses RLE data
     */
    CompressionResult decompress(const ByteBuffer& input) {
        if (input.empty()) {
            return CompressionResult::ok({}, 0, 0);
        }
        
        ByteBuffer output;
        output.reserve(input.size() * 2);  // Estimate
        
        Size i = 0;
        while (i < input.size()) {
            if (input[i] == escape_byte_) {
                if (i + 2 >= input.size()) {
                    return CompressionResult::fail("Truncated RLE sequence");
                }
                
                Byte count = input[i + 1];
                Byte value = input[i + 2];
                
                if (count == 0) {
                    // Literal escape byte
                    output.push_back(escape_byte_);
                } else {
                    // Run of bytes
                    for (Size j = 0; j < count; ++j) {
                        output.push_back(value);
                    }
                }
                
                i += 3;
            } else {
                output.push_back(input[i]);
                ++i;
            }
        }
        
        return CompressionResult::ok(std::move(output), input.size(), output.size());
    }
};

// =============================================================================
// Dictionary-Based Compression (LZW-like)
// =============================================================================

/**
 * @brief Simple dictionary-based compression
 * 
 * Uses a sliding window dictionary for pattern matching.
 * Suitable for structured mainframe records with repeating fields.
 */
class DictionaryCompressor {
private:
    Size max_dict_size_{4096};
    Size min_match_length_{3};
    Size max_match_length_{255};
    Size window_size_{4096};
    
    struct Match {
        Size offset;
        Size length;
        Byte next_byte;
    };
    
    Match find_best_match(const ByteBuffer& data, Size pos, 
                         Size window_start) const {
        Match best{0, 0, 0};
        
        if (pos >= data.size()) return best;
        
        best.next_byte = data[pos];
        
        Size search_start = pos > window_size_ ? pos - window_size_ : window_start;
        
        for (Size i = search_start; i < pos; ++i) {
            Size match_len = 0;
            while (pos + match_len < data.size() && 
                   match_len < max_match_length_ &&
                   data[i + match_len] == data[pos + match_len]) {
                ++match_len;
            }
            
            if (match_len > best.length && match_len >= min_match_length_) {
                best.offset = pos - i;
                best.length = match_len;
                if (pos + match_len < data.size()) {
                    best.next_byte = data[pos + match_len];
                }
            }
        }
        
        return best;
    }
    
public:
    DictionaryCompressor() = default;
    
    /**
     * @brief Sets the window size for pattern matching
     */
    void set_window_size(Size size) { window_size_ = size; }
    
    /**
     * @brief Sets minimum match length
     */
    void set_min_match_length(Size length) { min_match_length_ = length; }
    
    /**
     * @brief Compresses data using dictionary matching
     * 
     * Output format:
     * - Flag byte: bit 7 = is_match, bits 0-6 = count
     * - If literal: followed by 'count' literal bytes
     * - If match: followed by 2-byte offset (big-endian), then length byte
     */
    CompressionResult compress(const ByteBuffer& input) {
        if (input.empty()) {
            return CompressionResult::ok({}, 0, 0);
        }
        
        ByteBuffer output;
        output.reserve(input.size());
        
        ByteBuffer literal_buffer;
        Size pos = 0;
        
        auto flush_literals = [&]() {
            while (!literal_buffer.empty()) {
                Size chunk_size = std::min(literal_buffer.size(), Size{127});
                output.push_back(static_cast<Byte>(chunk_size));  // Flag byte, bit 7 = 0
                output.insert(output.end(), 
                             literal_buffer.begin(), 
                             literal_buffer.begin() + chunk_size);
                literal_buffer.erase(literal_buffer.begin(), 
                                    literal_buffer.begin() + chunk_size);
            }
        };
        
        while (pos < input.size()) {
            Match match = find_best_match(input, pos, 0);
            
            if (match.length >= min_match_length_) {
                // Flush any pending literals
                flush_literals();
                
                // Output match
                Byte flag = 0x80 | static_cast<Byte>(std::min(match.length, Size{127}));
                output.push_back(flag);
                output.push_back(static_cast<Byte>((match.offset >> 8) & 0xFF));
                output.push_back(static_cast<Byte>(match.offset & 0xFF));
                
                pos += match.length;
            } else {
                // Add to literal buffer
                literal_buffer.push_back(input[pos]);
                ++pos;
            }
        }
        
        // Flush remaining literals
        flush_literals();
        
        return CompressionResult::ok(std::move(output), input.size(), output.size());
    }
    
    /**
     * @brief Decompresses dictionary-compressed data
     */
    CompressionResult decompress(const ByteBuffer& input) {
        if (input.empty()) {
            return CompressionResult::ok({}, 0, 0);
        }
        
        ByteBuffer output;
        output.reserve(input.size() * 2);
        
        Size pos = 0;
        while (pos < input.size()) {
            Byte flag = input[pos++];
            bool is_match = (flag & 0x80) != 0;
            Size count = flag & 0x7F;
            
            if (is_match) {
                // Match reference
                if (pos + 1 >= input.size()) {
                    return CompressionResult::fail("Truncated match reference");
                }
                
                Size offset = (static_cast<Size>(input[pos]) << 8) | input[pos + 1];
                pos += 2;
                
                if (offset > output.size()) {
                    return CompressionResult::fail("Invalid match offset");
                }
                
                Size match_start = output.size() - offset;
                for (Size i = 0; i < count; ++i) {
                    output.push_back(output[match_start + i]);
                }
            } else {
                // Literal bytes
                if (pos + count > input.size()) {
                    return CompressionResult::fail("Truncated literal data");
                }
                
                output.insert(output.end(), input.begin() + pos, 
                             input.begin() + pos + count);
                pos += count;
            }
        }
        
        return CompressionResult::ok(std::move(output), input.size(), output.size());
    }
};

// =============================================================================
// Record Compression (Optimized for Mainframe Records)
// =============================================================================

/**
 * @brief Compression optimized for fixed-format mainframe records
 * 
 * Takes advantage of:
 * - Fixed field positions
 * - Common padding patterns
 * - Repeated field values across records
 */
class RecordCompressor {
private:
    Size record_length_{0};
    ByteBuffer reference_record_;
    RleCompressor rle_;
    
    struct FieldDelta {
        UInt16 offset;
        UInt16 length;
        // Followed by 'length' bytes of data
    };
    
public:
    RecordCompressor() = default;
    
    /**
     * @brief Sets the fixed record length
     */
    void set_record_length(Size length) { 
        record_length_ = length; 
    }
    
    /**
     * @brief Sets a reference record for delta compression
     */
    void set_reference_record(const ByteBuffer& reference) {
        reference_record_ = reference;
    }
    
    /**
     * @brief Compresses a record using delta from reference
     * 
     * Output format:
     * - 2 bytes: number of delta fields
     * - For each delta: 2-byte offset, 2-byte length, then data
     */
    CompressionResult compress_record(const ByteBuffer& record) {
        if (record.empty()) {
            return CompressionResult::ok({}, 0, 0);
        }
        
        // If no reference, use RLE
        if (reference_record_.empty() || 
            reference_record_.size() != record.size()) {
            return rle_.compress(record);
        }
        
        ByteBuffer output;
        Vector<FieldDelta> deltas;
        
        Size i = 0;
        while (i < record.size()) {
            // Find start of difference
            while (i < record.size() && record[i] == reference_record_[i]) {
                ++i;
            }
            
            if (i >= record.size()) break;
            
            Size delta_start = i;
            
            // Find end of difference (include some context for efficiency)
            while (i < record.size() && 
                   (record[i] != reference_record_[i] || 
                    (i + 1 < record.size() && record[i + 1] != reference_record_[i + 1]))) {
                ++i;
            }
            
            Size delta_length = i - delta_start;
            
            // Record delta
            FieldDelta delta;
            delta.offset = static_cast<UInt16>(delta_start);
            delta.length = static_cast<UInt16>(delta_length);
            deltas.push_back(delta);
            
            // Store delta data
            output.insert(output.end(), 
                         record.begin() + delta_start,
                         record.begin() + delta_start + delta_length);
        }
        
        // Build final output
        ByteBuffer final_output;
        final_output.reserve(4 + deltas.size() * 4 + output.size());
        
        // Number of deltas
        UInt16 num_deltas = static_cast<UInt16>(deltas.size());
        final_output.push_back(static_cast<Byte>((num_deltas >> 8) & 0xFF));
        final_output.push_back(static_cast<Byte>(num_deltas & 0xFF));
        
        // Delta descriptors
        for (const auto& delta : deltas) {
            final_output.push_back(static_cast<Byte>((delta.offset >> 8) & 0xFF));
            final_output.push_back(static_cast<Byte>(delta.offset & 0xFF));
            final_output.push_back(static_cast<Byte>((delta.length >> 8) & 0xFF));
            final_output.push_back(static_cast<Byte>(delta.length & 0xFF));
        }
        
        // Delta data
        final_output.insert(final_output.end(), output.begin(), output.end());
        
        // Use delta compression only if it's smaller
        if (final_output.size() < record.size()) {
            return CompressionResult::ok(std::move(final_output), 
                                        record.size(), final_output.size());
        }
        
        // Fall back to RLE
        return rle_.compress(record);
    }
    
    /**
     * @brief Decompresses a delta-compressed record
     */
    CompressionResult decompress_record(const ByteBuffer& input) {
        if (input.empty()) {
            return CompressionResult::ok({}, 0, 0);
        }
        
        // Check if it's delta compression (starts with valid count)
        // or RLE (starts with escape byte or literal)
        if (input.size() < 2) {
            return rle_.decompress(input);
        }
        
        UInt16 num_deltas = (static_cast<UInt16>(input[0]) << 8) | input[1];
        
        // Sanity check
        Size expected_header_size = 2 + num_deltas * 4;
        if (num_deltas > 1000 || expected_header_size > input.size()) {
            // Probably RLE compressed
            return rle_.decompress(input);
        }
        
        // If no reference record, can't decompress delta
        if (reference_record_.empty()) {
            return rle_.decompress(input);
        }
        
        // Parse deltas
        ByteBuffer output = reference_record_;
        Size pos = 2;
        Size data_pos = expected_header_size;
        
        for (UInt16 i = 0; i < num_deltas; ++i) {
            if (pos + 3 >= input.size()) {
                return CompressionResult::fail("Truncated delta header");
            }
            
            UInt16 offset = (static_cast<UInt16>(input[pos]) << 8) | input[pos + 1];
            UInt16 length = (static_cast<UInt16>(input[pos + 2]) << 8) | input[pos + 3];
            pos += 4;
            
            if (offset + length > output.size() || data_pos + length > input.size()) {
                return CompressionResult::fail("Invalid delta bounds");
            }
            
            std::copy(input.begin() + data_pos,
                     input.begin() + data_pos + length,
                     output.begin() + offset);
            data_pos += length;
        }
        
        return CompressionResult::ok(std::move(output), input.size(), output.size());
    }
    
    /**
     * @brief Compresses multiple records together
     */
    CompressionResult compress_block(const Vector<ByteBuffer>& records) {
        if (records.empty()) {
            return CompressionResult::ok({}, 0, 0);
        }
        
        ByteBuffer output;
        Size total_original = 0;
        
        // Use first record as reference for subsequent records
        reference_record_.clear();
        
        // Record count (4 bytes)
        UInt32 count = static_cast<UInt32>(records.size());
        output.push_back(static_cast<Byte>((count >> 24) & 0xFF));
        output.push_back(static_cast<Byte>((count >> 16) & 0xFF));
        output.push_back(static_cast<Byte>((count >> 8) & 0xFF));
        output.push_back(static_cast<Byte>(count & 0xFF));
        
        for (const auto& record : records) {
            total_original += record.size();
            
            auto result = compress_record(record);
            if (!result.success) {
                return result;
            }
            
            // Record compressed size (2 bytes)
            UInt16 comp_size = static_cast<UInt16>(result.data.size());
            output.push_back(static_cast<Byte>((comp_size >> 8) & 0xFF));
            output.push_back(static_cast<Byte>(comp_size & 0xFF));
            
            // Compressed data
            output.insert(output.end(), result.data.begin(), result.data.end());
            
            // Update reference
            reference_record_ = record;
        }
        
        return CompressionResult::ok(std::move(output), total_original, output.size());
    }
    
    /**
     * @brief Decompresses a block of records
     */
    Vector<CompressionResult> decompress_block(const ByteBuffer& input) {
        Vector<CompressionResult> results;
        
        if (input.size() < 4) {
            results.push_back(CompressionResult::fail("Truncated block header"));
            return results;
        }
        
        UInt32 count = (static_cast<UInt32>(input[0]) << 24) |
                       (static_cast<UInt32>(input[1]) << 16) |
                       (static_cast<UInt32>(input[2]) << 8) |
                       static_cast<UInt32>(input[3]);
        
        reference_record_.clear();
        Size pos = 4;
        
        for (UInt32 i = 0; i < count; ++i) {
            if (pos + 1 >= input.size()) {
                results.push_back(CompressionResult::fail("Truncated record header"));
                break;
            }
            
            UInt16 comp_size = (static_cast<UInt16>(input[pos]) << 8) | input[pos + 1];
            pos += 2;
            
            if (pos + comp_size > input.size()) {
                results.push_back(CompressionResult::fail("Truncated record data"));
                break;
            }
            
            ByteBuffer record_data(input.begin() + pos, input.begin() + pos + comp_size);
            pos += comp_size;
            
            auto result = decompress_record(record_data);
            if (result.success) {
                reference_record_ = result.data;
            }
            results.push_back(std::move(result));
        }
        
        return results;
    }
};

// =============================================================================
// Compression Statistics
// =============================================================================

/**
 * @brief Statistics for compression operations
 */
struct CompressionStats {
    Size total_original_bytes{0};
    Size total_compressed_bytes{0};
    Size compression_operations{0};
    Size decompression_operations{0};
    Size compression_failures{0};
    Size decompression_failures{0};
    Duration total_compression_time{0};
    Duration total_decompression_time{0};
    
    double overall_ratio() const {
        if (total_original_bytes == 0) return 0.0;
        return 1.0 - (static_cast<double>(total_compressed_bytes) / 
                      static_cast<double>(total_original_bytes));
    }
    
    void record_compression(const CompressionResult& result, Duration time) {
        ++compression_operations;
        if (result.success) {
            total_original_bytes += result.original_size;
            total_compressed_bytes += result.compressed_size;
        } else {
            ++compression_failures;
        }
        total_compression_time += time;
    }
    
    void record_decompression(const CompressionResult& result, Duration time) {
        ++decompression_operations;
        if (!result.success) {
            ++decompression_failures;
        }
        total_decompression_time += time;
    }
    
    void reset() {
        *this = CompressionStats{};
    }
};

// =============================================================================
// Compression Factory
// =============================================================================

/**
 * @brief Factory for creating appropriate compressor based on data type
 */
enum class CompressionAlgorithm {
    None,
    RLE,
    Dictionary,
    Record
};

inline String compression_algorithm_to_string(CompressionAlgorithm type) {
    switch (type) {
        case CompressionAlgorithm::None: return "None";
        case CompressionAlgorithm::RLE: return "RLE";
        case CompressionAlgorithm::Dictionary: return "Dictionary";
        case CompressionAlgorithm::Record: return "Record";
        default: return "Unknown";
    }
}

/**
 * @brief Analyzes data to recommend best compression algorithm
 */
inline CompressionAlgorithm recommend_compression(const ByteBuffer& data) {
    if (data.empty() || data.size() < 64) {
        return CompressionAlgorithm::None;
    }
    
    // Count run lengths
    Size total_runs = 0;
    Size run_bytes = 0;
    Size i = 0;
    
    while (i < data.size()) {
        Byte current = data[i];
        Size run_length = 1;
        while (i + run_length < data.size() && data[i + run_length] == current) {
            ++run_length;
        }
        if (run_length >= 3) {
            ++total_runs;
            run_bytes += run_length;
        }
        i += run_length;
    }
    
    double run_ratio = static_cast<double>(run_bytes) / data.size();
    
    // If lots of runs, use RLE
    if (run_ratio > 0.3) {
        return CompressionAlgorithm::RLE;
    }
    
    // For larger data, try dictionary
    if (data.size() > 1024) {
        return CompressionAlgorithm::Dictionary;
    }
    
    return CompressionAlgorithm::RLE;
}

} // namespace ims::common

#endif // IMS_COMMON_COMPRESSION_HPP
