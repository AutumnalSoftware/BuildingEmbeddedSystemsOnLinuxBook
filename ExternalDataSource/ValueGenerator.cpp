// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#include "ValueGenerator.h"

#include <cmath>

namespace weather
{
ValueGenerator::ValueGenerator(GeneratorKind kind,
                               std::uint32_t seed,
                               double constant_value,
                               double sine_amplitude,
                               double sine_hz,
                               double random_walk_step_stddev) noexcept
    : m_kind(kind)
    , m_constant(constant_value)
    , m_amp(sine_amplitude)
    , m_sine_hz(sine_hz)
    , m_value(constant_value)
    , m_step_stddev(random_walk_step_stddev)
    , m_rng(seed)
{
}

double ValueGenerator::next(double dt_sec) noexcept
{
    switch (m_kind)
    {
        case GeneratorKind::Constant:
            return next_constant();
        case GeneratorKind::Sine:
            return next_sine(dt_sec);
        case GeneratorKind::RandomWalk:
            return next_random_walk();
        default:
            return next_random_walk();
    }
}

double ValueGenerator::next_constant() noexcept
{
    return m_constant;
}

double ValueGenerator::next_sine(double dt_sec) noexcept
{
    const double two_pi = 6.2831853071795864769;
    m_phase += two_pi * m_sine_hz * dt_sec;

    if (m_phase > two_pi)
    {
        m_phase = std::fmod(m_phase, two_pi);
    }

    return m_amp * std::sin(m_phase);
}

double ValueGenerator::next_random_walk() noexcept
{
    const double step = m_norm(m_rng) * m_step_stddev;
    m_value += step;
    return m_value;
}
} // namespace weather
