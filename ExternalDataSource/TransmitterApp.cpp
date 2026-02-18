// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#include "TransmitterApp.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace weather
{
TransmitterApp::TransmitterApp(const weather::Options& opt,
                               std::unique_ptr<ITransmitEndpoint> sink)
    : m_opt(opt)
    , m_sink(std::move(sink))
    , m_packer(4096)
    , m_temp_gen(opt.gen,
                 opt.seed,
                 22.0,
                 5.0,
                 0.05,
                 0.02)
{
}

void TransmitterApp::initStreams()
{
    if (m_opt.temp_hz > 0.0)
    {
        m_sched.addStream(Stream_Temperature, m_opt.temp_hz);
    }
}

bool TransmitterApp::emitTemperature(double dt_sec) noexcept
{
    Temperature t{};
    t.value = m_temp_gen.next(dt_sec);

    MeasurementHeaderV1 h{};
    const std::uint64_t ts = now_ns();

    h.rxTime = ts;
    h.eventTime = ts;
    h.kind = MeasurementKind::Temperature;
    h.source = SourceId::Unknown;
    h.flags = 0;

    ImmutableByteView payload{};
    if (!m_packer.packTemperature(h, t, payload))
    {
        return false;
    }

    return m_sink->send(payload);
}

int TransmitterApp::run() noexcept
{
    initStreams();

    if (m_sched.empty())
    {
        std::cerr << "No streams enabled.\n";
        return 2;
    }

    using Clock = StreamScheduler::Clock;

    const auto start = Clock::now();
    auto next_log = start + std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(m_opt.log_every_sec));

    std::uint64_t sent = 0;
    std::uint64_t send_fail = 0;

    while (true)
    {
        const auto now = Clock::now();

        if (m_opt.duration_sec > 0.0)
        {
            const auto elapsed = std::chrono::duration<double>(now - start).count();
            if (elapsed >= m_opt.duration_sec)
            {
                break;
            }
        }

        const auto& s = m_sched.nextStream();

        if (now < s.next_due)
        {
            std::this_thread::sleep_until(s.next_due);
        }

        const auto fire = Clock::now();
        const double dt_sec = std::chrono::duration<double>(fire - s.last_fire).count();

        bool ok = false;

        switch (s.id)
        {
            case Stream_Temperature:
                ok = emitTemperature(dt_sec);
                break;
            default:
                ok = false;
                break;
        }

        if (!ok)
        {
            send_fail++;
        }
        else
        {
            sent++;
        }

        m_sched.markFired(s.id, fire);

        const auto after = Clock::now();
        if (after >= next_log)
        {
            std::cout << "tx: sent=" << sent
                      << " fail=" << send_fail
                      << "\n";

            next_log = after + std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>(m_opt.log_every_sec));
        }
    }

    std::cout << "tx: done\n";
    return 0;
}
} // namespace weather
