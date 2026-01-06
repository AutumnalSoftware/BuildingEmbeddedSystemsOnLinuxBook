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
//#include "MutableByteView.h"

#include "BdsCommon.h"

//------------------------------------------------------------------------------
// BinaryReadStream
//------------------------------------------------------------------------------

class BinaryReadStream
{
public:
    explicit BinaryReadStream(ImmutableByteView buffer,
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

    std::size_t bytesRead() const noexcept { return m_pos; }

    std::size_t remaining() const noexcept
    {
        return (m_pos <= m_buf.size()) ? (m_buf.size() - m_pos) : 0;
    }

    // ---- Primitive reads (chaining) ----

    BinaryReadStream& readUInt8(uint8_t& out) noexcept
    {
        if (!ok())
        {
            return *this;
        }
        if (!ensureAvailable(1))
        {
            return *this;
        }

        out = u8(m_buf)[m_pos];
        m_pos += 1;
        return *this;
    }

    BinaryReadStream& readInt8(int8_t& out) noexcept
    {
        uint8_t tmp = 0;
        readUInt8(tmp);
        if (ok())
        {
            out = static_cast<int8_t>(tmp);
        }
        return *this;
    }

    BinaryReadStream& readUInt16(uint16_t& out) noexcept { return readScalar(out); }

    BinaryReadStream& readInt16(int16_t& out) noexcept
    {
        uint16_t tmp = 0;
        readUInt16(tmp);
        if (ok())
        {
            std::memcpy(&out, &tmp, sizeof(out));
        }
        return *this;
    }

    BinaryReadStream& readUInt32(uint32_t& out) noexcept { return readScalar(out); }

    BinaryReadStream& readInt32(int32_t& out) noexcept
    {
        uint32_t tmp = 0;
        readUInt32(tmp);
        if (ok())
        {
            std::memcpy(&out, &tmp, sizeof(out));
        }
        return *this;
    }

    BinaryReadStream& readUInt64(uint64_t& out) noexcept { return readScalar(out); }

    BinaryReadStream& readInt64(int64_t& out) noexcept
    {
        uint64_t tmp = 0;
        readUInt64(tmp);
        if (ok())
        {
            std::memcpy(&out, &tmp, sizeof(out));
        }
        return *this;
    }

    BinaryReadStream& readBool(bool& out) noexcept
    {
        uint8_t b = 0;
        readUInt8(b);
        if (ok())
        {
            out = (b != 0u);
        }
        return *this;
    }

    BinaryReadStream& readFloat(float& out) noexcept
    {
        uint32_t bits = 0;
        readUInt32(bits);
        if (ok())
        {
            std::memcpy(&out, &bits, sizeof(out));
        }
        return *this;
    }

    BinaryReadStream& readDouble(double& out) noexcept
    {
        uint64_t bits = 0;
        readUInt64(bits);
        if (ok())
        {
            std::memcpy(&out, &bits, sizeof(out));
        }
        return *this;
    }

    // ---- Size decoding ----

    BinaryReadStream& readSize(uint32_t& out) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (!ensureAvailable(1))
        {
            return *this;
        }

        const uint8_t first = u8(m_buf)[m_pos];

        if (first <= 254u)
        {
            out = static_cast<uint32_t>(first);
            m_pos += 1;
            return *this;
        }

        if (!ensureAvailable(4))
        {
            return *this;
        }

        const uint8_t b0 = u8(m_buf)[m_pos + 0];
        const uint8_t b1 = u8(m_buf)[m_pos + 1];
        const uint8_t b2 = u8(m_buf)[m_pos + 2];
        const uint8_t b3 = u8(m_buf)[m_pos + 3];

        if (b0 != 0xFF)
        {
            m_err = StreamError::InvalidData;
            return *this;
        }

        const uint32_t n = (static_cast<uint32_t>(b1) << 16)
                           | (static_cast<uint32_t>(b2) << 8)
                           | (static_cast<uint32_t>(b3));

        if (n <= 254u || n > m_maxSizedField)
        {
            m_err = StreamError::InvalidData;
            return *this;
        }

        out = n;
        m_pos += 4;
        return *this;
    }

    // Pass-through view (no decoding)
    BinaryReadStream& readBytesView(uint32_t n, ImmutableByteView& outView) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (!ensureAvailable(n))
        {
            return *this;
        }

        outView = subview(m_buf, m_pos, n);
        m_pos += n;
        return *this;
    }

    BinaryReadStream& readStringView(std::string_view& out) noexcept
    {
        uint32_t n = 0;
        readSize(n);
        if (!ok())
        {
            return *this;
        }

        if (!ensureAvailable(n))
        {
            return *this;
        }

        const char* p = reinterpret_cast<const char*>(u8(m_buf) + m_pos);
        out = std::string_view(p, n);
        m_pos += n;
        return *this;
    }

private:
    template <typename T>
    BinaryReadStream& readScalar(T& out) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        const std::size_t n = sizeof(T);
        if (!ensureAvailable(n))
        {
            return *this;
        }

        uint8_t tmp[sizeof(T)];
        const uint8_t* src = u8(m_buf) + m_pos;

        if (m_swap)
        {
            copyReversed(tmp, src, n);
        }
        else
        {
            copyForward(tmp, src, n);
        }

        std::memcpy(&out, tmp, n);
        m_pos += n;
        return *this;
    }

    bool ensureAvailable(std::size_t n) noexcept
    {
        if (remaining() < n)
        {
            m_err = StreamError::BufferUnderflow;
            return false;
        }
        return true;
    }

private:
    ImmutableByteView m_buf;
    std::size_t m_pos;
    StreamError m_err;

    Endianness m_host;
    Endianness m_wire;
    bool m_swap;

    uint32_t m_maxSizedField;
};
