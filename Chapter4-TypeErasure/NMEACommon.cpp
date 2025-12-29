// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <string.h>

#include "NMEACommon.h"
#include "NMEAExtractionStream.h"

#include <cstddef>
#include <cstdint>
#include <cstddef>

std::uint8_t calculateNMEAChecksum(const std::byte* data,
                                   std::size_t length) noexcept
{
    std::uint8_t checksum = 0;

    // NMEA: XOR of bytes between '$' and '*', excluding both
    // We assume:
    //  - data[0] == '$'
    //  - length is the number of valid bytes written so far
    for (std::size_t i = 1; i < length; ++i)
    {
        const std::byte b = data[i];

        if (b == static_cast<std::byte>('*'))
        {
            break;
        }

        checksum ^= static_cast<std::uint8_t>(b);
    }

    return checksum;
}


#if 0
int8_t calculateNMEAChecksum(char* nmeaMsg, char terminationCharacter)
{
    unsigned char checkSum = 0;

    auto l = strlen(nmeaMsg);

    for (std::size_t idx = 1; nmeaMsg[idx]!=terminationCharacter; idx++) // Index 1 is '$' and doesn't count in checksum
    {
        checkSum ^= nmeaMsg[idx];
    }

    return checkSum;
}
#endif

bool validateNMEAMessage(char *nmeaMsg)
{
    std::size_t l = strlen(nmeaMsg);

    if ( nmeaMsg[0] != '$' )
        return false;

    return true;
}

NMEAExtractionStream &operator>>(NMEAExtractionStream &stream, messageResult_t &v)
{
    int i;
    stream >> i;

    switch (i) {
    case 0:
        v = messageResult_t::NACK;
        break;
    case 1:
        v = messageResult_t::ACK;
        break;
    }

    return stream;
}

NMEAExtractionStream &operator>>(NMEAExtractionStream &stream, memoryClass_t &v)
{
    int i;
    stream >> i;

    switch (i) {
    case 1:
        v = memoryClass_t::VOLATILE;
        break;
    case 2:
        v = memoryClass_t::NONVOLATILE;
        break;
    }

    return stream;
}

std::ostream &operator<<(std::ostream &os, messageResult_t v)
{
    switch (v)
    {
    case messageResult_t::NACK:
        os << "NACK";
        break;
    case messageResult_t::ACK:
        os << "ACK";
        break;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, memoryClass_t v)
{
    switch (v) {
    case memoryClass_t::VOLATILE:
        os << "VOLATILE";
        break;
    case memoryClass_t::NONVOLATILE:
        os << "NONVOLATILE";
        break;
    }

    return os;
}


std::string toString(messageResult_t mr)
{
    switch (mr) {
    case messageResult_t::NACK:
        return "NACK";
        break;
    case messageResult_t::ACK:
        return "ACK";
        break;
    default:
        return "UNKNOWN_MSG_RESULT";
    }
}

std::string toString(memoryClass_t mc)
{
    switch (mc) {
    case memoryClass_t::VOLATILE:
        return "VOLATILE";
        break;
    case memoryClass_t::NONVOLATILE:
        return "NONVOLATILE";
        break;
    default:
        return "UNKNOWN";
    }
}
