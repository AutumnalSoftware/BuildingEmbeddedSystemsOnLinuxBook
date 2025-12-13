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

#include <cstddef>

class ImmutableBuffer
{
public:
    ImmutableBuffer() = delete;

    /// @todo delete copy and move

    ImmutableBuffer(const char* buffer, std::size_t size);

    const char *data() const;

    std::size_t size() const;

private:
    const char* mBuffer;
    std::size_t mSize;
};
