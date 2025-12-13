//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include "ImmutableBuffer.h"

ImmutableBuffer::ImmutableBuffer(const char * buffer, std::size_t size) :
    mBuffer(buffer),
    mSize(size)
{

}

const char *ImmutableBuffer::data() const
{
    return mBuffer;
}

std::size_t ImmutableBuffer::size() const
{
    return mSize;
}
