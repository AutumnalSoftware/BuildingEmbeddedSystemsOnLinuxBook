// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include <array>
#include <atomic>
#include <thread>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <iomanip>

#include <sys/epoll.h>
#include <unistd.h>

#include "RunLoops.h"
#include "SensorContext.h"
#include "UdpSensor.h"


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

static void updateMax(std::atomic<std::uint64_t>& maxVal,
                      std::uint64_t value)
{
    std::uint64_t cur = maxVal.load(std::memory_order_relaxed);
    while (value > cur &&
           !maxVal.compare_exchange_weak(cur, value,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed))
    {
        // cur updated
    }
}

static bool add_epoll(int epfd, weather::SensorContext& ctx)
{
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.ptr = &ctx;

    if (::epoll_ctl(epfd, EPOLL_CTL_ADD, ctx.fd(), &ev) < 0)
    {
        std::cerr << "epoll_ctl add failed: " << std::strerror(errno) << "\n";
        return false;
    }

    return true;
}


RunLoops::RunLoops(
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement, 128>& inQ,
    moodycamel::ReaderWriterQueue<LogEvent>& logQ)
    : m_inQ(inQ)
    , m_logQ(logQ)
    , m_stats {}
    , m_fusion(m_stats)
{
}

void RunLoops::producer(const std::atomic<bool>& stop)
{
    using namespace std::chrono;

    auto nextTemp = steady_clock::now();
    auto nextPos  = steady_clock::now();

    while (!stop.load(std::memory_order_relaxed))
    {
        const auto now = steady_clock::now();

        if (now >= nextTemp)
        {
            weather::MeasurementHeaderV1 h{};
            h.rxTime = duration_cast<nanoseconds>(now.time_since_epoch()).count();
            h.eventTime = h.rxTime;

            weather::Temperature t{25.0};

            if (m_inQ.try_enqueue(weather::AnyMeasurement(h, t)))
            {
                m_stats.enqTempOk.fetch_add(1, std::memory_order_relaxed);

                const auto depth =
                    m_stats.inQDepth.fetch_add(1, std::memory_order_relaxed) + 1;

                updateHighWater(m_stats.inQDepthHi, depth);
            }
            else
            {
                m_stats.enqDrops.fetch_add(1, std::memory_order_relaxed);
            }

            nextTemp += milliseconds(1);
        }

        if (now >= nextPos)
        {
            weather::MeasurementHeaderV1 h{};
            h.rxTime = duration_cast<nanoseconds>(now.time_since_epoch()).count();
            h.eventTime = h.rxTime;

            weather::Position p{42.0, -77.0, 150.0};

            if (m_inQ.try_enqueue(weather::AnyMeasurement(h, p)))
            {
                m_stats.enqPosOk.fetch_add(1, std::memory_order_relaxed);

                const auto depth =
                    m_stats.inQDepth.fetch_add(1, std::memory_order_relaxed) + 1;

                updateHighWater(m_stats.inQDepthHi, depth);
            }
            else
            {
                m_stats.enqDrops.fetch_add(1, std::memory_order_relaxed);
            }

            nextPos += seconds(1);
        }

        std::this_thread::sleep_for(milliseconds(1));
    }
}

void RunLoops::consumer(const std::atomic<bool>& stop)
{
    using namespace std::chrono;

    weather::AnyMeasurement msg = weather::AnyMeasurement::empty();

    while (!stop.load(std::memory_order_relaxed))
    {
        if (m_inQ.try_dequeue(msg))
        {
            m_stats.inQDepth.fetch_sub(1, std::memory_order_relaxed);

            switch (msg.kind())
            {
            case weather::MeasurementKind::Temperature:
                m_stats.deqTemp.fetch_add(1, std::memory_order_relaxed);
                break;

            case weather::MeasurementKind::Position:
                m_stats.deqPos.fetch_add(1, std::memory_order_relaxed);
                break;

            default:
                break;
            }

            const auto now = steady_clock::now();
            const std::uint64_t nowNs =
                static_cast<std::uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());

            const std::uint64_t rxNs =
                static_cast<std::uint64_t>(msg.header().rxTime);

            if (nowNs >= rxNs)
            {
                updateMax(m_stats.latencyMaxNs, nowNs - rxNs);
            }

            m_fusion.accept(msg);
        }
        else
        {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
}


void RunLoops::logger(const std::atomic<bool>& stop)
{
    using namespace std::chrono;

    auto start      = steady_clock::now();
    auto lastReport = start;

    std::uint64_t lastEnqTempOk = 0;
    std::uint64_t lastEnqPosOk  = 0;
    std::uint64_t lastDeqTemp   = 0;
    std::uint64_t lastDeqPos    = 0;
    std::uint64_t lastDrops     = 0;

    std::uint64_t lastFusionMatches       = 0;
    std::uint64_t lastFusionNoTemp        = 0;
    std::uint64_t lastFusionNoPos         = 0;
    std::uint64_t lastFusionOutsideWindow = 0;

    while (!stop.load(std::memory_order_relaxed))
    {
        const auto now = steady_clock::now();

        if (now - lastReport >= seconds(1))
        {
            const auto tSec =
                duration_cast<seconds>(now - start).count();

            const std::uint64_t enqTempOk =
                m_stats.enqTempOk.load(std::memory_order_relaxed);
            const std::uint64_t enqPosOk =
                m_stats.enqPosOk.load(std::memory_order_relaxed);

            const std::uint64_t deqTemp =
                m_stats.deqTemp.load(std::memory_order_relaxed);
            const std::uint64_t deqPos =
                m_stats.deqPos.load(std::memory_order_relaxed);

            const std::uint64_t drops =
                m_stats.enqDrops.load(std::memory_order_relaxed);

            const std::uint64_t enqTempPerSec = enqTempOk - lastEnqTempOk;
            const std::uint64_t enqPosPerSec  = enqPosOk  - lastEnqPosOk;
            const std::uint64_t deqTempPerSec = deqTemp   - lastDeqTemp;
            const std::uint64_t deqPosPerSec  = deqPos    - lastDeqPos;
            const std::uint64_t dropsPerSec   = drops     - lastDrops;

            lastEnqTempOk = enqTempOk;
            lastEnqPosOk  = enqPosOk;
            lastDeqTemp   = deqTemp;
            lastDeqPos    = deqPos;
            lastDrops     = drops;

            const std::uint64_t fusionMatches =
                m_stats.fusionMatches.load(std::memory_order_relaxed);
            const std::uint64_t fusionNoTemp =
                m_stats.fusionNoTemp.load(std::memory_order_relaxed);
            const std::uint64_t fusionNoPos =
                m_stats.fusionNoPos.load(std::memory_order_relaxed);
            const std::uint64_t fusionOutsideWindow =
                m_stats.fusionOutsideWindow.load(std::memory_order_relaxed);


            const std::uint64_t depth =
                m_stats.inQDepth.load(std::memory_order_relaxed);

            const std::uint64_t hi =
                m_stats.inQDepthHi.load(std::memory_order_relaxed);

            const std::uint64_t latMaxNs =
                m_stats.latencyMaxNs.exchange(0, std::memory_order_relaxed);

            const double latMaxMs =
                static_cast<double>(latMaxNs) / 1'000'000.0;

            const std::uint64_t fusionMatchesPerSec =
                fusionMatches - lastFusionMatches;
            const std::uint64_t fusionNoTempPerSec =
                fusionNoTemp - lastFusionNoTemp;
            const std::uint64_t fusionNoPosPerSec =
                fusionNoPos - lastFusionNoPos;
            const std::uint64_t fusionOutsideWindowPerSec =
                fusionOutsideWindow - lastFusionOutsideWindow;

            std::cout
                << "[STAT t=" << tSec << "s] "
                << "enq(temp=" << enqTempPerSec << "/s pos=" << enqPosPerSec << "/s) "
                << "deq(temp=" << deqTempPerSec << "/s pos=" << deqPosPerSec << "/s) "
                << "drops=" << drops << " (+" << dropsPerSec << "/s) "
                << "q(depth=" << depth << " hi=" << hi << ") "
                << "fusion(match=" << fusionMatchesPerSec
                << "/s missWin=" << fusionOutsideWindowPerSec
                << "/s noTemp=" << fusionNoTempPerSec
                << "/s noPos=" << fusionNoPosPerSec << "/s) "
                << "latMax=" << std::fixed << std::setprecision(2) << latMaxMs << "ms\n";

            lastReport = now;

            lastFusionMatches       = fusionMatches;
            lastFusionNoTemp        = fusionNoTemp;
            lastFusionNoPos         = fusionNoPos;
            lastFusionOutsideWindow = fusionOutsideWindow;
        }
        else
        {
            std::this_thread::sleep_for(milliseconds(10));
        }
    }
}

void RunLoops::external_inputs(const std::atomic<bool>& stop)
{
    auto& inQ = m_inQ;

    // UDP-only for the first Chapter 11 milestone.
    std::array<weather::SensorContext, 1> sensors{};

    // weather_tx should target this port.
    weather::configure_udp_sensor(sensors[0], 9000, inQ, 4096, m_stats);

    const int epfd = ::epoll_create1(0);
    if (epfd < 0)
    {
        std::cerr << "epoll_create1 failed: " << std::strerror(errno) << "\n";
        return;
    }

    if (!add_epoll(epfd, sensors[0]))
    {
        ::close(epfd);
        return;
    }

    std::cout << "Chapter 11: UDP BDS ingest\n";
    std::cout << "  Receiver: UDP 9000\n";
    std::cout << "  Example:\n";
    std::cout << "    weather_tx --udp 127.0.0.1:9000 --temp-hz 10 --position-hz 1 --duration 0\n";
    std::cout << "Ctrl-C to stop.\n";

    epoll_event events[8]{};

    while (!stop.load(std::memory_order_relaxed))
    {
        const int n = ::epoll_wait(epfd, events, 8, 250);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait failed: " << std::strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            auto* ctx = reinterpret_cast<weather::SensorContext*>(events[i].data.ptr);
            if (ctx) (void)ctx->on_readable();
        }
    }

    ::close(epfd);
}

