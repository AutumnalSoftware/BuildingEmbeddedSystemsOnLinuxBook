// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once
#include <cstdint>

namespace weather
{
struct PortStats
{
    std::uint64_t frames_received = 0;
    std::uint64_t frames_dropped = 0;
    std::uint64_t verify_failures = 0;
};
}
