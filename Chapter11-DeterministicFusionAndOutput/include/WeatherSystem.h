// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <thread>

#include "AnyMeasurement.h"
#include "FusedWeatherSample.h"
#include "LogEvent.h"
#include "readerwriterqueue/readerwriterqueue.h"

class RunLoops;

class WeatherSystem
{
public:
    static constexpr std::size_t ThreadCount = 4;

    WeatherSystem();
    ~WeatherSystem();

    WeatherSystem(const WeatherSystem&) = delete;
    WeatherSystem& operator=(const WeatherSystem&) = delete;

    void setThreadEntry(std::size_t index,
                        std::function<void(const std::atomic<bool>&)> entry);

    void start();
    void stop() noexcept;
    void join();

    moodycamel::ReaderWriterQueue<weather::AnyMeasurement, 128>& inQueue() noexcept;
    moodycamel::ReaderWriterQueue<LogEvent>& logQueue() noexcept;

    RunLoops& runLoops() noexcept;

private:
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement, 128> m_inQ;
    moodycamel::ReaderWriterQueue<LogEvent> m_logQ;
    moodycamel::ReaderWriterQueue<weather::FusedWeatherSample, 64> m_fusedQ;

    RunLoops* m_runLoops;

    std::array<std::function<void(const std::atomic<bool>&)>, ThreadCount> m_entries {};
    std::array<std::thread, ThreadCount> m_threads {};

    std::atomic<bool> m_stopRequested { false };
};
