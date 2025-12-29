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

#include "Common/ByteView.h"

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

    std::size_t numberOfFields() const;

    bool isChecksumValid() const;

    void reset();

    bool hasError() const noexcept { return mErrorFlag; }

    NMEAExtractionStream& operator>>(int& value);

    NMEAExtractionStream& operator>>(unsigned int& value);

    NMEAExtractionStream& operator>>(double& value);

    NMEAExtractionStream& operator>>(Register32Bits& value);

    NMEAExtractionStream& operator>>(std::string& value);

    std::string_view nextField() noexcept;

private:
    const ByteView mNMEAMessage;
    bool mChecksumValidFlag {false};

    FieldStrings mFields;

    unsigned int mChecksum {0};

    bool mErrorFlag {false};

    /**
     * @brief mTalker NMEA talker, which is 2 bytes. SSO will kick in, so std::string is okay here.
     */
    std::string mTalker;

    /**
     * @brief mMessage NMEA message, which is 3 bytes. SSO will kick in, so std::string is okay here.
     */
    std::string mMessage;

    uint16_t mFieldIdx{1};
};
