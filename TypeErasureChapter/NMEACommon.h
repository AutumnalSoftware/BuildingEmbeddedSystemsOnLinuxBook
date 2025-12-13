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
 * @brief calculateNMEAChecksum calculates checksum from after '$' (index 1)
 * to the last character. Assumes that nmeaMsg is $AABBB,f,f,f.
 *
 * @param NmeaSentence
 * @return The checksum of the NMEA sentence
 *
 * @todo Make this function more robust, could easily crash depending on
 * message and termination character.
 */
std::int8_t calculateNMEAChecksum(char *nmeaMsg, char terminationCharacter=0);

/**
 * @brief validateNMEAMessage
 * @param nmeaMsg
 * @return true if starts with "$TTMMM", has correct checksum,
 * and last two characters are "\r\n". Returns false if any are
 * incorrect.
 */
bool validateNMEAMessage(char* nmeaMsg);
