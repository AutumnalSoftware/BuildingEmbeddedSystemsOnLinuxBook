// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <cmath>
#include <climits>
#include <cstdlib>
#include <string>
#include <charconv>

#include "Common/ByteView.h"

#include "NMEACommon.h"
#include "NMEAExtractionStream.h"
#include "Register32Bits.h"

using namespace std;

static bool parseHex2(std::string_view sv, std::uint8_t& out) noexcept;
static std::string_view trimTrailing(std::string_view sv) noexcept;

// Forward declarations
FieldStrings parseMessage(std::string_view message);
std::string_view skipLeadingWhitespace(std::string_view strv);


static inline bool isHexDigit(char c) noexcept
{
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

std::string_view NMEAExtractionStream::nextField() noexcept
{
    if (mFieldIdx >= mFields.size())
    {
        mErrorFlag = true;
        return {};
    }

    return mFields[mFieldIdx++];
}


NMEAExtractionStream::NMEAExtractionStream(const ByteView& nmeaMessage)
    : mNMEAMessage(nmeaMessage)
{
    // Build a view over the bytes (text layered on bytes)
    std::string_view msg{ toCharPtr(nmeaMessage.data()), nmeaMessage.size() };
    msg = trimTrailing(msg);

    // Validate checksum if present: "...*HH"
    mChecksumValidFlag = false;
    mChecksum = 0;

    const std::size_t star = msg.rfind('*');
    if (star != std::string_view::npos && star + 3 <= msg.size())
    {
        std::uint8_t parsed = 0;
        if (parseHex2(msg.substr(star + 1, 2), parsed))
        {
            // Compute over bytes up to '*' (NMEA XOR excludes '$' and '*')
            const std::uint8_t computed =
                calculateNMEAChecksum(mNMEAMessage.data(), star + 1);

            mChecksum = parsed;
            mChecksumValidFlag = (computed == parsed);
        }
    }

    // Parse fields (your parseMessage should ignore checksum portion)
    mFields = parseMessage(msg);

    if (!mFields.empty() && mFields[0].size() >= 5)
    {
        mTalker  = mFields[0].substr(0, 2);
        mMessage = mFields[0].substr(2, 3);
    }
    else
    {
        mTalker  = "XX";
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

std::size_t NMEAExtractionStream::numberOfFields() const
{
    return mFields.size();
}

void NMEAExtractionStream::reset()
{
    mFieldIdx = 1;
}

NMEAExtractionStream& NMEAExtractionStream::operator>>(int& value)
{
    const std::string_view f = nextField();
    if (f.empty())
    {
        mErrorFlag = true;
        value = 0;
        return *this;
    }

    int v = 0;
    const char* begin = f.data();
    const char* end   = f.data() + f.size();

    const auto res = std::from_chars(begin, end, v, 10);
    if (res.ec != std::errc{} || res.ptr != end)
    {
        mErrorFlag = true;
        value = 0;
        return *this;
    }

    value = v;
    return *this;
}

NMEAExtractionStream& NMEAExtractionStream::operator>>(double& value)
{
    const std::string_view f = nextField();
    if (f.empty())
    {
        mErrorFlag = true;
        value = 0.0;
        return *this;
    }

    std::string tmp(f);          // ensures null-terminated
    char* endPtr = nullptr;
    errno = 0;

    const double v = std::strtod(tmp.c_str(), &endPtr);

    if (errno != 0 || endPtr == tmp.c_str() || *endPtr != '\0')
    {
        mErrorFlag = true;
        value = 0.0;
        return *this;
    }

    value = v;
    return *this;
}

NMEAExtractionStream& NMEAExtractionStream::operator>>(Register32Bits& value)
{
    const std::string_view f = nextField();
    if (f.empty())
    {
        mErrorFlag = true;
        value = Register32Bits{0};
        return *this;
    }

    std::string_view hex = f;
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
    {
        hex.remove_prefix(2);
    }

    if (hex.empty())
    {
        mErrorFlag = true;
        value = Register32Bits{0};
        return *this;
    }

    for (char c : hex)
    {
        if (!isHexDigit(c))
        {
            mErrorFlag = true;
            value = Register32Bits{0};
            return *this;
        }
    }

    std::uint32_t v = 0;
    const auto res = std::from_chars(hex.data(), hex.data() + hex.size(), v, 16);
    if (res.ec != std::errc{} || res.ptr != hex.data() + hex.size())
    {
        mErrorFlag = true;
        value = Register32Bits{0};
        return *this;
    }

    value = Register32Bits{v};
    return *this;
}

NMEAExtractionStream& NMEAExtractionStream::operator>>(std::string& value)
{
    const std::string_view f = nextField();
    value.assign(f.begin(), f.end());
    return *this;
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

static std::string_view trimTrailing(std::string_view sv) noexcept
{
    while (!sv.empty())
    {
        const unsigned char c = static_cast<unsigned char>(sv.back());
        if (c == '\0' || c == '\r' || c == '\n' || std::isspace(c))
        {
            sv.remove_suffix(1);
            continue;
        }
        break;
    }
    return sv;
}

std::vector<std::string_view> parseMessage(std::string_view message)
{
    std::vector<std::string_view> fields;

    message = trimTrailing(message);

    // Must start with '$'
    if (message.empty() || message.front() != '$')
    {
        // You can set an error flag here if you have one
        return fields;
    }

    // Ignore checksum portion (everything from '*' onward)
    const std::size_t starPos = message.find('*');
    if (starPos != std::string_view::npos)
    {
        message = message.substr(0, starPos);
        message = trimTrailing(message);
    }

    // Drop the leading '$' for tokenizing (fields[0] becomes TALKER_MSG)
    std::string_view body = message.substr(1);

    // Split by commas into string_view fields
    std::size_t start = 0;
    while (start <= body.size())
    {
        const std::size_t commaPos = body.find(',', start);

        if (commaPos == std::string_view::npos)
        {
            // Last field (or only field)
            fields.emplace_back(body.substr(start));
            break;
        }

        fields.emplace_back(body.substr(start, commaPos - start));
        start = commaPos + 1;

        // Handle trailing comma (produces an empty final field)
        if (start == body.size())
        {
            fields.emplace_back(std::string_view{});
            break;
        }
    }

    return fields;
}


std::string_view skipLeadingWhitespace(std::string_view strv) {
    size_t pos = 0;
    while (pos < strv.length() && std::isspace(strv[pos])) {
        pos++;
    }
    strv.remove_prefix(pos);
    return strv;
}

bool parseHex2(std::string_view sv, std::uint8_t& out) noexcept
{
    if (sv.size() < 2) return false;
    auto hexVal = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        return -1;
    };
    int hi = hexVal(sv[0]);
    int lo = hexVal(sv[1]);
    if (hi < 0 || lo < 0) return false;
    out = static_cast<std::uint8_t>((hi << 4) | lo);
    return true;
}
