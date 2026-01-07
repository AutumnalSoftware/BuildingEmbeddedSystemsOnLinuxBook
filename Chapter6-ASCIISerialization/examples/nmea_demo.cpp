// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#include <iostream>
#include <optional>
#include <string_view>

#include "nmea/NMEATokenizer.h"
#include "nmea/NMEAExtractionStream.h"

struct ExamplePosSentence
{
    std::string_view talker;
    std::string_view type;
    double latitude = 0.0;
    char ns = 'N';
    double longitude = 0.0;
    char ew = 'E';
    bool checksumOk = false;
};

static bool parseExamplePos(std::string_view sentence, ExamplePosSentence& out)
{
    nmea::Tokenizer tok(sentence);
    if (!tok.valid()) return false;

    out.checksumOk = tok.checksumValid();
    out.talker = tok.identifier().substr(0, 2);
    out.type   = tok.identifier().substr(2, 3);

    nmea::ExtractionStream xs(tok);

    std::optional<std::string_view> time;
    xs.readOptionalString(time);

    if (!xs.readDouble(out.latitude).ok()) return false;
    if (!xs.readChar(out.ns).ok()) return false;
    if (!xs.readDouble(out.longitude).ok()) return false;
    if (!xs.readChar(out.ew).ok()) return false;

    return true;
}

int main()
{
    const std::string_view s = "$GPXYZ,123519,4807.038,N,01131.000,E*54\r\n";

    ExamplePosSentence pos{};
    if (!parseExamplePos(s, pos))
    {
        std::cout << "Parse failed\n";
        return 1;
    }

    std::cout << "id=" << pos.talker << pos.type
              << " lat=" << pos.latitude << pos.ns
              << " lon=" << pos.longitude << pos.ew
              << " checksumOk=" << (pos.checksumOk ? "true" : "false")
              << "\n";
    return 0;
}
