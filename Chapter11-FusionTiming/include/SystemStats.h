// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

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
};
} // namespace weather
