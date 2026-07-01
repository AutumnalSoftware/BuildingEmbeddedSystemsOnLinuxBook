// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#include "WeatherSystem.h"
#include "Chapter8RunLoops.h"

WeatherSystem::WeatherSystem()
    : m_inQ(256)
    , m_logQ(64)
    , m_runLoops(new Chapter8RunLoops(m_inQ, m_logQ))
{
}

WeatherSystem::~WeatherSystem()
{
    stop();
    join();
    delete m_runLoops;
    m_runLoops = nullptr;
}

void WeatherSystem::setThreadEntry(std::size_t index,
                                   std::function<void(const std::atomic<bool>&)> entry)
{
    m_entries[index] = std::move(entry);
}

void WeatherSystem::start()
{
    for (std::size_t i = 0; i < ThreadCount; ++i)
    {
        m_threads[i] = std::thread(m_entries[i], std::cref(m_stopRequested));
    }
}

void WeatherSystem::stop() noexcept
{
    m_stopRequested.store(true, std::memory_order_relaxed);
}

void WeatherSystem::join()
{
    for (auto& t : m_threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& WeatherSystem::inQueue() noexcept
{
    return m_inQ;
}

moodycamel::ReaderWriterQueue<LogEvent>& WeatherSystem::logQueue() noexcept
{
    return m_logQ;
}

Chapter8RunLoops& WeatherSystem::runLoops() noexcept
{
    return *m_runLoops;
}
