#pragma once

#include "BuildStatus.h"
#include "AnyMeasurement.h"
#include "LogEvent.h"
#include "Chapter9RunLoops.h"
#include "WeatherSystem.h"

#include <optional>

#include "readerwriterqueue.h"

class WeatherSystemBuilder
{
public:
    WeatherSystemBuilder();

    WeatherSystemBuilder& withChapter9Demo();

    BuildStatus build(WeatherSystem& system);

private:
    BuildStatus validate() const noexcept;

private:
    bool m_hasChapter9Demo { false };

    // IMPORTANT: This builder must outlive the WeatherSystem while it runs.
    // It owns the queues and the run loop object referenced by thread entry points.
    std::optional<moodycamel::ReaderWriterQueue<AnyMeasurement>> m_inQ;
    std::optional<moodycamel::ReaderWriterQueue<LogEvent>>       m_logQ;
    std::optional<Chapter9RunLoops>                              m_runLoops;
};
