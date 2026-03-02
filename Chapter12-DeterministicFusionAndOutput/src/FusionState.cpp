// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "FusionState.h"
#include "SystemStats.h"

weather::FusionState::FusionState(SystemStats& stats) noexcept :
    m_stats(stats)
{
}

bool weather::FusionState::accept(const weather::AnyMeasurement& m,
                                  weather::FusedWeatherSample& out) noexcept
{
    bool updatedTemp = false;
    bool updatedPos  = false;

    const auto kind = m.kind();

    if (kind == MeasurementKind::Temperature)
    {
        if (const auto* t = m.try_get<Temperature>())
        {
            m_temp.value = *t;
            m_temp.rxTimeNs = m.header().rxTime;
            m_temp.valid = true;
            updatedTemp = true;
        }
    }
    else if (kind == MeasurementKind::Position)
    {
        if (const auto* p = m.try_get<Position>())
        {
            m_pos.value = *p;
            m_pos.rxTimeNs = m.header().rxTime;
            m_pos.valid = true;
            updatedPos = true;
        }
    }

    if (!updatedTemp && !updatedPos)
    {
        return false;
    }

    // Emit only when Position updates so output cadence is readable.
    if (!updatedPos)
    {
        return false;
    }

    if (!m_temp.valid)
    {
        m_stats.fusionNoTemp.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (!m_pos.valid)
    {
        m_stats.fusionNoPos.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    const std::uint64_t dt =
        (m_temp.rxTimeNs > m_pos.rxTimeNs)
            ? (m_temp.rxTimeNs - m_pos.rxTimeNs)
            : (m_pos.rxTimeNs - m_temp.rxTimeNs);

    const std::uint64_t windowNs =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(matchWindow).count());

    if (dt <= windowNs)
    {
        m_stats.fusionMatches.fetch_add(1, std::memory_order_relaxed);

        out.temperature = m_temp.value;
        out.position    = m_pos.value;
        out.tempRxTimeNs = m_temp.rxTimeNs;
        out.posRxTimeNs  = m_pos.rxTimeNs;
        out.dtNs         = dt;

        return true;
    }

    m_stats.fusionOutsideWindow.fetch_add(1, std::memory_order_relaxed);
    return false;
}
