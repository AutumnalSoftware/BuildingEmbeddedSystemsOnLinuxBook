// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software
#pragma once

#include <chrono>

#include "AnyMeasurement.h"
#include "FusedWeatherSample.h"

namespace weather
{

class SystemStats;

/**
 * @brief Stateful fusion timing evaluator (entity semantics).
 *
 * FusionState represents evolving interpretation state over time.
 * It is modeled as an entity type.
 *
 * Copying would duplicate internal state semantically and permit two
 * divergent interpretations of the same input stream. Moving is also
 * disabled to keep ownership and lifetime obvious in the System Controller.
 *
 * FusionState owns no threads and no queues. It is called from a consumer run
 * loop that owns execution.
 */
class FusionState
{
public:
    static constexpr std::chrono::milliseconds matchWindow{250};  // window withing sensor readings are considered together.

    FusionState() = delete;

    explicit FusionState(SystemStats& stats) noexcept;

    ~FusionState() = default;

    /// @brief Non-copyable (entity semantics).
    FusionState(const FusionState&) = delete;

    /// @brief Non-copyable (entity semantics).
    FusionState& operator=(const FusionState&) = delete;

    /// @brief Non-movable (keeps ownership and lifetime explicit).
    FusionState(FusionState&&) = delete;

    /// @brief Non-movable (keeps ownership and lifetime explicit).
    FusionState& operator=(FusionState&&) = delete;

    /**
     * @brief Accepts a single measurement and updates internal state.
     *
     * This function performs no blocking and allocates no memory.
     * Timing policy and match logic live here (Chapter 11).
     */
    bool accept(const AnyMeasurement& m,
                FusedWeatherSample& out) noexcept;

private:
    struct TimedTemperature
    {
        Temperature value{};
        std::uint64_t rxTimeNs = 0;
        bool valid = false;
    };

    struct TimedPosition
    {
        Position value{};
        std::uint64_t rxTimeNs = 0;
        bool valid = false;
    };

    TimedTemperature m_temp;
    TimedPosition m_pos;

    std::uint64_t m_matchCount = 0;

    SystemStats& m_stats;
};


} // namespace weather
