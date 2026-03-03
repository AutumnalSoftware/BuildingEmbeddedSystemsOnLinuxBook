// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "FusionState.h"
#include "SystemStats.h"

weather::FusionState::FusionState(SystemStats& stats) noexcept :
    m_stats(stats)
{

}

void weather::FusionState::accept(const weather::AnyMeasurement& m) noexcept
{
    bool updated = false;

    const auto kind = m.kind();

    if (kind == MeasurementKind::Temperature)
    {
        if (const auto* t = m.try_get<Temperature>())
        {
            m_temp.value = *t;
            m_temp.rxTimeNs = m.header().rxTime;
            m_temp.valid = true;
            updated = true;
        }
    }
    else if (kind == MeasurementKind::Position)
    {
        if (const auto* p = m.try_get<Position>())
        {
            m_pos.value = *p;
            m_pos.rxTimeNs = m.header().rxTime;
            m_pos.valid = true;
            updated = true;
        }
    }

    if (!updated)
    {
        return;
    }

    if (!m_temp.valid)
    {
        m_stats.fusionNoTemp.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    if (!m_pos.valid)
    {
        m_stats.fusionNoPos.fetch_add(1, std::memory_order_relaxed);
        return;
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
    }
    else
    {
        m_stats.fusionOutsideWindow.fetch_add(1, std::memory_order_relaxed);
    }
}
