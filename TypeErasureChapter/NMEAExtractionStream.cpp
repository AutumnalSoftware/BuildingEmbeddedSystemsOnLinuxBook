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
#include <charconv>
#include <stdexcept>
#include <cmath>
#include <climits>
#include <cstdlib>
#include <string>

#include "Common/ByteView.h"

#include "NMEACommon.h"
#include "NMEAExtractionStream.h"
#include "Register32Bits.h"

using namespace std;



// Forward declarations
FieldStrings parseMessage(std::string_view message);
std::string_view skip_leading_whitespace(std::string_view strv);


NMEAExtractionStream::NMEAExtractionStream(const ByteView &nmeaMessage) :
    mNMEAMessage(nmeaMessage)
{
    //mFields = parseMessage(nmeaMessage.data());

    mFields = parseMessage(std::string_view{
        toCharPtr(nmeaMessage.data()),
        nmeaMessage.size()
    });


    //for (auto field : mFields)
        //cout << "** field = " << field << endl;

    if ( mFields.size() > 0 )
    {
        mTalker = mFields[0].substr(0, 2);
        mMessage = mFields[0].substr(2, 3);
    }
    else
    {
        mTalker = "XX";
        mMessage = "YYY";
    }
}

std::string NMEAExtractionStream::getTalker() const
{
    return mTalker;
}

std::string NMEAExtractionStream::getMessage() const
{
    return mMessage;
}

bool NMEAExtractionStream::isChecksumValid() const
{
    return false;
}

bool NMEAExtractionStream::numberOfFields() const
{
    return mFields.size();
}

void NMEAExtractionStream::reset()
{
    mFieldIdx = 1;
}

const NMEAExtractionStream &NMEAExtractionStream::operator>>(int &value)
{
    auto nows = skip_leading_whitespace(mFields[mFieldIdx]);
    const char *start = nows.data();
    std::size_t sz = nows.size();

    int i;

    char *endptr;
    i = strtol(start, &endptr, 10);
    value = i;

#if 0
    auto result = std::from_chars(start, start + sz, i);
    if (result.ec == std::errc::invalid_argument)
    {
        value = std::numeric_limits<int>::min();
    }
    else
    {
        value = i;
    }
#endif

    mFieldIdx++;

    return *this;
}

const NMEAExtractionStream &NMEAExtractionStream::operator>>(unsigned int &value)
{
    auto nows = skip_leading_whitespace(mFields[mFieldIdx]);
    const char *start = nows.data();
    //std::size_t sz = nows.size();

    int i;

    char *endptr;
    i = strtol(start, &endptr, 10);
    value = i;

    mFieldIdx++;

    return *this;
}

const NMEAExtractionStream &NMEAExtractionStream::operator>>(double &value)
{
    auto nows = skip_leading_whitespace(mFields[mFieldIdx]);
    const char *start = nows.data();
    std::size_t sz = nows.size();

    if (sz == 0) {
        value = std::nan("");
        mFieldIdx++;
        return *this;
    }

    double d;
    char *end;

    d = std::strtod(start, &end);
    value = d;

#if 0
    auto result = std::from_chars(start, start + sz, d);
    if (result.ec == std::errc::invalid_argument) {
        value = std::nan("");
    }
    else
        value = d;

#endif

    mFieldIdx++;

    return *this;
}

const NMEAExtractionStream &NMEAExtractionStream::operator>>(Register32Bits &value)
{
    auto nows = skip_leading_whitespace(mFields[mFieldIdx]);
    const char *start = nows.data();
    std::size_t sz = nows.size();

    int i;
    //auto result = std::from_chars(start, start + sz, i, 16);
    char *endptr;
    i = strtol(start, &endptr, 16);

    //if (result.ec == std::errc::invalid_argument)
    //{
        // value will be empty, do 'if (value)' to tell if it is good.
        //value = Register32Bits();
    //}
    //else
    //{
    //}

    value = Register32Bits(i);

    mFieldIdx++;

    return *this;
}

const NMEAExtractionStream &NMEAExtractionStream::operator>>(std::string &value)
{
    std::string_view f = mFields[mFieldIdx];

    std::copy(f.begin(), f.end(), std::back_inserter(value));

    mFieldIdx++;

    return *this;
}

void NMEAExtractionStream::_extractPayload(const char *nmeaMessage)
{

}

// Function to split string based on a delimiter into a vector of string_views
FieldStrings splitString(std::string_view str, char delim)
{
    std::vector<std::string_view> result;
    size_t start = 0;
    size_t end = str.find(delim);

    while (end != std::string_view::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delim, start);
    }

    result.push_back(str.substr(start));

    return result;
}

// Function to parse the message and return a vector of fields
FieldStrings parseMessage(std::string_view message)
{
    //cout << "[parseMessage] message = '" << message << "'" << endl;
    // Check for the starting '$' and the '*' before the checksum

    if (message.front() != '$' || message.find('*') == std::string_view::npos) {
        //throw std::invalid_argument("Invalid message format");
        std::cerr << "MISSED A MESSAGE DUE TO INVALID FORMAT" << std::endl;
        return FieldStrings {};
        
    }

    // Find position of '*' which starts the checksum part
    size_t checksumStart = message.rfind('*');
    if (checksumStart == std::string_view::npos || checksumStart + 3 != message.size()) {
        std::cerr << "MISSED A MESSAGE DUE TO INVALID CHECKSUM FORMAT" << std::endl;
        //throw std::invalid_argument("Invalid checksum format");
        return FieldStrings {};
    }

    // Extract the checksum part (excluding '*')
    std::string_view checksum = message.substr(checksumStart + 1, 2);

    // Extract the main content between '$' and '*'
    std::string_view content = message.substr(1, checksumStart - 1);

    // Split the content into fields based on ','
    return splitString(content, ',');
}


std::string_view skip_leading_whitespace(std::string_view strv) {
    size_t pos = 0;
    while (pos < strv.length() && std::isspace(strv[pos])) {
        pos++;
    }
    strv.remove_prefix(pos);
    return strv;
}
