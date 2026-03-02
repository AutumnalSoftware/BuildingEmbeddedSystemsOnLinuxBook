// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "UdpSensor.h"
#include "SensorContext.h"

#include "BdsMeasurementCodecs.h"
#include "BinaryReadStream.h"
//#include "ImmutableByteView.h"
#include "MutableByteView.h"
#include "MeasurementTypes.h"

#include <chrono>

#include <cassert>
#include <array>
#include <atomic>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace weather
{
static constexpr std::size_t MaxUdpDatagramBytes = 4096;

static std::uint64_t now_ns() noexcept
{
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}


//
// Helpers
//
static void updateHighWater(std::atomic<std::uint64_t>& high,
                            std::uint64_t value)
{
    std::uint64_t cur = high.load(std::memory_order_relaxed);
    while (value > cur &&
           !high.compare_exchange_weak(cur, value,
                                       std::memory_order_relaxed,
                                       std::memory_order_relaxed))
    {
        // cur updated
    }
}

static bool decode_bds_to_any(const std::byte* data,
                              std::size_t size,
                              AnyMeasurement& out) noexcept
{
    MutableByteView buf(const_cast<std::byte*>(data), size);
    BinaryReadStream bs(buf, Endianness::Little);

    MeasurementHeaderV1 h{};
    readMeasurementHeader(bs, h);
    if (!bs.ok())
    {
        return false;
    }

    // Receive time is established at ingress.
    h.rxTime = now_ns();

    switch (h.kind)
    {
    case MeasurementKind::Temperature:
    {
        Temperature m{};
        readTemperature(bs, m);
        if (!bs.ok()) return false;
        out = AnyMeasurement(h, m);
        return true;
    }
    case MeasurementKind::BarometricPressure:
    {
        BarometricPressure m{};
        readBarometricPressure(bs, m);
        if (!bs.ok()) return false;
        out = AnyMeasurement(h, m);
        return true;
    }
    case MeasurementKind::Humidity:
    {
        Humidity m{};
        readHumidity(bs, m);
        if (!bs.ok()) return false;
        out = AnyMeasurement(h, m);
        return true;
    }
    case MeasurementKind::WindSpeed:
    {
        WindSpeed m{};
        readWindSpeed(bs, m);
        if (!bs.ok()) return false;
        out = AnyMeasurement(h, m);
        return true;
    }
    case MeasurementKind::WindDirection:
    {
        WindDirection m{};
        readWindDirection(bs, m);
        if (!bs.ok()) return false;
        out = AnyMeasurement(h, m);
        return true;
    }
    case MeasurementKind::Precipitation:
    {
        Precipitation m{};
        readPrecipitation(bs, m);
        if (!bs.ok()) return false;
        out = AnyMeasurement(h, m);
        return true;
    }
    case MeasurementKind::Position:
    {
        Position m{};
        readPosition(bs, m);
        if (!bs.ok()) return false;
        out = AnyMeasurement(h, m);
        return true;
    }
    default:
        return false;
    }
}

struct UdpSensorImpl
{
    int fd = -1;
    std::size_t maxMessageSize = 0;
    moodycamel::ReaderWriterQueue<AnyMeasurement, 128>* outQ = nullptr;
    SystemStats* sysStats = nullptr;
    PortStats stats{};

    UdpSensorImpl(std::uint16_t port,
                  moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& q,
                  std::size_t maxMsg,
                  SystemStats& s) noexcept
        : maxMessageSize(maxMsg), outQ(&q), sysStats(&s)
    {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        bind(fd, (sockaddr*)&addr, sizeof(addr));
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }

    ~UdpSensorImpl()
    {
        if (fd >= 0) close(fd);
    }

    int get_fd() const noexcept { return fd; }

    Status on_readable() noexcept
    {
        for (;;)
        {
            std::array<std::byte, MaxUdpDatagramBytes> rx{};
            ssize_t n = recv(fd, rx.data(), rx.size(), 0);
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return Status{};
                stats.frames_dropped++;
                return Status{};
            }

            stats.frames_received++;
            if ((std::size_t)n > maxMessageSize)
            {
                stats.frames_dropped++;
                continue;
            }

            AnyMeasurement m = AnyMeasurement::empty();
            if (!decode_bds_to_any(rx.data(), static_cast<std::size_t>(n), m))
            {
                stats.verify_failures++;
                continue;
            }

            const auto kind = m.kind();

            if (outQ->try_enqueue(std::move(m)))
            {
                switch(kind)
                {
                case MeasurementKind::Temperature:
                    sysStats->enqTempOk.fetch_add(1, std::memory_order_relaxed);
                    break;

                case MeasurementKind::Position:
                    sysStats->enqPosOk.fetch_add(1, std::memory_order_relaxed);
                    break;

                default:
                    break;
                }

                const auto depth =
                    sysStats->inQDepth.fetch_add(1, std::memory_order_relaxed) + 1;

                updateHighWater(sysStats->inQDepthHi, depth);
            }
            else
            {
                sysStats->enqDrops.fetch_add(1, std::memory_order_relaxed);
                stats.frames_dropped++;
            }
        }
    }
};

static void udp_destroy(void* p) noexcept { reinterpret_cast<UdpSensorImpl*>(p)->~UdpSensorImpl(); }
static void udp_move(void*, void*) noexcept { assert(false); }
static int udp_fd(const void* p) noexcept { return reinterpret_cast<const UdpSensorImpl*>(p)->get_fd(); }
static Status udp_read(void* p) noexcept { return reinterpret_cast<UdpSensorImpl*>(p)->on_readable(); }

static const SensorContext::Ops& UdpOps() noexcept
{
    static const SensorContext::Ops ops{
        &udp_destroy, &udp_move, &udp_fd, &udp_read
    };
    return ops;
}

void configure_udp_sensor(SensorContext& ctx,
                          std::uint16_t port,
                          moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                          std::size_t maxMessageSize,
                          SystemStats& sysStats) noexcept
{
    ctx.destroy_to_unconfigured();
    ctx.mOps = &UdpOps();
    new (ctx.mStorage.bytes) UdpSensorImpl(port, outQ, maxMessageSize, sysStats);
}

const PortStats* udp_stats(const SensorContext& ctx) noexcept
{
    if (ctx.mOps != &UdpOps()) return nullptr;
    return &reinterpret_cast<const UdpSensorImpl*>(ctx.mStorage.bytes)->stats;
}

}
