// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <atomic>

#include "AnyMeasurement.h"
#include "LogEvent.h"
#include "readerwriterqueue/readerwriterqueue.h"
#include "FusionState.h"

#include "SystemStats.h"

class RunLoops
{
public:
    RunLoops(moodycamel::ReaderWriterQueue<weather::AnyMeasurement, 128>& inQ,
                     moodycamel::ReaderWriterQueue<LogEvent>& logQ);

    void producer(const std::atomic<bool>& stop);
    void consumer(const std::atomic<bool>& stop);
    void logger(const std::atomic<bool>& stop);

    void external_inputs(const std::atomic<bool>& stop);

private:
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement, 128>& m_inQ;
    moodycamel::ReaderWriterQueue<LogEvent>& m_logQ;

    weather::SystemStats m_stats;

    weather::FusionState m_fusion;
};
