#pragma once

#include <cstddef>
#include <cstdint>
#include "NMEAStatus.h"

namespace moodycamel { template <typename T, std::size_t N> class ReaderWriterQueue; }

namespace weather
{
class AnyMeasurement;
struct PortStats;

class SensorContext
{
public:
    static constexpr std::size_t SboSizeBytes = 256;

    SensorContext() noexcept;
    ~SensorContext();

    SensorContext(const SensorContext&) = delete;
    SensorContext& operator=(const SensorContext&) = delete;

    SensorContext(SensorContext&&) noexcept;
    SensorContext& operator=(SensorContext&&) noexcept;

    int fd() const noexcept;
    Status on_readable() noexcept;

public:
    struct Ops
    {
        void (*destroy)(void*) noexcept;
        void (*move_construct)(void*, void*) noexcept;
        int (*get_fd)(const void*) noexcept;
        Status (*on_readable)(void*) noexcept;
    };

private:

    struct Storage
    {
        alignas(std::max_align_t) std::byte bytes[SboSizeBytes];
    };

    Storage mStorage{};
    const Ops* mOps = nullptr;

    void destroy_to_unconfigured() noexcept;

    friend void configure_udp_sensor(SensorContext& ctx,
                                     std::uint16_t port,
                                     moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                                     std::size_t maxMessageSize) noexcept;

    friend const PortStats* udp_stats(const SensorContext& ctx) noexcept;
};
}
