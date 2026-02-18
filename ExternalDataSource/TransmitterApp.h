// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#pragma once

#include "MeasurementHeaderV1.h"
#include "MeasurementPacker.h"
#include "MeasurementTypes.h"
#include "StreamScheduler.h"
#include "TimeUtils.h"
#include "ValueGenerator.h"

#include <memory>
#include <optional>

namespace weather
{

class ITransmitEndpoint
{
public:
    virtual ~ITransmitEndpoint() = default;
    virtual bool send(ImmutableByteView bytes) noexcept = 0;
};

struct Options
{
    std::optional<std::string> uart_device;
    int uart_baud = 115200;

    double temp_hz = 0.0;
    double pressure_hz = 0.0;
    double humidity_hz = 0.0;
    double position_hz = 0.0;
    double wind_hz = 0.0;

    GeneratorKind gen = GeneratorKind::RandomWalk;
    std::uint32_t seed = 1;

    double duration_sec = 0.0;
    double log_every_sec = 5.0;
};

class TransmitterApp
{
public:
    TransmitterApp(const Options& opt,
                   std::unique_ptr<ITransmitEndpoint> sink);

    int run() noexcept;

private:
    void initStreams();
    bool emitTemperature(double dt_sec) noexcept;

private:
    Options m_opt{};
    std::unique_ptr<ITransmitEndpoint> m_sink;

    MeasurementPacker m_packer{4096};
    StreamScheduler m_sched{};

    ValueGenerator m_temp_gen{};

    static constexpr std::size_t Stream_Temperature = 1;
};
} // namespace weather
