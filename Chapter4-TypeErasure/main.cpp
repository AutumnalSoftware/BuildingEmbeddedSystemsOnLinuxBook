#include <string>
#include <vector>
#include <iostream>

#include "AnyNmeaMessage.h"

struct ExamplePositionSentence
{
    std::string talker;
    std::string sentenceType;
    double latitude = 0.0;
    double longitude = 0.0;
    bool isChecksumValid() const
    {
       return true;
    }
};

struct ExampleStatusSentence
{
    std::string talker;
    std::string sentenceType;
    bool ok = false;
    bool isChecksumValid() const
    {
        return ok;
    }
};

int main(int argc, char** argv)
{
    std::vector<AnyNmeaSentence> sentences;

    sentences.emplace_back(
        ExamplePositionSentence{"GP", "POS", 43.12, -77.62}
        );

    sentences.emplace_back(
        ExampleStatusSentence{"GP", "STS", true}
        );

    for (const auto& s : sentences)
    {
        std::cout
            << "talker=" << s.talker()
            << " type=" << s.sentenceType()
            << " checksumValid=" << (s.isChecksumValid() ? "true" : "false")
            << "\n";
    }
}
