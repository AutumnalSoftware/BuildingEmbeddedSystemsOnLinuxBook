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

#include "BinaryReadStream.h"
#include "BdsMeasurementCodecs.h"
#include "MeasurementHeaderV1.h"
#include "MeasurementTypes.h"
#include "MutableByteView.h"

namespace weather
{
struct UdpBind
{
    std::string host;
    std::uint16_t port = 0;
};

static void print_usage(const cxxopts::Options& opts)
{
    std::cout << opts.help() << "\n";
}

static bool parse_udp_endpoint(const std::string& s, UdpBind& ep) noexcept
{
    const auto pos = s.find(':');
    if (pos == std::string::npos)
    {
        return false;
    }

    const std::string host = s.substr(0, pos);
    const std::string portStr = s.substr(pos + 1);

    if (host.empty() || portStr.empty())
    {
        return false;
    }

    char* end = nullptr;
    const long portLong = std::strtol(portStr.c_str(), &end, 10);
    if (end == nullptr || *end != '\0')
    {
        return false;
    }
    if (portLong <= 0 || portLong > 65535)
    {
        return false;
    }

    ep.host = host;
    ep.port = static_cast<std::uint16_t>(portLong);
    return true;
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
        std::size_t max_packet = 4096;

        opts.add_options()
            ("h,help", "Print usage")
            ("udp", "UDP bind host:port (e.g. 0.0.0.0:9000)", cxxopts::value<std::string>(udp)->default_value("0.0.0.0:9000"))
            ("max-packet", "Max UDP packet bytes", cxxopts::value<std::size_t>(max_packet)->default_value("4096"))
            ;

        const auto result = opts.parse(argc, argv);

        if (result.count("help") != 0)
        {
            weather::print_usage(opts);
            return 0;
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
