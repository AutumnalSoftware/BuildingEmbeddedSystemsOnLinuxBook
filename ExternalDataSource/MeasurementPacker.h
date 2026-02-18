// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software
#pragma once

#include <cstddef>
#include <vector>

#include "BinaryWriteStream.h"
#include "BdsMeasurementCodecs.h"
#include "ImmutableByteView.h"
#include "MutableByteView.h"
#include "MeasurementHeaderV1.h"
#include "MeasurementTypes.h"

namespace weather
{
class MeasurementPacker
{
public:
    explicit MeasurementPacker(std::size_t capacityBytes)
        : m_buf(capacityBytes)
    {
    }

    bool packTemperature(const MeasurementHeaderV1& header,
                         const Temperature& m,
                         ImmutableByteView& outPayload) noexcept
    {
        return pack_impl(header,
                         [&](BinaryWriteStream& bs)
                         {
                             writeTemperature(bs, m);
                         },
                         outPayload);
    }

    bool packBarometricPressure(const MeasurementHeaderV1& header,
                                const BarometricPressure& m,
                                ImmutableByteView& outPayload) noexcept
    {
        return pack_impl(header,
                         [&](BinaryWriteStream& bs)
                         {
                             writeBarometricPressure(bs, m);
                         },
                         outPayload);
    }

    bool packHumidity(const MeasurementHeaderV1& header,
                      const Humidity& m,
                      ImmutableByteView& outPayload) noexcept
    {
        return pack_impl(header,
                         [&](BinaryWriteStream& bs)
                         {
                             writeHumidity(bs, m);
                         },
                         outPayload);
    }

    bool packWindSpeed(const MeasurementHeaderV1& header,
                       const WindSpeed& m,
                       ImmutableByteView& outPayload) noexcept
    {
        return pack_impl(header,
                         [&](BinaryWriteStream& bs)
                         {
                             writeWindSpeed(bs, m);
                         },
                         outPayload);
    }

    bool packWindDirection(const MeasurementHeaderV1& header,
                           const WindDirection& m,
                           ImmutableByteView& outPayload) noexcept
    {
        return pack_impl(header,
                         [&](BinaryWriteStream& bs)
                         {
                             writeDirection(bs, m);
                         },
                         outPayload);
    }

    bool packPrecipitation(const MeasurementHeaderV1& header,
                           const Precipitation& m,
                           ImmutableByteView& outPayload) noexcept
    {
        return pack_impl(header,
                         [&](BinaryWriteStream& bs)
                         {
                             writePrecipitation(bs, m);
                         },
                         outPayload);
    }

    bool packPosition(const MeasurementHeaderV1& header,
                      const Position& m,
                      ImmutableByteView& outPayload) noexcept
    {
        return pack_impl(header,
                         [&](BinaryWriteStream& bs)
                         {
                             writePosition(bs, m);
                         },
                         outPayload);
    }

private:
    template <typename Fn>
    bool pack_impl(const MeasurementHeaderV1& header,
                   Fn&& write_body,
                   ImmutableByteView& outPayload) noexcept
    {
        MutableByteView dst(m_buf.data(), m_buf.size());
        BinaryWriteStream bs(dst, Endianness::Little);

        writeMeasurementHeader(bs, header);
        write_body(bs);

        if (!bs.ok())
        {
            outPayload = ImmutableByteView{};
            return false;
        }

        outPayload = ImmutableByteView(m_buf.data(), bs.bytesWritten());
        return true;
    }

private:
    std::vector<std::byte> m_buf;
};

} // namespace weather
