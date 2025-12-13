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
#include <iostream>
#include <stdio.h>

#include "NMEAInsertionStream.h"
#include "NMEACommon.h"
#include "Register32Bits.h"
#include "MutableBuffer.h"

using namespace std;

NMEAInsertionStream::NMEAInsertionStream(MutableBuffer &buffer, const char *talker, const char *msg) :
    mBuffer(buffer),
    mBufferSize(buffer.size()),
    mCurrentPtr(buffer.data()),
    mTalker(talker),
    mMsg(msg)
{
    *mCurrentPtr++ = '$';
    strcpy((char*)mCurrentPtr, talker);
    mCurrentPtr += strlen(talker);
    strcpy((char*)mCurrentPtr, msg);
    mCurrentPtr += strlen(msg);
    *mCurrentPtr++ = ',';
}

void NMEAInsertionStream::resetBuffer()
{
    mCurrentPtr = mBuffer.data();
}

using namespace std;

NMEAInsertionStream &NMEAInsertionStream::operator<<(int i)
{
    if ( mBase == 10 )
    {
        std::size_t sz = sprintf((char*)mCurrentPtr, "%d,", i);
        mLen += sz;
        mCurrentPtr += sz;
    }

    if ( mBase == 16 )
    {
        std::size_t sz = sprintf((char*)mCurrentPtr, "0x%04X,", i);
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

NMEAInsertionStream &NMEAInsertionStream::operator<<(const std::string &s)
{
    std::size_t sz = s.size();
    std::copy(s.begin(), s.end(), mCurrentPtr);
    mCurrentPtr += sz;
    *mCurrentPtr++ = ',';
    mLen += sz+1;

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
    *mCurrentPtr = ',';
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

NMEAInsertionStream& NMEAInsertionStream::operator<<(const EndMsg& end)
{
    mCurrentPtr--;
    *mCurrentPtr = 0;

    std::int8_t cs = calculateNMEAChecksum(mBuffer.data());

    sprintf(mCurrentPtr, "*%xc", cs);
    mCurrentPtr += 3;

    strcpy((char*)mCurrentPtr, "\r\n");
    mCurrentPtr += 2;

    printf("s: %s\n", mBuffer.data());

    return *this;
}
