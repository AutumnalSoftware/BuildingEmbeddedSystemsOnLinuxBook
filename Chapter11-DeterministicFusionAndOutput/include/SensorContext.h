// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <cstddef>
#include <cstdint>

#include "Status.h"

namespace moodycamel { template <typename T, std::size_t N> class ReaderWriterQueue; }

namespace weather
{
class AnyMeasurement;
struct PortStats;
struct SystemStats;

/**
 * @brief Reactor-owned sensor wrapper with explicit configuration.
 *
 * SensorContext is constructed in a valid but unconfigured state.
 * In this state it owns no file descriptor and performs no I/O.
 *
 * Configuration installs a concrete sensor implementation (UDP, UART, etc.)
 * using explicit type erasure rather than inheritance.
 *
 * This preserves:
 * - construction completeness (object is always valid)
 * - fixed memory layout (no heap allocation)
 * - uniform container type for the reactor
 * - explicit wiring in the System Controller
 *
 * Configuration represents a transition from one valid state
 * to another valid state. No constructor performs I/O.
 */
class SensorContext
{
public:
    static constexpr std::size_t SboSizeBytes = 256;

    /**
    * @brief Constructs an unconfigured SensorContext.
    *
    * The object is fully valid and destructible in this state.
    * It represents "no sensor installed" until explicitly configured.
    */
    SensorContext() noexcept;
    ~SensorContext();

    /**
    * @brief Non-copyable.
    *
    * SensorContext owns a unique operating system resource (file descriptor)
    * and associated reactor registration state. Copying would imply shared
    * ownership of that resource without duplicating the underlying descriptor,
    * leading to ambiguous lifetime and undefined shutdown behavior.
    *
    * SensorContext models an entity object, not a value.
    *
    * SensorContext therefore models unique ownership and is intentionally
    * non-copyable.
    */
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

    /**
    * @brief Configures a SensorContext as a UDP sensor.
    *
    * Transitions the context from a valid unconfigured state
    * to a valid UDP-configured state.
    *
    * No I/O occurs during construction; socket creation and
    * registration are performed explicitly during configuration.
    */
    friend void configure_udp_sensor(SensorContext& ctx,
                                     std::uint16_t port,
                                     moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                                     std::size_t maxMessageSize,
                                     SystemStats& sysStats) noexcept;

    friend const PortStats* udp_stats(const SensorContext& ctx) noexcept;
};
}
