// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <atomic>

#include "AnyMeasurement.h"
#include "LogEvent.h"
#include "readerwriterqueue.h"

class Chapter9RunLoops
{
public:
    Chapter9RunLoops(moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& inQ,
                     moodycamel::ReaderWriterQueue<LogEvent>& logQ);

    void producer(const std::atomic<bool>& stop);
    void consumer(const std::atomic<bool>& stop);
    void logger(const std::atomic<bool>& stop);

private:
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement>& m_inQ;
    moodycamel::ReaderWriterQueue<LogEvent>& m_logQ;
};
