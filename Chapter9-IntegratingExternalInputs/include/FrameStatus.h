#pragma once
#include <cstdint>

namespace weather
{
enum class FrameStatus : std::uint8_t
{
    GoodFrame = 0,
    VerifyFailure,
    Overflow,
    BadHeader
};
}
