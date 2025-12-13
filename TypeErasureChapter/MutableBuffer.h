//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef MUTABLEBUFFER_H
#define MUTABLEBUFFER_H

#include <cstddef>

#include <array>

/**
 * @brief The MutableBuffer class is a window into different types of buffers, including
 * std::array and plain C arrays. Models a contiguous data array.
 */
class MutableBuffer
{
public:
    MutableBuffer(char* buffer, std::size_t size);

    char* data() const;

    std::size_t size();

private:
    char* mBuffer;
    std::size_t mSize;
};

MutableBuffer buffer(char* buffer, std::size_t size);

template<std::size_t SZ>
MutableBuffer buffer(std::array<char, SZ>& buffer)
{
    return MutableBuffer(buffer.data(), buffer.size());
}

#endif // MUTABLEBUFFER_H
