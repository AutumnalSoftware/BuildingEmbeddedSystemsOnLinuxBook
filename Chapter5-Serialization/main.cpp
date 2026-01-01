// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Mark Wilson
//
// BinaryDataStream demo (C++17, embedded-friendly, no exceptions)
//
// - Two streams: BinaryWriteStream (MutableBuffer) and BinaryReadStream (ImmutableBuffer)
// - Chaining readX()/writeX() (latched error, no partial writes/reads)
// - Endianness: ctor detects host endianness, you specify wire endianness
// - Size optimization for sized fields (strings/blobs/vectors):
//     * Narrow size: 1 byte for lengths 0..254
//     * Wide size: 4 bytes total where the FIRST byte is 0xFF (part of the wide encoding),
//                  and the remaining 3 bytes store the size as a 24-bit unsigned value.
//                  This supports sizes 255..16,777,215 (0x00FF'FFFF).
//
// Notes:
// - Replace the sample MutableBuffer / ImmutableBuffer below with your real ones.
// - This file is intentionally explicit and repetitive (good for embedded and for teaching).

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>

namespace bds
{

//------------------------------------------------------------------------------
// Minimal buffer types for a self-contained demo.
// Replace these with your actual MutableBuffer / ImmutableBuffer.
//
// Requirements used by the streams:
//   - data() -> uint8_t* / const uint8_t*
//   - size() -> size_t
//   - slice(offset, len) -> same buffer type, non-owning view
//------------------------------------------------------------------------------

class ImmutableBuffer;

class MutableBuffer
{
public:
    MutableBuffer()
        : m_data(nullptr)
        , m_size(0)
    {
    }

    MutableBuffer(uint8_t* data, std::size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    uint8_t* data()
    {
        return m_data;
    }

    const uint8_t* data() const
    {
        return m_data;
    }

    std::size_t size() const
    {
        return m_size;
    }

    MutableBuffer slice(std::size_t offset, std::size_t len) const
    {
        if (offset > m_size)
        {
            return MutableBuffer();
        }
        const std::size_t avail = m_size - offset;
        const std::size_t n = (len <= avail) ? len : avail;
        return MutableBuffer(m_data + offset, n);
    }

private:
    uint8_t* m_data;
    std::size_t m_size;
};

class ImmutableBuffer
{
public:
    ImmutableBuffer()
        : m_data(nullptr)
        , m_size(0)
    {
    }

    ImmutableBuffer(const uint8_t* data, std::size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    const uint8_t* data() const
    {
        return m_data;
    }

    std::size_t size() const
    {
        return m_size;
    }

    ImmutableBuffer slice(std::size_t offset, std::size_t len) const
    {
        if (offset > m_size)
        {
            return ImmutableBuffer();
        }
        const std::size_t avail = m_size - offset;
        const std::size_t n = (len <= avail) ? len : avail;
        return ImmutableBuffer(m_data + offset, n);
    }

private:
    const uint8_t* m_data;
    std::size_t m_size;
};

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
    // If first byte is 0x04, least significant byte comes first => little endian.
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

inline void copyForward(uint8_t* dst, const uint8_t* src, std::size_t n)
{
    std::memcpy(dst, src, n);
}

inline void copyReversed(uint8_t* dst, const uint8_t* src, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i)
    {
        dst[i] = src[n - 1 - i];
    }
}

//------------------------------------------------------------------------------
// BinaryWriteStream
//------------------------------------------------------------------------------

class BinaryWriteStream
{
public:
    explicit BinaryWriteStream(MutableBuffer buffer,
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

    bool ok() const noexcept
    {
        return m_err == StreamError::None;
    }

    StreamError error() const noexcept
    {
        return m_err;
    }

    std::size_t bytesWritten() const noexcept
    {
        return m_pos;
    }

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

    BinaryWriteStream& writeUInt16(uint16_t v) noexcept
    {
        return writeScalar(v);
    }

    BinaryWriteStream& writeInt16(int16_t v) noexcept
    {
        const uint16_t u = 0;
        (void)u;
        uint16_t tmp;
        std::memcpy(&tmp, &v, sizeof(tmp));
        return writeUInt16(tmp);
    }

    BinaryWriteStream& writeUInt32(uint32_t v) noexcept
    {
        return writeScalar(v);
    }

    BinaryWriteStream& writeInt32(int32_t v) noexcept
    {
        uint32_t tmp;
        std::memcpy(&tmp, &v, sizeof(tmp));
        return writeUInt32(tmp);
    }

    BinaryWriteStream& writeUInt64(uint64_t v) noexcept
    {
        return writeScalar(v);
    }

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
        static_assert(sizeof(bits) == sizeof(v), "float must be 32-bit");
        std::memcpy(&bits, &v, sizeof(bits));
        return writeUInt32(bits);
    }

    BinaryWriteStream& writeDouble(double v) noexcept
    {
        uint64_t bits;
        static_assert(sizeof(bits) == sizeof(v), "double must be 64-bit");
        std::memcpy(&bits, &v, sizeof(bits));
        return writeUInt64(bits);
    }

    // ---- Sized field encoding (size optimization) ----
    //
    // Narrow size: 1 byte (0..254)
    // Wide size:   4 bytes total:
    //    byte0 = 0xFF
    //    byte1..3 = size24 in big-endian (most significant first)
    //
    // This keeps the "0xFF is part of the 4 bytes" property AND avoids an extra flag byte.

    BinaryWriteStream& writeSize(uint32_t n) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (n > m_maxSizedField)
        {
            m_err = StreamError::SizeLimitExceeded;
            return *this;
        }

        if (n <= 254u)
        {
            return writeUInt8(static_cast<uint8_t>(n));
        }

        // Wide size encoding supports up to 0x00FFFFFF (24-bit).
        if (n > 0x00FFFFFFu)
        {
            m_err = StreamError::SizeLimitExceeded;
            return *this;
        }

        // 4 bytes total: 0xFF + 3 bytes of size
        uint8_t tmp[4];
        tmp[0] = 0xFF;
        tmp[1] = static_cast<uint8_t>((n >> 16) & 0xFF);
        tmp[2] = static_cast<uint8_t>((n >> 8) & 0xFF);
        tmp[3] = static_cast<uint8_t>(n & 0xFF);

        return writeRawBytes(tmp, 4);
    }

    BinaryWriteStream& writeBytes(ImmutableBuffer bytes) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (!ensureCapacity(bytes.size()))
        {
            return *this;
        }

        std::memcpy(m_buf.data() + m_pos, bytes.data(), bytes.size());
        m_pos += bytes.size();
        return *this;
    }

    BinaryWriteStream& writeString(std::string_view s) noexcept
    {
        if (!ok())
        {
            return *this;
        }

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

        if (m_swap)
        {
            copyReversed(m_buf.data() + m_pos, tmp, n);
        }
        else
        {
            copyForward(m_buf.data() + m_pos, tmp, n);
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

        std::memcpy(m_buf.data() + m_pos, data, n);
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
    MutableBuffer m_buf;
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
    explicit BinaryReadStream(ImmutableBuffer buffer,
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

    bool ok() const noexcept
    {
        return m_err == StreamError::None;
    }

    StreamError error() const noexcept
    {
        return m_err;
    }

    std::size_t bytesRead() const noexcept
    {
        return m_pos;
    }

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
        out = m_buf.data()[m_pos];
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

    BinaryReadStream& readUInt16(uint16_t& out) noexcept
    {
        return readScalar(out);
    }

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

    BinaryReadStream& readUInt32(uint32_t& out) noexcept
    {
        return readScalar(out);
    }

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

    BinaryReadStream& readUInt64(uint64_t& out) noexcept
    {
        return readScalar(out);
    }

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

    // ---- Size decoding (matches writeSize) ----

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

        const uint8_t first = m_buf.data()[m_pos];

        if (first <= 254u)
        {
            // Narrow form
            out = static_cast<uint32_t>(first);
            m_pos += 1;
            return *this;
        }

        // Wide form: 4 bytes total, first byte 0xFF, next 3 bytes size24 big-endian.
        if (!ensureAvailable(4))
        {
            return *this;
        }

        const uint8_t b0 = m_buf.data()[m_pos + 0];
        const uint8_t b1 = m_buf.data()[m_pos + 1];
        const uint8_t b2 = m_buf.data()[m_pos + 2];
        const uint8_t b3 = m_buf.data()[m_pos + 3];

        if (b0 != 0xFF)
        {
            m_err = StreamError::InvalidData;
            return *this;
        }

        const uint32_t n = (static_cast<uint32_t>(b1) << 16)
                           | (static_cast<uint32_t>(b2) << 8)
                           | (static_cast<uint32_t>(b3));

        if (n <= 254u)
        {
            m_err = StreamError::InvalidData;
            return *this;
        }

        if (n > m_maxSizedField)
        {
            m_err = StreamError::SizeLimitExceeded;
            return *this;
        }

        out = n;
        m_pos += 4;
        return *this;
    }

    // Pass-through view (no decoding)
    BinaryReadStream& readBytesView(uint32_t n, ImmutableBuffer& outView) noexcept
    {
        if (!ok())
        {
            return *this;
        }

        if (!ensureAvailable(n))
        {
            return *this;
        }

        outView = m_buf.slice(m_pos, n);
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

        const char* p = reinterpret_cast<const char*>(m_buf.data() + m_pos);
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
        const uint8_t* src = m_buf.data() + m_pos;

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
    ImmutableBuffer m_buf;
    std::size_t m_pos;
    StreamError m_err;

    Endianness m_host;
    Endianness m_wire;
    bool m_swap;

    uint32_t m_maxSizedField;
};

//------------------------------------------------------------------------------
// Fixed-size header (transport framing)
//------------------------------------------------------------------------------

struct MessageHeader
{
    uint16_t serviceId = 0;
    uint16_t messageType = 0;
    uint32_t payloadSize = 0; // always 4 bytes, fixed header size
};

inline BinaryWriteStream& writeHeader(BinaryWriteStream& w, const MessageHeader& h) noexcept
{
    w.writeUInt16(h.serviceId)
    .writeUInt16(h.messageType)
        .writeUInt32(h.payloadSize);
    return w;
}

inline BinaryReadStream& readHeader(BinaryReadStream& r, MessageHeader& h) noexcept
{
    r.readUInt16(h.serviceId)
    .readUInt16(h.messageType)
        .readUInt32(h.payloadSize);
    return r;
}

} // namespace bds

//------------------------------------------------------------------------------
// Minimal NMEA-ish parsing helper for demo purposes
// (Hard-coded strings, no I/O. This is intentionally simple.)
//------------------------------------------------------------------------------

static std::vector<std::string_view> splitFields(std::string_view s)
{
    std::vector<std::string_view> out;

    // Strip checksum portion if present
    const std::size_t star = s.find('*');
    if (star != std::string_view::npos)
    {
        s = s.substr(0, star);
    }

    // Trim leading '$' if present
    if (!s.empty() && s.front() == '$')
    {
        s.remove_prefix(1);
    }

    std::size_t start = 0;
    while (start <= s.size())
    {
        const std::size_t comma = s.find(',', start);
        if (comma == std::string_view::npos)
        {
            out.emplace_back(s.substr(start));
            break;
        }
        out.emplace_back(s.substr(start, comma - start));
        start = comma + 1;
    }

    return out;
}

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
    // 1) Round-trip binary test
    // -----------------------------

    uint8_t storage[256] = {};
    MutableBuffer outBuf(storage, sizeof(storage));

    Sample s1;
    s1.ms = 123456u;
    s1.tempC = 21.25f;
    s1.pressurePa = 101325.125;
    s1.ok = true;

    const Endianness wire = Endianness::Little;

    BinaryWriteStream w(outBuf, wire);

    // Serialize sample + a short string (size <= 254 => narrow)
    std::string_view name = "TMPP";
    w.writeUInt32(s1.ms)
        .writeFloat(s1.tempC)
        .writeDouble(s1.pressurePa)
        .writeBool(s1.ok)
        .writeString(name);

    assert(w.ok());
    const std::size_t used = w.bytesWritten();

    ImmutableBuffer inBuf(storage, used);
    BinaryReadStream r(inBuf, wire);

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
    // 2) Size encoding tests
    // -----------------------------

    // (a) narrow size (10)
    {
        uint8_t b[32] = {};
        BinaryWriteStream w2(MutableBuffer(b, sizeof(b)), wire);

        std::string ten(10, 'A');
        w2.writeString(ten);
        assert(w2.ok());
        assert(b[0] == 10); // narrow size is a single byte

        BinaryReadStream r2(ImmutableBuffer(b, w2.bytesWritten()), wire);
        std::string_view sv;
        r2.readStringView(sv);
        assert(r2.ok());
        assert(sv.size() == 10);
    }

    // (b) narrow max (254)
    {
        uint8_t b[300] = {};
        BinaryWriteStream w2(MutableBuffer(b, sizeof(b)), wire);

        std::string s(254, 'B');
        w2.writeString(s);
        assert(w2.ok());
        assert(b[0] == 254);

        BinaryReadStream r2(ImmutableBuffer(b, w2.bytesWritten()), wire);
        std::string_view sv;
        r2.readStringView(sv);
        assert(r2.ok());
        assert(sv.size() == 254);
    }

    // (c) wide min (255) => 4-byte size: 0xFF + 0x00 0x00 0xFF
    {
        uint8_t b[400] = {};
        BinaryWriteStream w2(MutableBuffer(b, sizeof(b)), wire);

        std::string s(255, 'C');
        w2.writeString(s);
        assert(w2.ok());
        assert(b[0] == 0xFF);
        assert(b[1] == 0x00);
        assert(b[2] == 0x00);
        assert(b[3] == 0xFF);

        BinaryReadStream r2(ImmutableBuffer(b, w2.bytesWritten()), wire);
        std::string_view sv;
        r2.readStringView(sv);
        assert(r2.ok());
        assert(sv.size() == 255);
    }

    // -----------------------------
    // 3) Fixed header + pass-through payload view demo
    // -----------------------------

    {
        uint8_t frame[256] = {};

        // Build a payload (opaque bytes, but produced by BinaryWriteStream)
        uint8_t payloadBytes[64] = {};
        BinaryWriteStream pw(MutableBuffer(payloadBytes, sizeof(payloadBytes)), wire);

        pw.writeUInt16(42)
            .writeUInt32(777)
            .writeString("opaque");
        assert(pw.ok());

        const uint32_t payloadSize = static_cast<uint32_t>(pw.bytesWritten());

        // Frame it: [fixed header][payload]
        BinaryWriteStream fw(MutableBuffer(frame, sizeof(frame)), wire);

        MessageHeader h;
        h.serviceId = 1;
        h.messageType = 9;
        h.payloadSize = payloadSize;

        writeHeader(fw, h);
        fw.writeBytes(ImmutableBuffer(payloadBytes, payloadSize));
        assert(fw.ok());

        // Now "router" side: parse header, slice payload, forward without decoding payload
        BinaryReadStream fr(ImmutableBuffer(frame, fw.bytesWritten()), wire);

        MessageHeader hdr;
        fr.readUInt16(hdr.serviceId)
            .readUInt16(hdr.messageType)
            .readUInt32(hdr.payloadSize);

        assert(fr.ok());
        assert(hdr.payloadSize == payloadSize);

        ImmutableBuffer payloadView;
        fr.readBytesView(hdr.payloadSize, payloadView);
        assert(fr.ok());
        assert(payloadView.size() == payloadSize);

        // Consumer later decodes payloadView with its own BinaryReadStream
        BinaryReadStream cr(payloadView, wire);
        uint16_t a = 0;
        uint32_t b = 0;
        std::string_view sv;
        cr.readUInt16(a).readUInt32(b).readStringView(sv);
        assert(cr.ok());
        assert(a == 42);
        assert(b == 777);
        assert(sv == "opaque");
    }

    // -----------------------------
    // 4) Hard-coded NMEA examples (ASCII)
    // -----------------------------

    {
        std::string_view nmea1 = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
        std::string_view nmea2 = "$GPRMC,235947,A,5540.123,N,01234.567,W,000.5,054.7,191194,020.3,E*68";

        auto f1 = splitFields(nmea1);
        auto f2 = splitFields(nmea2);

        // Minimal sanity: talker+type field present and splits
        assert(!f1.empty());
        assert(!f2.empty());
        // e.g. "GPGGA", "GPRMC"
        assert(f1[0].size() >= 5);
        assert(f2[0].size() >= 5);
    }

    std::cout << "All BinaryDataStream demo tests passed.\n";
    return 0;
}
