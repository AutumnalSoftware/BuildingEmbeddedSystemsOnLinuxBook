// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#include <iostream>
#include <optional>
#include <string_view>

#include "NMEATokenizer.h"
#include "NMEAExtractionStream.h"
#include "NMEAInsertionStream.h"

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
    weather::Tokenizer tok(sentence);
    if (!tok.valid()) return false;

    out.checksumOk = tok.checksumValid();
    out.talker = tok.identifier().substr(0, 2);
    out.type   = tok.identifier().substr(2, 3);

    weather::ExtractionStream xs(tok);

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
    weather::InsertionStream ins("GPXYZ");

    ins.writeOptionalString(std::optional<std::string_view>("123519"));
    ins.writeDouble(4807.038);
    ins.writeChar('N');
    ins.writeDouble(1131.000);
    ins.writeChar('E');
    ins.finalize(true);

    std::cout << ins.sentence();

    ExamplePosSentence pos{};
    if (!parseExamplePos(ins.sentence(), pos))
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
