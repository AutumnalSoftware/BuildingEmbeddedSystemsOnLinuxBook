// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <iostream>
#include <string>

#include "AnyNMEAMessage.h"

#include "NMEAInsertionStream.h"
#include "NMEAExtractionStream.h"

#include "MutableBuffer.h"
#include "ImmutableBuffer.h"

using namespace std;

//
// Strawmen NMEA messages to keep things simple.
//
struct GGAMessage
{
    int i{42};
    double d{123.456};
    std::string s{"STRING"};
};

struct RMCMessage
{
    double d{456.789};
    int i{105};
};

// Pretty-print helpers (demo-friendly)
ostream &operator<<(ostream &str, const GGAMessage &msg)
{
    str << "GGA: i = " << msg.i << ", d = " << msg.d << ", s = " << msg.s;
    return str;
}

ostream &operator<<(ostream &str, const RMCMessage &msg)
{
    str << "RMC: d = " << msg.d << ", i = " << msg.i;
    return str;
}

// NMEA traits for AnyNMEAMessage
template<>
struct NMEATraits<GGAMessage>
{
    static std::string messageName() { return "GGA"; }
};

template<>
struct NMEATraits<RMCMessage>
{
    static std::string messageName() { return "RMC"; }
};

// -----------------------------------------------------------------------------
// IMPORTANT:
// Your linker error was looking for:
//   operator>><GGAMessage>(NMEAExtractionStream&, GGAMessage&)
// which means there is a *templated* operator>> declared somewhere.
// These explicit specializations provide the missing definitions.
// If the template operators are in a namespace, put these specializations there.
// -----------------------------------------------------------------------------

template<>
NMEAExtractionStream &operator>><GGAMessage>(NMEAExtractionStream &stream, GGAMessage &msg)
{
    stream >> msg.i;
    stream >> msg.d;
    stream >> msg.s;
    return stream;
}

template<>
NMEAExtractionStream &operator>><RMCMessage>(NMEAExtractionStream &stream, RMCMessage &msg)
{
    stream >> msg.i;
    stream >> msg.d;
    return stream;
}

// Optional symmetry: explicit insertion specializations too (harmless if your code uses them)
template<>
NMEAInsertionStream &operator<<<GGAMessage>(NMEAInsertionStream &stream, const GGAMessage &msg)
{
    stream << msg.i;
    stream << msg.d;
    stream << msg.s;
    stream << NMEAInsertionStream::EndMsg();
    return stream;
}

template<>
NMEAInsertionStream &operator<<<RMCMessage>(NMEAInsertionStream &stream, const RMCMessage &msg)
{
    stream << msg.i;
    stream << msg.d;
    stream << NMEAInsertionStream::EndMsg();
    return stream;
}

static void demoSerialization()
{
    GGAMessage gga1{1, 43.34, "HELLO"};
    AnyNMEAMessage m1("MW", gga1);

    char buffer[1024]{};
    MutableBuffer mb(buffer, sizeof(buffer));

    // Note: talker/message here are for the output sentence framing,
    // while AnyNMEAMessage carries its own talker/message metadata too.
    NMEAInsertionStream nis(mb, "GT", "GGA");
    m1.serialize(nis);

    cout << "Serialized GGA message is " << buffer << endl;
}

int main()
{
    demoSerialization();
    return 0;
}

