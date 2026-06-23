#pragma once

#include <atomic>
#include <cstddef>
#include <thread>

#include "readerwriterqueue.h"
#include "Counters.h"

#include "AnyMeasurement.h"

//
// A simple, fixed-topology pipeline:
// Producer thread -> SPSC queue -> Consumer thread
//
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

    void status(std::ostream& os);

private:
    void producerLoop();
    void consumerLoop();

private:
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement> m_queue;
    std::thread m_producerThread;
    std::thread m_consumerThread;
    std::atomic<bool> m_running { false };

    Counter producer_enqueued;
    Counter producer_blocked;
    Counter consumer_blocked;
    Counter consumer_dequeued;
};
