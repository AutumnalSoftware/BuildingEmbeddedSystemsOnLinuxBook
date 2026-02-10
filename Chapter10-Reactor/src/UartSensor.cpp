#include "UartSensor.h"
#include "SensorContext.h"

#include <array>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace weather
{
static speed_t to_speed(int baud) noexcept
{
    switch (baud)
    {
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: return B9600;
    }
}

struct UartNmeaImpl
{
    int fd = -1;
    std::size_t maxLineBytes = 128;

    moodycamel::ReaderWriterQueue<AnyMeasurement, 128>* outQ = nullptr;
    PortStats stats{};

    std::array<char, 128> line{};
    std::size_t lineLen = 0;

    UartNmeaImpl(const char* devicePath,
                 int baud,
                 moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& q,
                 std::size_t maxLine) noexcept
        : maxLineBytes(maxLine)
        , outQ(&q)
    {
        assert(maxLineBytes <= line.size());

        fd = ::open(devicePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) return;

        termios tio{};
        if (::tcgetattr(fd, &tio) == 0)
        {
            ::cfmakeraw(&tio);
            tio.c_cflag |= (CLOCAL | CREAD);
            tio.c_cc[VMIN] = 0;
            tio.c_cc[VTIME] = 0;

            const speed_t sp = to_speed(baud);
            ::cfsetispeed(&tio, sp);
            ::cfsetospeed(&tio, sp);

            (void)::tcsetattr(fd, TCSANOW, &tio);
        }
    }

    ~UartNmeaImpl()
    {
        if (fd >= 0)
        {
            ::close(fd);
            fd = -1;
        }
    }

    int get_fd() const noexcept { return fd; }

    void reset_line() noexcept { lineLen = 0; }

    bool accept_byte(char c) noexcept
    {
        if (lineLen >= maxLineBytes)
        {
            stats.frames_dropped++;
            reset_line();
            return false;
        }

        line[lineLen++] = c;

        if (c == '\n')
        {
            if (lineLen >= 2 && line[lineLen - 2] == '\r')
            {
                line[lineLen - 2] = '\n';
                lineLen -= 1;
            }
            return true;
        }

        return false;
    }

    Status on_readable() noexcept
    {
        assert(outQ);

        std::array<char, 256> rx{};

        for (;;)
        {
            const ssize_t n = ::read(fd, rx.data(), rx.size());
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return Status{};
                stats.frames_dropped++;
                return Status{};
            }
            if (n == 0)
            {
                return Status{};
            }

            for (ssize_t i = 0; i < n; ++i)
            {
                const char c = rx[static_cast<std::size_t>(i)];
                const bool complete = accept_byte(c);
                if (!complete) continue;

                stats.frames_received++;

                // Chapter 10 stub measurement per completed line.
                MeasurementHeaderV1 h{};
                Temperature t{};
                AnyMeasurement m(h, t);

                if (!outQ->try_enqueue(std::move(m)))
                {
                    stats.frames_dropped++;
                }

                reset_line();
            }
        }
    }
};

static void uart_destroy(void* p) noexcept { reinterpret_cast<UartNmeaImpl*>(p)->~UartNmeaImpl(); }
static void uart_move(void*, void*) noexcept { assert(false && "UartNmeaImpl move should not occur"); }
static int uart_fd(const void* p) noexcept { return reinterpret_cast<const UartNmeaImpl*>(p)->get_fd(); }
static Status uart_read(void* p) noexcept { return reinterpret_cast<UartNmeaImpl*>(p)->on_readable(); }

static const SensorContext::Ops& UartOps() noexcept
{
    static const SensorContext::Ops ops{
        &uart_destroy,
        &uart_move,
        &uart_fd,
        &uart_read
    };
    return ops;
}

void configure_uart_nmea_sensor(SensorContext& ctx,
                                const char* devicePath,
                                int baud,
                                moodycamel::ReaderWriterQueue<AnyMeasurement, 128>& outQ,
                                std::size_t maxLineBytes) noexcept
{
    ctx.destroy_to_unconfigured();
    ctx.mOps = &UartOps();

    static_assert(sizeof(UartNmeaImpl) <= SensorContext::SboSizeBytes,
                  "UartNmeaImpl does not fit in SensorContext SBO");

    new (ctx.mStorage.bytes) UartNmeaImpl(devicePath, baud, outQ, maxLineBytes);

    assert(ctx.fd() >= 0 && "UART open failed");
}

const PortStats* uart_stats(const SensorContext& ctx) noexcept
{
    if (ctx.mOps != &UartOps()) return nullptr;
    return &reinterpret_cast<const UartNmeaImpl*>(ctx.mStorage.bytes)->stats;
}
}
