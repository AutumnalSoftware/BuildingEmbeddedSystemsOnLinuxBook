#pragma once

#include <atomic>
#include <cstddef>
#include <thread>

#include "readerwriterqueue.h"

#include "AnyMeasurement.h"

// A deliberately simple, fixed-topology pipeline:
// Producer thread -> SPSC queue -> Consumer thread
class SensorPipeline
{
public:
    explicit SensorPipeline(std::size_t capacity);
    ~SensorPipeline();

    SensorPipeline(const SensorPipeline&) = delete;
    SensorPipeline& operator=(const SensorPipeline&) = delete;

    void start();
    void stop();
    void join();

private:
    void producerLoop();
    void consumerLoop();

private:
    std::atomic<bool> m_running { false };

    // Note: ReaderWriterQueue is SPSC. Capacity is a hint / initial sizing.
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement> m_queue;

    std::thread m_producerThread;
    std::thread m_consumerThread;
};
