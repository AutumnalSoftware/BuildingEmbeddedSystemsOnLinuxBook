// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Mark Wilson

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

// For offsetof
#include <cstddef>

#include "ImmutableByteView.h"
#include "MutableByteView.h"

//------------------------------------------------------------------------------
// Helpers for working with std::byte buffers
//------------------------------------------------------------------------------

inline uint8_t* u8(MutableByteView b) noexcept
{
    return reinterpret_cast<uint8_t*>(b.data());
}

inline const uint8_t* u8(ImmutableByteView b) noexcept
{
    return reinterpret_cast<const uint8_t*>(b.data());
}

inline MutableByteView subview(MutableByteView b, std::size_t offset, std::size_t len) noexcept
{
    if (offset > b.size())
    {
        return MutableByteView();
    }
    const std::size_t avail = b.size() - offset;
    const std::size_t n = (len <= avail) ? len : avail;
    return MutableByteView(b.data() + offset, n);
}

inline ImmutableByteView subview(ImmutableByteView b, std::size_t offset, std::size_t len) noexcept
{
    if (offset > b.size())
    {
        return ImmutableByteView();
    }
    const std::size_t avail = b.size() - offset;
    const std::size_t n = (len <= avail) ? len : avail;
    return ImmutableByteView(b.data() + offset, n);
}

//------------------------------------------------------------------------------
// Endianness
//------------------------------------------------------------------------------

enum class Endianness : uint8_t
{
    Little = 0,
    Big = 1
};

inline Endianness detectHostEndianness() noexcept
{
    const uint32_t x = 0x01020304u;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&x);
    return (p[0] == 0x04) ? Endianness::Little : Endianness::Big;
}

//------------------------------------------------------------------------------
// Stream error (embedded-friendly, latched)
//------------------------------------------------------------------------------

enum class StreamError : uint8_t
{
    None = 0,
    BufferOverflow,
    BufferUnderflow,
    SizeLimitExceeded,
    InvalidData
};

//------------------------------------------------------------------------------
// Utility: safe copy with optional byte reversal
//------------------------------------------------------------------------------

inline void copyForward(uint8_t* dst, const uint8_t* src, std::size_t n) noexcept
{
    std::memcpy(dst, src, n);
}

inline void copyReversed(uint8_t* dst, const uint8_t* src, std::size_t n) noexcept
{
    for (std::size_t i = 0; i < n; ++i)
    {
        dst[i] = src[n - 1 - i];
    }
}

//------------------------------------------------------------------------------
// Rolling XOR checksum (NMEA-style). Corruption detection, not security.
//------------------------------------------------------------------------------

inline uint8_t xorChecksum(const uint8_t* data, std::size_t n) noexcept
{
    uint8_t c = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        c ^= data[i];
    }
    return c;
}

inline uint8_t xorChecksum(ImmutableByteView b) noexcept
{
    return xorChecksum(u8(b), b.size());
}
