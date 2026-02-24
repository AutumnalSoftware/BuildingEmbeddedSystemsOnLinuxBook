// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <atomic>
#include <cstdint>

namespace weather
{

struct SystemStats
{
    std::atomic<std::uint64_t> enqTempOk{0};
    std::atomic<std::uint64_t> enqPosOk{0};

    std::atomic<std::uint64_t> deqTemp{0};
    std::atomic<std::uint64_t> deqPos{0};

    std::atomic<std::uint64_t> enqDrops{0};

    std::atomic<std::uint64_t> inQDepth{0};
    std::atomic<std::uint64_t> inQDepthHi{0};

    std::atomic<std::uint64_t> latencyMaxNs{0};

    // Chapter 11: fusion timing policy observability
    std::atomic<std::uint64_t> fusionMatches{0};
    std::atomic<std::uint64_t> fusionNoTemp{0};
    std::atomic<std::uint64_t> fusionNoPos{0};
    std::atomic<std::uint64_t> fusionOutsideWindow{0};
};

} // namespace weather
