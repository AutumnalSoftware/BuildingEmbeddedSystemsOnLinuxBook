// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#pragma once

#include <string>
#include <ostream>
#include <cstdint>
#include <cstddef>

class NMEAExtractionStream;

enum class messageResult_t : std::uint8_t {
    NACK = 0,
    ACK = 1,
};

enum class memoryClass_t : std::uint8_t
{
    VOLATILE = 1,
    NONVOLATILE = 2
};

NMEAExtractionStream &operator>>(NMEAExtractionStream &stream, messageResult_t &v);

NMEAExtractionStream &operator>>(NMEAExtractionStream &stream, memoryClass_t &v);

std::ostream &operator<<(std::ostream &os, messageResult_t v);

std::ostream &operator<<(std::ostream &os, memoryClass_t v);

std::string toString(messageResult_t mr);

std::string toString(memoryClass_t mc);

/**
 * @brief Calculate the NMEA 0183 checksum (XOR) for a sentence under construction.
 *
 * Computes the XOR of all bytes after the leading '$' up to (but not including)
 * the '*' checksum delimiter if present, otherwise up to @p length.
 *
 * @param data   Pointer to the beginning of the sentence buffer (expected to start with '$').
 * @param length Number of valid bytes currently in the buffer.
 * @return 8-bit XOR checksum value.
 *
 * @note This function is length-based and does not require null termination.
 */
std::uint8_t calculateNMEAChecksum(const std::byte* data, std::size_t length) noexcept;


/**
 * @brief validateNMEAMessage
 * @param nmeaMsg
 * @return true if starts with "$TTMMM", has correct checksum,
 * and last two characters are "\r\n". Returns false if any are
 * incorrect.
 */
bool validateNMEAMessage(char* nmeaMsg);

inline const char* toCharPtr(const std::byte* p) noexcept
{
    return reinterpret_cast<const char*>(p);
}

inline char* toCharPtr(std::byte* p) noexcept
{
    return reinterpret_cast<char*>(p);
}

inline std::byte toByte(char c) noexcept
{
    return static_cast<std::byte>(c);
}

inline bool isAscii(std::byte b, char c) noexcept
{
    return b == static_cast<std::byte>(c);
}
