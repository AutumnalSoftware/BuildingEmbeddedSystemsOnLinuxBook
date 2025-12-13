// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
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


void testQueryAndAccessors()
{
    cout << "TEST QUERY AND ACCESSORS" << endl;
    cout << "===================================" << endl;

    GGAMessage gga1{1, 43.34, "HELLO"};
    cout << gga1 << endl;
    RMCMessage rmc1{};
    cout << rmc1 << endl;

    AnyNMEAMessage m1("MW", gga1);
    AnyNMEAMessage m2("MW", "RMC", rmc1);

    if (!m1.isEmpty()) {
        cout << "m1 contains something" << endl;
    }

    if (!m1) {
        cout << "m1 contains something" << endl;
    }

    cout << endl;

    if (m1.isType<GGAMessage>()) {
        cout << "m1 is a GGAMessage" << endl;
        cout << "Message is " << m1.getMessageName() << endl;
    }

    if (m1) {
        auto v1 = m1.get<GGAMessage>();
    }


    if (m2.isType<RMCMessage>()) {
        cout << "m2 is a RMCMessage" << endl;
        cout << "Message is " << m2.getMessageName() << endl;
    }

    if (!m1.isType<RMCMessage>()) {
        cout << "m1 is NOT an RMCMessage" << endl;
    }

    cout << "m1 talker is " << m1.getTalker() << endl;
    cout << "m1 message is " << m1.getMessageName() << endl;
    cout << "m1 checksum is " << static_cast<int>(m1.getChecksum()) << endl;
    cout << "m1 size is " << m1.getSize() << endl;

    AnyNMEAMessage empty;
    if (empty.isEmpty()) {
        cout << "The empty message is empty" << endl;
    }
    if (empty) {
        cout << "The empty message is empty" << endl;
    }
}

void testCopy()
{
    cout << "TEST ASSIGNMENT" << endl;
    cout << "===================================" << endl;

    GGAMessage gga1{1, 43.34, "HELLO"};
    AnyNMEAMessage m1("MW", gga1);

    AnyNMEAMessage m2 = m1;

    cout << "[m1] " << m1.get<GGAMessage>() << endl;
    cout << "[m2] " << m2.get<GGAMessage>() << endl;

}

void testSerialization()
{
    GGAMessage gga1{1, 43.34, "HELLO"};
    AnyNMEAMessage m1("MW", gga1);

    char buffer[1024];
    MutableBuffer mb(buffer, 1024);

    NMEAInsertionStream nis(mb, "GT", "GGA");
    m1.serialize(nis);

    cout << "Serialized GGA message is " << buffer << endl;
}

int main()
{
    testQueryAndAccessors();
    testCopy();
    testSerialization();

    return 0;
}
