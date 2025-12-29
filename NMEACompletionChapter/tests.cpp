// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "AnyNMEAMessage.h"
#include "NMEAInsertionStream.h"
#include "NMEAExtractionStream.h"
#include "Common/ByteView.h"

using namespace std;

// Strawmen messages (same as your main.cpp)
struct GGAMessage
{
    int i{42};
    double d{123.456};
    std::string s{"STRING"};
};

ostream &operator<<(ostream &str, const GGAMessage &msg)
{
    str << "GGA: i = " << msg.i << ", d = " << msg.d << ", s = " << msg.s;
    return str;
}

NMEAInsertionStream &operator<<(NMEAInsertionStream &stream, const GGAMessage &msg)
{
    stream << msg.i;
    stream << msg.d;
    stream << msg.s;
    stream << NMEAInsertionStream::EndMsg();
    return stream;
}

NMEAExtractionStream &operator>>(NMEAExtractionStream &stream, GGAMessage &msg)
{
    stream >> msg.i;
    stream >> msg.d;
    stream >> msg.s;
    return stream;
}

struct RMCMessage
{
    double d{456.789};
    int i{105};
};

ostream &operator<<(ostream &str, const RMCMessage &msg)
{
    str << "RMC: d = " << msg.d << ", i = " << msg.i;
    return str;
}

NMEAInsertionStream &operator<<(NMEAInsertionStream &stream, const RMCMessage &msg)
{
    stream << msg.i;
    stream << msg.d;
    stream << NMEAInsertionStream::EndMsg();
    return stream;
}

NMEAExtractionStream &operator>>(NMEAExtractionStream &stream, RMCMessage &msg)
{
    stream >> msg.i;
    stream >> msg.d;
    return stream;
}


/// Requires NMEATraits<T> specialization; intended as a convenience.
template <>
struct NMEATraits<GGAMessage>
{
    static std::string_view messageName()
    {
        return "GGA";
    }
};

/// Requires NMEATraits<T> specialization; intended as a convenience.
template <>
struct NMEATraits<RMCMessage>
{
    static std::string_view messageName()
    {
        return "RMC";
    }
};

static void testQueryAndAccessors()
{
    GGAMessage gga1{1, 43.34, "HELLO"};
    RMCMessage rmc1{};

    AnyNMEAMessage m1("MW", gga1);
    AnyNMEAMessage m2("MW", "RMC", rmc1);

    assert(!m1.empty());
    assert(static_cast<bool>(m1));

    assert(m1.isType<GGAMessage>());
    assert(m1.getMessageName() == "GGA");
    assert(m1.getTalker() == "MW");

    assert(m2.isType<RMCMessage>());
    assert(m2.getMessageName() == "RMC");

    assert(!m1.isType<RMCMessage>());

    // Empty message should be false
    AnyNMEAMessage empty;
    assert(empty.empty());
    assert(!static_cast<bool>(empty));
}

static void testCopy()
{
    GGAMessage gga1{1, 43.34, "HELLO"};
    AnyNMEAMessage m1("MW", gga1);

    AnyNMEAMessage m2 = m1;

    auto v1 = m1.get<GGAMessage>();
    auto v2 = m2.get<GGAMessage>();

    assert(v1.i == v2.i);
    assert(v1.d == v2.d);
    assert(v1.s == v2.s);
}

static void testSerialization()
{
    GGAMessage gga1{1, 43.34, "HELLO"};

    // Use explicit talker + message name to avoid requiring NMEATraits<GGAMessage>.
    AnyNMEAMessage m1("GT", "GGA", gga1);

    char buffer[1024]{};
    MutableByteView mb(buffer, sizeof(buffer));

    // Stream owns framing ($ + talker + msg, commas, checksum, etc.)
    NMEAInsertionStream nis(mb, "GT", "GGA");

    // Payload only
    m1.serializePayload(nis);

    // Finalize framing/checksum and remove trailing commas
    nis << NMEAInsertionStream::EndMsg();

    // Basic sanity: something was written.
    assert(std::strlen(buffer) > 0);

    // Optional: check that talker/message appear in the sentence.
    assert(std::strstr(buffer, "GGA") != nullptr);
    assert(std::strstr(buffer, "GT") != nullptr);
}

int main()
{
    testQueryAndAccessors();
    testCopy();
    testSerialization();

    std::cout << "All tests passed.\n";
    return 0;
}

