// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#pragma once

#include <chrono>
#include <cstdint>

namespace weather
{
inline std::uint64_t now_ns() noexcept
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(
        steady_clock::now().time_since_epoch()).count();
}
} // namespace weather
