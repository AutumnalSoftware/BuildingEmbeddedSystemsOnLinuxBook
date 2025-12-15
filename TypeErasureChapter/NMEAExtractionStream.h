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

#include <vector>
#include <string>

class ByteView;
class Register32Bits;

using FieldStrings = std::vector<std::string_view>;

/**
 * @brief The NMEAExtractionStream class is used to extract field data from an NMEAMessage.
 */
class NMEAExtractionStream
{
public:
    NMEAExtractionStream() = delete;

    explicit NMEAExtractionStream(const ByteView &nmeaMessage);

    /// @todo delete copy and move

    std::string getTalker() const;

    std::string getMessage() const;

    bool numberOfFields() const;

    bool isChecksumValid() const;

    void reset();

    const NMEAExtractionStream& operator>>(int& value);

    const NMEAExtractionStream& operator>>(unsigned int& value);

    const NMEAExtractionStream& operator>>(double& value);

    const NMEAExtractionStream& operator>>(Register32Bits& value);

    const NMEAExtractionStream& operator>>(std::string& value);

private:
    const ByteView& mNMEAMessage;
    bool mChecksumValidFlag {false};

    FieldStrings mFields;

    unsigned int mChecksum {0};

    /**
     * @brief mTalker NMEA talker, which is 2 bytes. SSO will kick in, so std::string is okay here.
     */
    std::string mTalker;

    /**
     * @brief mMessage NMEA message, which is 3 bytes. SSO will kick in, so std::string is okay here.
     */
    std::string mMessage;

    uint16_t mFieldIdx{1};

    /**
     * @brief _extractPayload returns a view that includes only the actual NMEA payload fields.
     * @param nmeaMessage A complete NMEA message: $<talker><message>,<f1>,<f2>,<f3>,...,<fn>,*<checksum>
     */
    void _extractPayload(const char* nmeaMessage);
};
