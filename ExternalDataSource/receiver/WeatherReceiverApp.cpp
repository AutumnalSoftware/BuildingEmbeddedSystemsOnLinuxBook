// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#include <cxxopts.hpp>

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fcntl.h>
#include <termios.h>

#include "BinaryReadStream.h"
#include "BdsMeasurementCodecs.h"
#include "MeasurementHeaderV1.h"
#include "MeasurementTypes.h"
#include "MutableByteView.h"
#include "UdpEndpointParse.h"

namespace weather
{
struct UdpBind
{
    std::string host;
    std::uint16_t port = 0;
};
struct UartBind
{
    std::string device;
    int baud = 115200;
};


static void print_usage(const cxxopts::Options& opts)
{
    std::cout << opts.help() << "\n";
}

static void print_header(const MeasurementHeaderV1& h)
{
    std::cout << "rxTime=" << h.rxTime
              << " eventTime=" << h.eventTime
              << " kind=" << static_cast<std::uint16_t>(h.kind)
              << " source=" << static_cast<std::uint16_t>(h.source)
              << " flags=0x" << std::hex << h.flags << std::dec;
}

static bool decode_and_print(const std::byte* data, std::size_t size)
{
    MutableByteView buf(const_cast<std::byte*>(data), size);
    BinaryReadStream bs(buf, Endianness::Little);

    MeasurementHeaderV1 h{};
    readMeasurementHeader(bs, h);

    if (!bs.ok())
    {
        std::cerr << "decode: failed reading MeasurementHeaderV1\n";
        return false;
    }

    std::cout << "[MEAS] ";
    print_header(h);
    std::cout << " ";

    switch (h.kind)
    {
    case MeasurementKind::Temperature:
    {
        Temperature m{};
        readTemperature(bs, m);
        if (!bs.ok())
        {
            std::cerr << "decode: failed reading Temperature\n";
            return false;
        }
        std::cout << "Temperature.value=" << m.value << "\n";
        return true;
    }

    case MeasurementKind::BarometricPressure:
    {
        BarometricPressure m{};
        readBarometricPressure(bs, m);
        if (!bs.ok())
        {
            std::cerr << "decode: failed reading BarometricPressure\n";
            return false;
        }
        std::cout << "BarometricPressure.value=" << m.value << "\n";
        return true;
    }

    case MeasurementKind::Humidity:
    {
        Humidity m{};
        readHumidity(bs, m);
        if (!bs.ok())
        {
            std::cerr << "decode: failed reading Humidity\n";
            return false;
        }
        std::cout << "Humidity.value=" << m.value << "\n";
        return true;
    }

    case MeasurementKind::WindSpeed:
    {
        WindSpeed m{};
        readWindSpeed(bs, m);
        if (!bs.ok())
        {
            std::cerr << "decode: failed reading WindSpeed\n";
            return false;
        }
        std::cout << "WindSpeed.value=" << m.value << "\n";
        return true;
    }

    case MeasurementKind::WindDirection:
    {
        WindDirection m{};
        readWindDirection(bs, m);
        if (!bs.ok())
        {
            std::cerr << "decode: failed reading WindDirection\n";
            return false;
        }
        std::cout << "WindDirection.value=" << m.value << "\n";
        return true;
    }

    case MeasurementKind::Precipitation:
    {
        Precipitation m{};
        readPrecipitation(bs, m);
        if (!bs.ok())
        {
            std::cerr << "decode: failed reading Precipitation\n";
            return false;
        }
        std::cout << "Precipitation.value=" << m.value << "\n";
        return true;
    }

    case MeasurementKind::Position:
    {
        Position m{};
        readPosition(bs, m);
        if (!bs.ok())
        {
            std::cerr << "decode: failed reading Position\n";
            return false;
        }
        std::cout << "Position.lat=" << m.lat
                  << " lon=" << m.lon
                  << " alt=" << m.alt << "\n";
        return true;
    }

    case MeasurementKind::Empty:
    default:
        std::cout << "(unhandled kind)\n";
        return true;
    }
}

static speed_t to_speed(int baud) noexcept
{
    switch (baud)
    {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: return 0;
    }
}

static int open_uart_rx(const UartBind& b)
{
    const int fd = ::open(b.device.c_str(), O_RDONLY | O_NOCTTY);
    if (fd < 0)
    {
        std::cerr << "uart: open() failed: " << b.device << "\n";
        return -1;
    }

    termios tty{};
    if (tcgetattr(fd, &tty) != 0)
    {
        std::cerr << "uart: tcgetattr() failed\n";
        ::close(fd);
        return -1;
    }

    cfmakeraw(&tty);

    const speed_t sp = to_speed(b.baud);
    if (sp == 0)
    {
        std::cerr << "uart: unsupported baud: " << b.baud << "\n";
        ::close(fd);
        return -1;
    }

    cfsetispeed(&tty, sp);
    cfsetospeed(&tty, sp);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        std::cerr << "uart: tcsetattr() failed\n";
        ::close(fd);
        return -1;
    }

    return fd;
}

static std::size_t payload_size_bytes(MeasurementKind k) noexcept
{
    switch (k)
    {
    case MeasurementKind::Temperature:
    case MeasurementKind::BarometricPressure:
    case MeasurementKind::Humidity:
    case MeasurementKind::WindSpeed:
    case MeasurementKind::WindDirection:
    case MeasurementKind::Precipitation:
        return sizeof(double);

    case MeasurementKind::Position:
        return sizeof(double) * 3;

    case MeasurementKind::Empty:
    default:
        return 0;
    }
}

static bool try_decode_uart_record(std::vector<std::byte>& accum)
{
    // MeasurementHeaderV1 wire layout (BDS, little-endian):
    //   rxTime   : uint64 (8)
    //   eventTime: uint64 (8)
    //   kind     : uint16 (2)
    //   source   : uint16 (2)
    //   flags    : uint32 (4)
    static constexpr std::size_t HeaderBytes = 8 + 8 + 2 + 2 + 4;

    if (accum.size() < HeaderBytes)
    {
        return false;
    }

    // Peek kind (little-endian) at offset 16.
    const std::uint16_t kind_u =
        static_cast<std::uint16_t>(std::uint8_t(accum[16])) |
        (static_cast<std::uint16_t>(std::uint8_t(accum[17])) << 8);

    const auto kind = static_cast<MeasurementKind>(kind_u);
    const std::size_t total = HeaderBytes + payload_size_bytes(kind);

    if (accum.size() < total)
    {
        return false;
    }

    // We have a complete record.
    decode_and_print(accum.data(), total);

    // Consume.
    accum.erase(accum.begin(), accum.begin() + static_cast<std::ptrdiff_t>(total));
    return true;
}

static int run_uart_rx(const UartBind& b, std::size_t max_buffer)
{
    const int fd = open_uart_rx(b);
    if (fd < 0)
    {
        return 2;
    }

    std::vector<std::byte> accum;
    accum.reserve(max_buffer);

    std::vector<std::byte> tmp(512);

    std::cout << "weather_rx listening on UART " << b.device << " (baud " << b.baud << ")\n";

    while (true)
    {
        const auto n = ::read(fd, tmp.data(), tmp.size());
        if (n < 0)
        {
            std::cerr << "uart: read() failed\n";
            ::close(fd);
            return 2;
        }

        if (n == 0)
        {
            continue;
        }

        const std::size_t nn = static_cast<std::size_t>(n);

        if (accum.size() + nn > max_buffer)
        {
            // Drop accumulated data on overflow. This is a debug tool.
            accum.clear();
        }

        accum.insert(accum.end(), tmp.begin(), tmp.begin() + static_cast<std::ptrdiff_t>(nn));

        while (try_decode_uart_record(accum))
        {
            // keep draining
        }
    }
}

static int run_udp_rx(const UdpBind& bind_ep, std::size_t max_packet)
{
    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        std::cerr << "udp: socket() failed\n";
        return 2;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(bind_ep.port);

    if (::inet_pton(AF_INET, bind_ep.host.c_str(), &addr.sin_addr) != 1)
    {
        std::cerr << "udp: invalid bind address: " << bind_ep.host << "\n";
        ::close(fd);
        return 2;
    }

    if (::bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        std::cerr << "udp: bind() failed on " << bind_ep.host << ":" << bind_ep.port << "\n";
        ::close(fd);
        return 2;
    }

    std::vector<std::byte> buf(max_packet);

    std::cout << "weather_rx listening on UDP " << bind_ep.host << ":" << bind_ep.port << "\n";

    while (true)
    {
        const auto n = ::recvfrom(fd, buf.data(), buf.size(), 0, nullptr, nullptr);
        if (n < 0)
        {
            std::cerr << "udp: recvfrom() failed\n";
            ::close(fd);
            return 2;
        }

        if (n == 0)
        {
            continue;
        }

        decode_and_print(buf.data(), static_cast<std::size_t>(n));
    }
}

} // namespace weather

int main(int argc, char** argv)
{
    try
    {
        cxxopts::Options opts("weather_rx", "Minimal UDP receiver: decodes MeasurementHeaderV1 + typed payload (BDS)");

        std::string udp;
        std::string uart;
        int baud = 115200;
        std::size_t max_packet = 4096;
        std::size_t max_buffer = 16384;

        opts.add_options()
            ("h,help", "Print usage")
            ("udp", "UDP bind host:port (e.g. 0.0.0.0:9000)", cxxopts::value<std::string>(udp))
            ("uart", "UART device (e.g. /dev/pts/6)", cxxopts::value<std::string>(uart))
            ("baud", "UART baud rate", cxxopts::value<int>(baud)->default_value("115200"))
            ("max-packet", "Max UDP packet bytes", cxxopts::value<std::size_t>(max_packet)->default_value("4096"))
            ("max-buffer", "Max UART accumulate buffer bytes", cxxopts::value<std::size_t>(max_buffer)->default_value("16384"))
            ;

        const auto result = opts.parse(argc, argv);

        if (result.count("help") != 0)
        {
            weather::print_usage(opts);
            return 0;
        }

        if ((result.count("udp") != 0) && (result.count("uart") != 0))
        {
            std::cerr << "Specify only one of --udp or --uart\n";
            return 2;
        }

        if (result.count("uart") != 0)
        {
            weather::UartBind b{};
            b.device = uart;
            b.baud = baud;
            return weather::run_uart_rx(b, max_buffer);
        }

        // Default: UDP (if not specified, bind to 0.0.0.0:9000)
        if (udp.empty())
        {
            udp = "0.0.0.0:9000";
        }

        weather::UdpBind ep{};
        if (!weather::parse_udp_endpoint(udp, ep))
        {
            std::cerr << "Invalid --udp. Expected host:port\n";
            return 2;
        }

        return weather::run_udp_rx(ep, max_packet);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Argument parsing failed: " << e.what() << "\n";
        return 2;
    }
}
