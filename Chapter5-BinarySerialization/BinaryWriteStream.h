// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Mark Wilson
//

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <cassert>
#include <type_traits>

// For offsetof
#include <cstddef>

#include "ImmutableByteView.h"
#include "MutableByteView.h"

#include "BdsCommon.h"

namespace weather
{

//------------------------------------------------------------------------------
// BinaryWriteStream
//------------------------------------------------------------------------------

class BinaryWriteStream
{
public:
    explicit BinaryWriteStream(MutableByteView buffer,
                               Endianness wireEndianness = Endianness::Little,
                               uint32_t maxSizedField = 0x00FFFFFFu) noexcept
        : m_buf(buffer)
        , m_pos(0)
        , m_err(StreamError::None)
        , m_host(detectHostEndianness())
        , m_wire(wireEndianness)
        , m_swap(m_host != m_wire)
        , m_maxSizedField(maxSizedField)
    {
    }

    bool ok() const noexcept { return m_err == StreamError::None; }
    StreamError error() const noexcept { return m_err; }

    std::size_t bytesWritten() const noexcept { return m_pos; }

    std::size_t remaining() const noexcept
    {
        return (m_pos <= m_buf.size()) ? (m_buf.size() - m_pos) : 0;
    }

    // ---- Primitive writes (chaining) ----

    BinaryWriteStream& writeUInt8(uint8_t v) noexcept
    {
        return writeRawBytes(&v, 1);
    }

    BinaryWriteStream& writeInt8(int8_t v) noexcept
    {
        return writeUInt8(static_cast<uint8_t>(v));
    }

    BinaryWriteStream& writeUInt16(uint16_t v) noexcept { return writeScalar(v); }

    BinaryWriteStream& writeInt16(int16_t v) noexcept
    {
        uint16_t tmp;
        std::memcpy(&tmp, &v, sizeof(tmp));
        return writeUInt16(tmp);
    }

    BinaryWriteStream& writeUInt32(uint32_t v) noexcept { return writeScalar(v); }

    BinaryWriteStream& writeInt32(int32_t v) noexcept
    {
        uint32_t tmp;
        std::memcpy(&tmp, &v, sizeof(tmp));
        return writeUInt32(tmp);
    }

    BinaryWriteStream& writeUInt64(uint64_t v) noexcept { return writeScalar(v); }

    BinaryWriteStream& writeInt64(int64_t v) noexcept
    {
        uint64_t tmp;
        std::memcpy(&tmp, &v, sizeof(tmp));
        return writeUInt64(tmp);
    }

    BinaryWriteStream& writeBool(bool v) noexcept
    {
        const uint8_t b = v ? 1u : 0u;
        return writeUInt8(b);
    }

    BinaryWriteStream& writeFloat(float v) noexcept
    {
        uint32_t bits;
        static_assert(sizeof(bits) == sizeof(v), "float must be 32-bit IEEE-754");
        std::memcpy(&bits, &v, sizeof(bits));
        return writeUInt32(bits);
    }

    BinaryWriteStream& writeDouble(double v) noexcept
    {
        uint64_t bits;
        static_assert(sizeof(bits) == sizeof(v), "double must be 64-bit IEEE-754");
        std::memcpy(&bits, &v, sizeof(bits));
        return writeUInt64(bits);
    }

    // ---- Sized field encoding (size optimization) ----
    //
    // Narrow: 1 byte (0..254)
    // Wide:   4 bytes total: 0xFF + 24-bit size in big-endian
    //
    // Note: 0xFF is part of the 4 bytes, not an extra flag byte.

    BinaryWriteStream& writeSize(uint32_t n) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (n > m_maxSizedField || n > 0x00FFFFFFu)
        {
            m_err = StreamError::SizeLimitExceeded;
            return *this;
        }

        if (n <= 254u)
        {
            return writeUInt8(static_cast<uint8_t>(n));
        }

        uint8_t tmp[4];
        tmp[0] = 0xFF;
        tmp[1] = static_cast<uint8_t>((n >> 16) & 0xFF);
        tmp[2] = static_cast<uint8_t>((n >> 8) & 0xFF);
        tmp[3] = static_cast<uint8_t>(n & 0xFF);

        return writeRawBytes(tmp, 4);
    }

    BinaryWriteStream& writeBytes(ImmutableByteView bytes) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (!ensureCapacity(bytes.size()))
        {
            return *this;
        }

        std::memcpy(u8(m_buf) + m_pos, bytes.data(), bytes.size());
        m_pos += bytes.size();
        return *this;
    }

    BinaryWriteStream& writeString(std::string_view s) noexcept
    {
        writeSize(static_cast<uint32_t>(s.size()));
        if (!ok())
        {
            return *this;
        }

        return writeRawBytes(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }

private:
    template <typename T>
    BinaryWriteStream& writeScalar(const T& value) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        const std::size_t n = sizeof(T);
        if (!ensureCapacity(n))
        {
            return *this;
        }

        uint8_t tmp[sizeof(T)];
        std::memcpy(tmp, &value, n);

        uint8_t* dst = u8(m_buf) + m_pos;
        if (m_swap)
        {
            copyReversed(dst, tmp, n);
        }
        else
        {
            copyForward(dst, tmp, n);
        }

        m_pos += n;
        return *this;
    }

    BinaryWriteStream& writeRawBytes(const void* data, std::size_t n) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (!ensureCapacity(n))
        {
            return *this;
        }

        std::memcpy(u8(m_buf) + m_pos, data, n);
        m_pos += n;
        return *this;
    }

    bool ensureCapacity(std::size_t n) noexcept
    {
        if (remaining() < n)
        {
            m_err = StreamError::BufferOverflow;
            return false;
        }
        return true;
    }

private:
    MutableByteView m_buf;
    std::size_t m_pos;
    StreamError m_err;

    Endianness m_host;
    Endianness m_wire;
    bool m_swap;

    uint32_t m_maxSizedField;
};

} // end of namespace weather
