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
#include <stdint.h>

#include "traits.h"

#include "Common/ByteView.h"

class Register32Bits;

/**
 * @brief The NMEAInsertionStream class serializes integers,floating point,
 * and string values as NMEA fields in an NMEA message string.
 */
class NMEAInsertionStream
{
public:
    /**
     * @brief The FloatFormat struct is an NMEAInsertionStream manipulator.
     */
    struct FloatFormat
    {
        std::string format = "%01f";
    };

    /**
     * @brief The Hex struct is an NMEAInsertionStream manipulator.
     */
    struct Hex {};

    /**
     * @brief The Dec struct is an NMEAInsertionStream manipulator.
     */
    struct Dec {};

    /**
     * @brief The EmptyField struct is just a tag, indicating that we should
     * stream out just a ',' character and no value;
     */
    struct EmptyField {};

    /**
     * @brief The EndMsg struct is an NMEAInsertionStream manipulator. The struct is empty
     * and used only as a tag.  It causes the NMEAInsertionStream class to calculate the
     * checksum, and ensures there are no trailing ',' characters.
     */
    struct EndMsg {};

    NMEAInsertionStream() = delete;

    /// @todo delete copy and move

    NMEAInsertionStream(MutableByteView& buffer, const char *talker, const char *msg);

    NMEAInsertionStream& operator<<(const FloatFormat& fmt);

    NMEAInsertionStream& operator<<(const Hex& hex);

    NMEAInsertionStream& operator<<(const Dec& hex);

    NMEAInsertionStream& operator<<(const EndMsg& end);

    NMEAInsertionStream& operator<<(int i);

    NMEAInsertionStream& operator<<(double d);

    NMEAInsertionStream& operator<<(const std::string &s);

    NMEAInsertionStream &operator<<(const Register32Bits &reg);

    NMEAInsertionStream &operator<<(EmptyField ef);

    /**
     * @brief Makes it easy to insert scoped enums into an NMEA stream
     * @param enumerator
     * @return
     */
    template<typename T>
    typename std::enable_if<is_scoped_enum<T>::value, NMEAInsertionStream&>::type
    operator<<(T enumerator)
    {
        std::size_t sz = sprintf(mCurrentPtr, "%d,", static_cast<T>(enumerator));
        mLen += sz;
        mCurrentPtr += sz;

        return *this;
    }

    void resetBuffer();

private:
    MutableByteView mBuffer;
    std::size_t     mCapacity{0};
    std::byte*      mBeginPtr{nullptr};    // Start of the sentence in the output buffer
    std::byte*      mCurrentPtr{nullptr};  // Current write cursor
    std::size_t     mLen{0};               // Bytes written so far

    const char*     mTalker{nullptr};
    const char*     mMsg{nullptr};

    std::string     mFloatFormat{"%0.1f"};
    std::uint8_t    mBase{10};

};
