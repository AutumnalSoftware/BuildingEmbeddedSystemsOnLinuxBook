#include "UdpSensor.h"
#include "SensorContext.h"
#include <cassert>
#include <array>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace weather
{
static constexpr std::size_t MaxUdpDatagramBytes = 2048;

struct UdpSensorImpl
{
    int fd = -1;
    std::size_t maxMessageSize = 0;
    moodycamel::ReaderWriterQueue<AnyMeasurement, 128>* outQ = nullptr;
    PortStats stats{};

    UdpSensorImpl(std::uint16_t port,
                  moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& q,
                  std::size_t maxMsg) noexcept
        : maxMessageSize(maxMsg), outQ(&q)
    {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        bind(fd, (sockaddr*)&addr, sizeof(addr));
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }

    ~UdpSensorImpl()
    {
        if (fd >= 0) close(fd);
    }

    int get_fd() const noexcept { return fd; }

    Status on_readable() noexcept
    {
        for (;;)
        {
            std::array<std::byte, MaxUdpDatagramBytes> rx{};
            ssize_t n = recv(fd, rx.data(), rx.size(), 0);
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return Status::Ok();
                stats.frames_dropped++;
                return Status::Ok();
            }

            stats.frames_received++;
            if ((std::size_t)n > maxMessageSize)
            {
                stats.frames_dropped++;
                continue;
            }

            MeasurementHeaderV1 h{};
            Temperature t{};
            AnyMeasurement m(h, t);
            outQ->try_enqueue(std::move(m));
        }
    }
};

static void udp_destroy(void* p) noexcept { reinterpret_cast<UdpSensorImpl*>(p)->~UdpSensorImpl(); }
static void udp_move(void*, void*) noexcept { assert(false); }
static int udp_fd(const void* p) noexcept { return reinterpret_cast<const UdpSensorImpl*>(p)->get_fd(); }
static Status udp_read(void* p) noexcept { return reinterpret_cast<UdpSensorImpl*>(p)->on_readable(); }

static const SensorContext::Ops& UdpOps() noexcept
{
    static const SensorContext::Ops ops{
        &udp_destroy, &udp_move, &udp_fd, &udp_read
    };
    return ops;
}

void configure_udp_sensor(SensorContext& ctx,
                          std::uint16_t port,
                          moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                          std::size_t maxMessageSize) noexcept
{
    ctx.destroy_to_unconfigured();
    ctx.mOps = &UdpOps();
    new (ctx.mStorage.bytes) UdpSensorImpl(port, outQ, maxMessageSize);
}

const PortStats* udp_stats(const SensorContext& ctx) noexcept
{
    if (ctx.mOps != &UdpOps()) return nullptr;
    return &reinterpret_cast<const UdpSensorImpl*>(ctx.mStorage.bytes)->stats;
}
}
