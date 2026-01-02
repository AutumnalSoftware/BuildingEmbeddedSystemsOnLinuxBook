// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Mark Wilson
//
// BinaryDataStream demo (C++17, embedded-friendly, no exceptions)
//
// - Two streams: BinaryWriteStream (MutableByteView) and BinaryReadStream (ByteView)
// - Chaining readX()/writeX() (latched error, no partial writes/reads)
// - Endianness: ctor detects host endianness, you specify wire endianness
// - Size optimization for sized fields (strings/blobs/vectors):
//     * Narrow size: 1 byte for lengths 0..254
//     * Wide size: 4 bytes total where the FIRST byte is 0xFF (part of the wide encoding),
//                  and the remaining 3 bytes store the size as a 24-bit unsigned value.
//                  This supports sizes 255..16,777,215 (0x00FF'FFFF).
//
// Checksums:
// - Rolling XOR (NMEA-style) for accidental corruption detection (not security).
// - Header checksum covers the fixed header bytes with headerChecksum treated as 0.
// - Payload checksum covers payload bytes.
//
// Notes:
// - Uses Mark's Common types: ByteView / MutableByteView (C++17).
// - This file is intentionally explicit and repetitive (good for embedded and for teaching).

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

namespace bds
{

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

//------------------------------------------------------------------------------
// Fixed-size framing header with checksums
//
// Header wire endianness is fixed by spec; payload endianness declared in header.
// Reserved fields/bits must be zero in v1 and ignored by receivers.
//------------------------------------------------------------------------------

static constexpr Endianness HeaderWireEndianness = Endianness::Little;

struct MessageHeaderV1
{
    uint8_t  version = 1;
    uint8_t  headerSize = 0;       // = sizeof(MessageHeaderV1)
    uint8_t  payloadEndian = 0;    // 0=Little, 1=Big
    uint8_t  headerFlags = 0;      // reserved bits (must be 0 in v1)

    uint16_t serviceId = 0;
    uint16_t messageType = 0;

    uint32_t payloadSize = 0;

    uint32_t flags = 0;            // reserved in v1

    uint8_t  headerChecksum = 0;   // XOR of header bytes with this treated as 0
    uint8_t  payloadChecksum = 0;  // XOR of payload bytes
    uint16_t reserved = 0;         // must be 0 in v1
};
static_assert(sizeof(MessageHeaderV1) == 20, "MessageHeaderV1 must be fixed size");
static_assert(std::is_standard_layout<MessageHeaderV1>::value,
              "MessageHeaderV1 must be standard-layout for offsetof()");

inline Endianness payloadEndianFromHeader(uint8_t payloadEndian) noexcept
{
    return (payloadEndian == 0) ? Endianness::Little : Endianness::Big;
}

inline uint8_t computeHeaderChecksum(const MessageHeaderV1& h) noexcept
{
    uint8_t tmp[sizeof(MessageHeaderV1)];
    std::memcpy(tmp, &h, sizeof(tmp));

    tmp[offsetof(MessageHeaderV1, headerChecksum)] = 0;

    return xorChecksum(tmp, sizeof(tmp));
}

inline void finalizeChecksums(MessageHeaderV1& h, ImmutableByteView payload) noexcept
{
    h.headerSize = static_cast<uint8_t>(sizeof(MessageHeaderV1));
    h.payloadChecksum = xorChecksum(payload);
    h.headerChecksum = computeHeaderChecksum(h);
}

inline BinaryWriteStream& writeHeaderV1(BinaryWriteStream& w, const MessageHeaderV1& h) noexcept
{
    w.writeUInt8(h.version)
    .writeUInt8(h.headerSize)
        .writeUInt8(h.payloadEndian)
        .writeUInt8(h.headerFlags)
        .writeUInt16(h.serviceId)
        .writeUInt16(h.messageType)
        .writeUInt32(h.payloadSize)
        .writeUInt32(h.flags)
        .writeUInt8(h.headerChecksum)
        .writeUInt8(h.payloadChecksum)
        .writeUInt16(h.reserved);

    return w;
}

inline BinaryReadStream& readHeaderV1(BinaryReadStream& r, MessageHeaderV1& h) noexcept
{
    r.readUInt8(h.version)
    .readUInt8(h.headerSize)
        .readUInt8(h.payloadEndian)
        .readUInt8(h.headerFlags)
        .readUInt16(h.serviceId)
        .readUInt16(h.messageType)
        .readUInt32(h.payloadSize)
        .readUInt32(h.flags)
        .readUInt8(h.headerChecksum)
        .readUInt8(h.payloadChecksum)
        .readUInt16(h.reserved);

    return r;
}

inline bool validateHeaderV1(const MessageHeaderV1& h) noexcept
{
    if (h.version != 1)
    {
        return false;
    }
    if (h.headerSize != sizeof(MessageHeaderV1))
    {
        return false;
    }
    if (h.headerFlags != 0 || h.flags != 0 || h.reserved != 0)
    {
        return false;
    }

    return computeHeaderChecksum(h) == h.headerChecksum;
}

inline bool validatePayloadChecksum(const MessageHeaderV1& h, ImmutableByteView payload) noexcept
{
    (void)h;
    return xorChecksum(payload) == h.payloadChecksum;
}

} // namespace bds

//------------------------------------------------------------------------------
// Demo / tests
//------------------------------------------------------------------------------

struct Sample
{
    uint32_t ms = 0;
    float tempC = 0.0f;
    double pressurePa = 0.0;
    bool ok = false;
};

static bool nearlyEqual(double a, double b, double eps = 1e-9)
{
    const double d = (a > b) ? (a - b) : (b - a);
    return d <= eps;
}

int main()
{
    using namespace bds;

    // -----------------------------
    // 1) Round-trip payload test
    // -----------------------------

    std::byte storage[256] = {};
    MutableByteView outBuf(storage, sizeof(storage));

    Sample s1;
    s1.ms = 123456u;
    s1.tempC = 21.25f;
    s1.pressurePa = 101325.125;
    s1.ok = true;

    const Endianness payloadWire = Endianness::Little;

    BinaryWriteStream w(outBuf, payloadWire);
    std::string_view name = "TMPP";

    w.writeUInt32(s1.ms)
        .writeFloat(s1.tempC)
        .writeDouble(s1.pressurePa)
        .writeBool(s1.ok)
        .writeString(name);

    assert(w.ok());

    const std::size_t used = w.bytesWritten();

    ImmutableByteView inBuf(storage, used);
    BinaryReadStream r(inBuf, payloadWire);

    Sample s2;
    std::string_view name2;

    r.readUInt32(s2.ms)
        .readFloat(s2.tempC)
        .readDouble(s2.pressurePa)
        .readBool(s2.ok)
        .readStringView(name2);

    assert(r.ok());
    assert(s2.ms == s1.ms);
    assert(s2.tempC == s1.tempC);
    assert(nearlyEqual(s2.pressurePa, s1.pressurePa));
    assert(s2.ok == s1.ok);
    assert(name2 == name);

    // -----------------------------
    // 2) Framed message with checksums + pass-through payload view
    // -----------------------------

    {
        std::byte frame[256] = {};
        MutableByteView frameBuf(frame, sizeof(frame));

        // Build payload bytes in a separate buffer first
        std::byte payloadStorage[64] = {};
        MutableByteView payloadBuf(payloadStorage, sizeof(payloadStorage));

        BinaryWriteStream pw(payloadBuf, payloadWire);
        pw.writeUInt16(42)
            .writeUInt32(777)
            .writeString("opaque");

        assert(pw.ok());

        const uint32_t payloadSize = static_cast<uint32_t>(pw.bytesWritten());

        // Frame layout: [header][payload]
        const std::size_t headerSize = sizeof(MessageHeaderV1);

        // Write a zero header (reserve space) + payload into frame
        {
            BinaryWriteStream fw(frameBuf, HeaderWireEndianness);

            MessageHeaderV1 zeroHeader;
            std::memset(&zeroHeader, 0, sizeof(zeroHeader));

            // Header bytes are treated as raw bytes in this reserve step.
            fw.writeBytes(ImmutableByteView(&zeroHeader, sizeof(zeroHeader)));
            assert(fw.ok());

            fw.writeBytes(ImmutableByteView(payloadStorage, payloadSize));
            assert(fw.ok());
        }

        // Payload view inside the frame
        ImmutableByteView framePayloadView(frame + headerSize, payloadSize);

        // Fill header + checksums
        MessageHeaderV1 h;
        h.version = 1;
        h.headerSize = static_cast<uint8_t>(sizeof(MessageHeaderV1));
        h.payloadEndian = static_cast<uint8_t>(payloadWire == Endianness::Little ? 0 : 1);
        h.headerFlags = 0;
        h.serviceId = 1;
        h.messageType = 9;
        h.payloadSize = payloadSize;
        h.flags = 0;
        h.reserved = 0;

        finalizeChecksums(h, framePayloadView);

        // Overwrite the header region
        {
            MutableByteView headerRegion = subview(frameBuf, 0, headerSize);
            BinaryWriteStream hw(headerRegion, HeaderWireEndianness);
            writeHeaderV1(hw, h);
            assert(hw.ok());
        }

        // Router reads header, validates, slices payload view, validates checksum
        ImmutableByteView frameView(frame, headerSize + payloadSize);
        BinaryReadStream fr(frameView, HeaderWireEndianness);

        MessageHeaderV1 rh;
        readHeaderV1(fr, rh);
        assert(fr.ok());
        assert(validateHeaderV1(rh));

        ImmutableByteView routedPayload;
        fr.readBytesView(rh.payloadSize, routedPayload);
        assert(fr.ok());
        assert(validatePayloadChecksum(rh, routedPayload));

        // Consumer decodes payload using payload endianness declared in header
        const Endianness consumerWire = payloadEndianFromHeader(rh.payloadEndian);
        BinaryReadStream cr(routedPayload, consumerWire);

        uint16_t a = 0;
        uint32_t b = 0;
        std::string_view sv;

        cr.readUInt16(a).readUInt32(b).readStringView(sv);
        assert(cr.ok());
        assert(a == 42);
        assert(b == 777);
        assert(sv == "opaque");

        // Corrupt one payload byte: checksum should fail
        frame[headerSize + 1] ^= std::byte{0x01};
        ImmutableByteView corruptedPayload(frame + headerSize, payloadSize);
        assert(!validatePayloadChecksum(rh, corruptedPayload));
    }

    std::cout << "All BinaryDataStream demo tests passed.\n";
    return 0;
}
