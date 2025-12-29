// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>

#include "AnyNMEAMessage.h"
#include "Common/ByteView.h"
#include "NMEAExtractionStream.h"
#include "NMEAInsertionStream.h"

//-----------------------------------------------------------------------------
// Strawman messages
//-----------------------------------------------------------------------------

struct GGAMessage
{
    int i{42};
    double d{123.456};
    std::string s{"STRING"};
};

struct RMCMessage
{
    int a{7};
    double b{3.14};
    std::string c{"RMC"};
};

std::ostream& operator<<(std::ostream& os, const GGAMessage& msg)
{
    os << "GGAMessage{ i=" << msg.i << ", d=" << msg.d << ", s=\"" << msg.s << "\" }";
    return os;
}

std::ostream& operator<<(std::ostream& os, const RMCMessage& msg)
{
    os << "RMCMessage{ a=" << msg.a << ", b=" << msg.b << ", c=\"" << msg.c << "\" }";
    return os;
}

//-----------------------------------------------------------------------------
// NMEA stream operators (PAYLOAD ONLY)
// Note: NO operator<<<T> templates. NO EndMsg here.
//-----------------------------------------------------------------------------

NMEAInsertionStream& operator<<(NMEAInsertionStream& stream, const GGAMessage& msg)
{
    stream << msg.i;
    stream << msg.d;
    stream << msg.s;
    return stream;
}

NMEAExtractionStream& operator>>(NMEAExtractionStream& stream, GGAMessage& msg)
{
    stream >> msg.i;
    stream >> msg.d;
    stream >> msg.s;
    return stream;
}

NMEAInsertionStream& operator<<(NMEAInsertionStream& stream, const RMCMessage& msg)
{
    stream << msg.a;
    stream << msg.b;
    stream << msg.c;
    return stream;
}

NMEAExtractionStream& operator>>(NMEAExtractionStream& stream, RMCMessage& msg)
{
    stream >> msg.a;
    stream >> msg.b;
    stream >> msg.c;
    return stream;
}

//-----------------------------------------------------------------------------
// Demo helpers
//-----------------------------------------------------------------------------

static void demoQueryAndAccessors()
{
    std::cout << "\n--- demoQueryAndAccessors ---\n";

    GGAMessage gga;
    AnyNMEAMessage m1("GP", "GGA", gga);

    std::cout << "talker=" << m1.getTalker()
              << " name=" << m1.getMessageName()
              << " type=" << m1.type().name()
              << "\n";

    if (m1.isType<GGAMessage>())
    {
        const GGAMessage& ref = m1.get<GGAMessage>();
        std::cout << "payload: " << ref << "\n";
    }

    AnyNMEAMessage empty;
    std::cout << "empty? " << (empty.empty() ? "yes" : "no") << "\n";
}

static void demoSerialization()
{
    std::cout << "\n--- demoSerialization ---\n";

    GGAMessage gga;
    AnyNMEAMessage m1("GP", "GGA", gga);

    // Backing store for the serialized sentence.
    std::array<std::byte, 256> backing{};
    MutableByteView out = asWritableBytes(backing);

    // IMPORTANT: Your inserter needs talker + msg at construction time.
    // It will write "$" + talker + msg + "," ... etc as part of framing.
    // Our AnyNMEAMessage writes payload only.
    NMEAInsertionStream nis(out, "GP", "GGA");

    // Payload only:
    m1.serializePayload(nis);

    // End message / checksum is also framing policy:
    nis << NMEAInsertionStream::EndMsg();

    // I can’t print the sentence without a "bytes written" / "view" API
    // from NMEAInsertionStream. Once you show that, we’ll add:
    //   ByteView sentence = nis.view();
    //   std::cout << std::string_view((const char*)sentence.data(), sentence.size()) << "\n";
    std::cout << "(serialized GGAMessage payload and wrote EndMsg)\n";
}

static void demoDeserialization()
{
    std::cout << "\n--- demoDeserialization ---\n";

    // Use a literal sentence. NOTE: checksum is placeholder; if extractor enforces it,
    // use a real checksum’d sentence from your inserter once we can print it.
    const char* sentence = "$GPGGA,42,123.456,STRING*00\r\n";
    const std::size_t len = std::strlen(sentence);

    ByteView in = asBytes(sentence, len);
    NMEAExtractionStream ex(in);

    // Since we already know header is GGA, we instantiate with GGAMessage payload.
    AnyNMEAMessage m("GP", "GGA", GGAMessage{});
    m.deserializePayload(ex);

    std::cout << "talker=" << ex.getTalker()
              << " msg=" << ex.getMessage()
              << " fields=" << ex.numberOfFields()
              << " checksumValid=" << (ex.isChecksumValid() ? "yes" : "no")
              << "\n";

    std::cout << "decoded: " << m.get<GGAMessage>() << "\n";
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------

int main()
{
    demoQueryAndAccessors();
    demoSerialization();
    demoDeserialization();
    return 0;
}
