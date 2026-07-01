#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

#include "readerwriterqueue.h"
#include "Counters.h"

#include "AnyMeasurement.h"

class SensorPipelineIntervals;

//
// A simple, fixed-topology pipeline:
// Producer thread -> SPSC queue -> Consumer thread
//
class SensorPipeline
{
public:
    explicit SensorPipeline(std::size_t capacity); // creates this "holder" but doesn't start work
    explicit SensorPipeline(std::size_t capacity,
                            const SensorPipelineIntervals& producerIntervals,
                            const SensorPipelineIntervals& consumerIntervals);
    ~SensorPipeline();

    SensorPipeline(const SensorPipeline&) = delete;
    SensorPipeline& operator=(const SensorPipeline&) = delete;

    void start(); // create pipeline (including its threads) and start flow
    void stop();  // _request_ work to end (does not block)
    void join();  // wait for completion of pipeline (blocks) and end threads (and resources)
    // ...now SensorPipeline is safe to destruct

    void status(std::ostream& os);

private:
    void producerLoop();
    void consumerLoop();

private:
    moodycamel::ReaderWriterQueue<weather::AnyMeasurement> m_queue;
    std::thread m_producerThread;
    std::thread m_consumerThread;
    std::atomic<bool> m_running { false };

    const SensorPipelineIntervals& m_producerIntervals;
    const SensorPipelineIntervals& m_consumerIntervals;

    Counter producer_enqueued;
    Counter producer_blocked;
    Counter consumer_blocked;
    Counter consumer_dequeued;
};

struct SensorPipelineIntervals
{
    virtual std::chrono::nanoseconds pollingInterval() const = 0;
    virtual std::chrono::nanoseconds blockedWaitingInterval() const = 0;

    virtual ~SensorPipelineIntervals() = default;
};
