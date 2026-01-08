#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - EBCDIC/ASCII Conversion
// Version: 3.6.2
// =============================================================================
//
// Full EBCDIC to ASCII and ASCII to EBCDIC conversion tables and utilities.
// Based on IBM Code Page 037 (US/Canada EBCDIC).
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"

namespace ims::ebcdic {

// =============================================================================
// EBCDIC to ASCII Conversion Table (Code Page 037)
// =============================================================================

// clang-format off
constexpr Byte EBCDIC_TO_ASCII[256] = {
    0x00, 0x01, 0x02, 0x03, 0x9C, 0x09, 0x86, 0x7F,  // 00-07
    0x97, 0x8D, 0x8E, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,  // 08-0F
    0x10, 0x11, 0x12, 0x13, 0x9D, 0x85, 0x08, 0x87,  // 10-17
    0x18, 0x19, 0x92, 0x8F, 0x1C, 0x1D, 0x1E, 0x1F,  // 18-1F
    0x80, 0x81, 0x82, 0x83, 0x84, 0x0A, 0x17, 0x1B,  // 20-27
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x05, 0x06, 0x07,  // 28-2F
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04,  // 30-37
    0x98, 0x99, 0x9A, 0x9B, 0x14, 0x15, 0x9E, 0x1A,  // 38-3F
    0x20, 0xA0, 0xE2, 0xE4, 0xE0, 0xE1, 0xE3, 0xE5,  // 40-47  Space, nbsp, etc.
    0xE7, 0xF1, 0xA2, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,  // 48-4F  .  <  (  +  |
    0x26, 0xE9, 0xEA, 0xEB, 0xE8, 0xED, 0xEE, 0xEF,  // 50-57  &
    0xEC, 0xDF, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,  // 58-5F  !  $  *  )  ;  ^
    0x2D, 0x2F, 0xC2, 0xC4, 0xC0, 0xC1, 0xC3, 0xC5,  // 60-67  -  /
    0xC7, 0xD1, 0xA6, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,  // 68-6F  ,  %  _  >  ?
    0xF8, 0xC9, 0xCA, 0xCB, 0xC8, 0xCD, 0xCE, 0xCF,  // 70-77
    0xCC, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,  // 78-7F  `  :  #  @  '  =  "
    0xD8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,  // 80-87  a-g
    0x68, 0x69, 0xAB, 0xBB, 0xF0, 0xFD, 0xFE, 0xB1,  // 88-8F  h-i
    0xB0, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,  // 90-97  j-p
    0x71, 0x72, 0xAA, 0xBA, 0xE6, 0xB8, 0xC6, 0xA4,  // 98-9F  q-r
    0xB5, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,  // A0-A7  ~, s-x
    0x79, 0x7A, 0xA1, 0xBF, 0xD0, 0x5B, 0xDE, 0xAE,  // A8-AF  y-z, [
    0xAC, 0xA3, 0xA5, 0xB7, 0xA9, 0xA7, 0xB6, 0xBC,  // B0-B7
    0xBD, 0xBE, 0xDD, 0xA8, 0xAF, 0x5D, 0xB4, 0xD7,  // B8-BF  ]
    0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  // C0-C7  {, A-G
    0x48, 0x49, 0xAD, 0xF4, 0xF6, 0xF2, 0xF3, 0xF5,  // C8-CF  H-I
    0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,  // D0-D7  }, J-P
    0x51, 0x52, 0xB9, 0xFB, 0xFC, 0xF9, 0xFA, 0xFF,  // D8-DF  Q-R
    0x5C, 0xF7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,  // E0-E7  \, S-X
    0x59, 0x5A, 0xB2, 0xD4, 0xD6, 0xD2, 0xD3, 0xD5,  // E8-EF  Y-Z
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  // F0-F7  0-7
    0x38, 0x39, 0xB3, 0xDB, 0xDC, 0xD9, 0xDA, 0x9F   // F8-FF  8-9
};

// =============================================================================
// ASCII to EBCDIC Conversion Table (Code Page 037)
// =============================================================================

constexpr Byte ASCII_TO_EBCDIC[256] = {
    0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F,  // 00-07
    0x16, 0x05, 0x25, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,  // 08-0F
    0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26,  // 10-17
    0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,  // 18-1F
    0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D,  // 20-27  Space ! " # $ % & '
    0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,  // 28-2F  ( ) * + , - . /
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,  // 30-37  0-7
    0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,  // 38-3F  8-9 : ; < = > ?
    0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,  // 40-47  @ A-G
    0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,  // 48-4F  H-O
    0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,  // 50-57  P-W
    0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,  // 58-5F  X-Z [ \ ] ^ _
    0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,  // 60-67  ` a-g
    0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,  // 68-6F  h-o
    0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,  // 70-77  p-w
    0xA7, 0xA8, 0xA9, 0xC0, 0x4F, 0xD0, 0xA1, 0x07,  // 78-7F  x-z { | } ~
    0x20, 0x21, 0x22, 0x23, 0x24, 0x15, 0x06, 0x17,  // 80-87
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x09, 0x0A, 0x1B,  // 88-8F
    0x30, 0x31, 0x1A, 0x33, 0x34, 0x35, 0x36, 0x08,  // 90-97
    0x38, 0x39, 0x3A, 0x3B, 0x04, 0x14, 0x3E, 0xFF,  // 98-9F
    0x41, 0xAA, 0x4A, 0xB1, 0x9F, 0xB2, 0x6A, 0xB5,  // A0-A7
    0xBB, 0xB4, 0x9A, 0x8A, 0xB0, 0xCA, 0xAF, 0xBC,  // A8-AF
    0x90, 0x8F, 0xEA, 0xFA, 0xBE, 0xA0, 0xB6, 0xB3,  // B0-B7
    0x9D, 0xDA, 0x9B, 0x8B, 0xB7, 0xB8, 0xB9, 0xAB,  // B8-BF
    0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9E, 0x68,  // C0-C7
    0x74, 0x71, 0x72, 0x73, 0x78, 0x75, 0x76, 0x77,  // C8-CF
    0xAC, 0x69, 0xED, 0xEE, 0xEB, 0xEF, 0xEC, 0xBF,  // D0-D7
    0x80, 0xFD, 0xFE, 0xFB, 0xFC, 0xBA, 0xAE, 0x59,  // D8-DF
    0x44, 0x45, 0x42, 0x46, 0x43, 0x47, 0x9C, 0x48,  // E0-E7
    0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57,  // E8-EF
    0x8C, 0x49, 0xCD, 0xCE, 0xCB, 0xCF, 0xCC, 0xE1,  // F0-F7
    0x70, 0xDD, 0xDE, 0xDB, 0xDC, 0x8D, 0x8E, 0xDF   // F8-FF
};
// clang-format on

// =============================================================================
// Conversion Functions
// =============================================================================

/// Convert a single EBCDIC byte to ASCII
constexpr Byte ebcdic_to_ascii(Byte ebcdic) {
    return EBCDIC_TO_ASCII[ebcdic];
}

/// Convert a single ASCII byte to EBCDIC
constexpr Byte ascii_to_ebcdic(Byte ascii) {
    return ASCII_TO_EBCDIC[ascii];
}

/// Convert EBCDIC buffer to ASCII string
inline String ebcdic_to_string(const ByteBuffer& ebcdic) {
    String result;
    result.reserve(ebcdic.size());
    for (Byte b : ebcdic) {
        result += static_cast<char>(EBCDIC_TO_ASCII[b]);
    }
    return result;
}

/// Convert ASCII string to EBCDIC buffer
inline ByteBuffer string_to_ebcdic(StringView ascii) {
    ByteBuffer result;
    result.reserve(ascii.size());
    for (char c : ascii) {
        result.push_back(ASCII_TO_EBCDIC[static_cast<Byte>(c)]);
    }
    return result;
}

/// Convert EBCDIC buffer to ASCII in-place
inline void ebcdic_to_ascii_inplace(ByteBuffer& buffer) {
    for (Byte& b : buffer) {
        b = EBCDIC_TO_ASCII[b];
    }
}

/// Convert ASCII buffer to EBCDIC in-place
inline void ascii_to_ebcdic_inplace(ByteBuffer& buffer) {
    for (Byte& b : buffer) {
        b = ASCII_TO_EBCDIC[b];
    }
}

/// Convert EBCDIC buffer to ASCII buffer
inline ByteBuffer ebcdic_to_ascii_buffer(const ByteBuffer& ebcdic) {
    ByteBuffer result;
    result.reserve(ebcdic.size());
    for (Byte b : ebcdic) {
        result.push_back(EBCDIC_TO_ASCII[b]);
    }
    return result;
}

/// Convert ASCII buffer to EBCDIC buffer
inline ByteBuffer ascii_to_ebcdic_buffer(const ByteBuffer& ascii) {
    ByteBuffer result;
    result.reserve(ascii.size());
    for (Byte b : ascii) {
        result.push_back(ASCII_TO_EBCDIC[b]);
    }
    return result;
}

// =============================================================================
// EBCDIC Character Classification
// =============================================================================

/// Check if EBCDIC byte is alphabetic (A-Z, a-z)
constexpr bool is_ebcdic_alpha(Byte b) {
    // Uppercase A-I: C1-C9, J-R: D1-D9, S-Z: E2-E9
    // Lowercase a-i: 81-89, j-r: 91-99, s-z: A2-A9
    return (b >= 0xC1 && b <= 0xC9) ||  // A-I
           (b >= 0xD1 && b <= 0xD9) ||  // J-R
           (b >= 0xE2 && b <= 0xE9) ||  // S-Z
           (b >= 0x81 && b <= 0x89) ||  // a-i
           (b >= 0x91 && b <= 0x99) ||  // j-r
           (b >= 0xA2 && b <= 0xA9);    // s-z
}

/// Check if EBCDIC byte is numeric (0-9)
constexpr bool is_ebcdic_digit(Byte b) {
    return b >= 0xF0 && b <= 0xF9;
}

/// Check if EBCDIC byte is alphanumeric
constexpr bool is_ebcdic_alnum(Byte b) {
    return is_ebcdic_alpha(b) || is_ebcdic_digit(b);
}

/// Check if EBCDIC byte is whitespace
constexpr bool is_ebcdic_space(Byte b) {
    return b == 0x40 ||  // Space
           b == 0x05 ||  // HT
           b == 0x25 ||  // LF
           b == 0x0D;    // CR
}

/// Convert EBCDIC byte to uppercase
constexpr Byte ebcdic_toupper(Byte b) {
    // Lowercase a-i (81-89) -> A-I (C1-C9)
    // Lowercase j-r (91-99) -> J-R (D1-D9)
    // Lowercase s-z (A2-A9) -> S-Z (E2-E9)
    if (b >= 0x81 && b <= 0x89) return static_cast<Byte>(b + 0x40);
    if (b >= 0x91 && b <= 0x99) return static_cast<Byte>(b + 0x40);
    if (b >= 0xA2 && b <= 0xA9) return static_cast<Byte>(b + 0x40);
    return b;
}

/// Convert EBCDIC byte to lowercase
constexpr Byte ebcdic_tolower(Byte b) {
    // Uppercase A-I (C1-C9) -> a-i (81-89)
    // Uppercase J-R (D1-D9) -> j-r (91-99)
    // Uppercase S-Z (E2-E9) -> s-z (A2-A9)
    if (b >= 0xC1 && b <= 0xC9) return static_cast<Byte>(b - 0x40);
    if (b >= 0xD1 && b <= 0xD9) return static_cast<Byte>(b - 0x40);
    if (b >= 0xE2 && b <= 0xE9) return static_cast<Byte>(b - 0x40);
    return b;
}

// =============================================================================
// EBCDIC Special Characters
// =============================================================================

namespace chars {
    constexpr Byte SPACE      = 0x40;
    constexpr Byte PERIOD     = 0x4B;
    constexpr Byte COMMA      = 0x6B;
    constexpr Byte COLON      = 0x7A;
    constexpr Byte SEMICOLON  = 0x5E;
    constexpr Byte PLUS       = 0x4E;
    constexpr Byte MINUS      = 0x60;
    constexpr Byte ASTERISK   = 0x5C;
    constexpr Byte SLASH      = 0x61;
    constexpr Byte EQUAL      = 0x7E;
    constexpr Byte LPAREN     = 0x4D;
    constexpr Byte RPAREN     = 0x5D;
    constexpr Byte LBRACKET   = 0xAD;
    constexpr Byte RBRACKET   = 0xBD;
    constexpr Byte LBRACE     = 0xC0;
    constexpr Byte RBRACE     = 0xD0;
    constexpr Byte AMPERSAND  = 0x50;
    constexpr Byte DOLLAR     = 0x5B;
    constexpr Byte AT_SIGN    = 0x7C;
    constexpr Byte HASH       = 0x7B;
    constexpr Byte NEWLINE    = 0x25;
    constexpr Byte CARRIAGE   = 0x0D;
    constexpr Byte TAB        = 0x05;
} // namespace chars

// =============================================================================
// Packed Decimal Support (COMP-3)
// =============================================================================

/// Convert packed decimal to Int64
inline Int64 packed_to_int(const ByteBuffer& packed) {
    if (packed.empty()) return 0;
    
    Int64 result = 0;
    bool negative = false;
    
    for (Size i = 0; i < packed.size(); ++i) {
        Byte high = (packed[i] >> 4) & 0x0F;
        Byte low = packed[i] & 0x0F;
        
        result = result * 10 + high;
        
        if (i == packed.size() - 1) {
            // Last byte: low nibble is sign
            // 0x0C = positive, 0x0D = negative, 0x0F = unsigned
            negative = (low == 0x0D || low == 0x0B);
        } else {
            result = result * 10 + low;
        }
    }
    
    return negative ? -result : result;
}

/// Convert Int64 to packed decimal
inline ByteBuffer int_to_packed(Int64 value, Size bytes) {
    bool negative = value < 0;
    UInt64 abs_value = negative ? static_cast<UInt64>(-value) : static_cast<UInt64>(value);
    
    ByteBuffer result(bytes, 0);
    
    // Fill from right to left
    Size digit_count = (bytes * 2) - 1;
    Vector<Byte> digits(digit_count, 0);
    
    for (Size i = 0; i < digit_count && abs_value > 0; ++i) {
        digits[digit_count - 1 - i] = static_cast<Byte>(abs_value % 10);
        abs_value /= 10;
    }
    
    Size digit_pos = 0;
    for (Size i = 0; i < bytes; ++i) {
        Byte high = (digit_pos < digit_count) ? digits[digit_pos++] : 0;
        Byte low;
        
        if (i == bytes - 1) {
            // Last byte: low nibble is sign
            low = negative ? 0x0D : 0x0C;
        } else {
            low = (digit_pos < digit_count) ? digits[digit_pos++] : 0;
        }
        
        result[i] = static_cast<Byte>((high << 4) | low);
    }
    
    return result;
}

} // namespace ims::ebcdic
