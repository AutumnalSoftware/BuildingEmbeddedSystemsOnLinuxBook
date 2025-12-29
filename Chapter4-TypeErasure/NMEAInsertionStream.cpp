// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <cstring>
#include <stdio.h>

#include "Common/ByteView.h"
#include "NMEAInsertionStream.h"
#include "NMEACommon.h"
#include "Register32Bits.h"

using namespace std;

#include <cstdio>
#include <cstring>

static constexpr std::byte ByteComma = static_cast<std::byte>(',');
static constexpr std::byte ByteZero  = static_cast<std::byte>(0);

NMEAInsertionStream::NMEAInsertionStream(MutableByteView& buffer,
                                         const char* talker,
                                         const char* msg)
    : mBuffer(buffer)
    , mCapacity(buffer.size())
    , mBeginPtr(buffer.data())
    , mCurrentPtr(buffer.data())
    , mLen(0)
    , mTalker(talker)
    , mMsg(msg)
{
    // Build: $ + talker + msg + ,
    // Need: 1 + strlen(talker) + strlen(msg) + 1
    const std::size_t tLen = std::strlen(talker);
    const std::size_t mLenLocal = std::strlen(msg);
    const std::size_t required = 1 + tLen + mLenLocal + 1;

    if (mCapacity < required)
    {
        return; // or set an error flag
    }

    *mCurrentPtr++ = toByte('$');
    mLen += 1;

    std::memcpy(reinterpret_cast<char*>(mCurrentPtr), talker, tLen);
    mCurrentPtr += tLen;
    mLen += tLen;

    std::memcpy(reinterpret_cast<char*>(mCurrentPtr), msg, mLenLocal);
    mCurrentPtr += mLenLocal;
    mLen += mLenLocal;

    *mCurrentPtr++ = ByteComma;
    mLen += 1;
}

void NMEAInsertionStream::resetBuffer()
{
    mCurrentPtr = mBuffer.data();
}

using namespace std;

NMEAInsertionStream &NMEAInsertionStream::operator<<(int i)
{
    const std::size_t remaining = mCapacity - mLen;   // you should already track this
    int written = 0;

    if (mBase == 10)
    {
        written = std::snprintf(
            reinterpret_cast<char*>(mCurrentPtr),
            remaining,
            "%d,", i);
    }
    else if (mBase == 16)
    {
        written = std::snprintf(
            reinterpret_cast<char*>(mCurrentPtr),
            remaining,
            "0x%04X,", i);
    }

    if (written > 0)
    {
        const std::size_t sz = static_cast<std::size_t>(written);
        mLen += sz;
        mCurrentPtr += sz;
    }

    return *this;
}

NMEAInsertionStream &NMEAInsertionStream::operator<<(double d)
{
    std::size_t sz = sprintf((char*)mCurrentPtr, "%f,", d);
    mLen += sz;
    mCurrentPtr += sz;

    return *this;
}

NMEAInsertionStream& NMEAInsertionStream::operator<<(const std::string& s)
{
    const std::size_t sz = s.size();

    // Need space for the string plus trailing comma
    if ((mCapacity - mLen) < (sz + 1))
    {
        return *this; // or set an error flag
    }

    // Convert chars -> bytes explicitly
    for (char c : s)
    {
        *mCurrentPtr++ = toByte(c);
    }

    *mCurrentPtr++ = ByteComma;

    mLen += (sz + 1);
    return *this;
}

NMEAInsertionStream &NMEAInsertionStream::operator<<(const Register32Bits &reg)
{
    int rv = reg.toUInt();
    *this << rv;

    return *this;
}


NMEAInsertionStream& NMEAInsertionStream::operator<<(EmptyField ef)
{
    *mCurrentPtr = toByte(',');
    mLen += 1;
    mCurrentPtr++;

   return *this;
}

NMEAInsertionStream& NMEAInsertionStream::operator<<(const FloatFormat& fmt)
{
    mFloatFormat = fmt.format;

    return *this;
}

NMEAInsertionStream &NMEAInsertionStream::operator<<(const Hex &hex)
{
    mBase = 16;

    return *this;
}

NMEAInsertionStream &NMEAInsertionStream::operator<<(const Dec &hex)
{
    mBase = 10;

    return *this;
}

NMEAInsertionStream& NMEAInsertionStream::operator<<(const EndMsg&)
{
    // Nothing written yet
    if (mLen == 0)
    {
        return *this;
    }

    // Remove trailing comma if present
    constexpr std::byte ByteComma = static_cast<std::byte>(',');
    constexpr std::byte ByteZero  = static_cast<std::byte>(0);

    if (*(mCurrentPtr - 1) == ByteComma)
    {
        --mCurrentPtr;
        --mLen;
    }

    // We will append: "*HH\r\n" plus a '\0' for debug printing
    // snprintf writes 5 chars ("*HH\r\n") + null terminator = 6 bytes
    constexpr std::size_t Required = 6;

    if ((mCapacity - mLen) < Required)
    {
        return *this;   // or set error state
    }

    // Temporary null-termination (not part of NMEA, just for safety/debug)
    *mCurrentPtr = ByteZero;

    // Compute checksum over bytes written so far
    const std::uint8_t checksum =
        calculateNMEAChecksum(mBeginPtr, mLen);

    const std::size_t remaining = mCapacity - mLen;

    const int written = std::snprintf(
        reinterpret_cast<char*>(mCurrentPtr),
        remaining,
        "*%02X\r\n",
        static_cast<unsigned>(checksum));

    if (written <= 0)
    {
        return *this;
    }

    const std::size_t sz = static_cast<std::size_t>(written);

    // snprintf returns number of chars excluding '\0'
    if (sz >= remaining)
    {
        // Truncated; clamp cursor to end
        mCurrentPtr = mBeginPtr + mCapacity;
        mLen        = mCapacity;
        return *this;
    }

    mCurrentPtr += sz;
    mLen        += sz;

    // Final null terminator for debug printing convenience
    if (mLen < mCapacity)
    {
        *mCurrentPtr = ByteZero;
    }

    // Debug output (optional)
    std::printf("s: %s\n",
                reinterpret_cast<const char*>(mBeginPtr));

    return *this;
}
