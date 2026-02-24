// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#pragma once

#include "MeasurementHeaderV1.h"
#include "MeasurementPacker.h"
#include "MeasurementTypes.h"
#include "StreamScheduler.h"
#include "TimeUtils.h"
#include "ValueGenerator.h"
#include "ITransmitEndpoint.h"
#include "Options.h"

#include <memory>
#include <optional>

namespace weather
{

class TransmitterApp
{
public:
    TransmitterApp(const weather::Options& opt,
                   std::unique_ptr<ITransmitEndpoint> sink);

    int run() noexcept;

private:
    void initStreams();
    bool emitTemperature(double dt_sec) noexcept;
    bool emitPosition(double dt_sec) noexcept;

private:
    Options m_opt{};
    std::unique_ptr<ITransmitEndpoint> m_sink;

    MeasurementPacker m_packer{4096};
    StreamScheduler m_sched{};

    ValueGenerator m_temp_gen{};

    ValueGenerator m_pos_lat_gen{};
    ValueGenerator m_pos_lon_gen{};
    ValueGenerator m_pos_alt_gen{};

    static constexpr std::size_t Stream_Temperature = 1;
    static constexpr std::size_t Stream_Position    = 2;
};
} // namespace weather
