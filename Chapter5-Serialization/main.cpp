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

#include "MutableByteView.h"

#include "BinaryReadStream.h"
#include "BinaryWriteStream.h"
#include "MessageHeader.h"

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
