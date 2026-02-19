// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software

#include <cxxopts.hpp>

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "TransmitterApp.h"
#include "UartTransmitEndpoint.h"
#include "UdpTransmitEndpoint.h"
#include "TransmitEndpointFactory.h"
#include "UdpEndpointParse.h"
#include "ValueGenerator.h"
#include "Options.h"



static void print_usage(const cxxopts::Options& opts)
{
    std::cout << opts.help() << "\n";
}

static bool parse_generator(std::string_view s, weather::GeneratorKind& out) noexcept
{
    if (s == "constant")
    {
        out = weather::GeneratorKind::Constant;
        return true;
    }
    if (s == "sine")
    {
        out = weather::GeneratorKind::Sine;
        return true;
    }
    if (s == "random-walk")
    {
        out = weather::GeneratorKind::RandomWalk;
        return true;
    }
    return false;
}

static bool validate_options(const weather::Options& opt, std::string& err)
{
    const bool hasUart = opt.uart_device.has_value();
    const bool hasUdp = opt.udp.has_value();

    if (hasUart == hasUdp)
    {
        err = "Must specify exactly one output:\n"
              "  --uart <device> [--baud <rate>]\n"
              "or\n"
              "  --udp <host:port>\n";
        return false;
    }

    if (hasUart)
    {
        switch (opt.uart_baud)
        {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
            break;
        default:
            err = "Invalid --baud. Allowed: 9600, 19200, 38400, 57600, 115200";
            return false;
        }
    }

    if (opt.temp_hz < 0.0 || opt.pressure_hz < 0.0 || opt.humidity_hz < 0.0 ||
        opt.position_hz < 0.0 || opt.wind_hz < 0.0)
    {
        err = "Stream rates must be >= 0 (0 disables the stream).";
        return false;
    }

    if (opt.duration_sec < 0.0)
    {
        err = "--duration must be >= 0 (0 runs forever).";
        return false;
    }

    if (opt.log_every_sec <= 0.0)
    {
        err = "--log-every must be > 0.";
        return false;
    }

    const bool anyEnabled =
        (opt.temp_hz > 0.0) ||
        (opt.pressure_hz > 0.0) ||
        (opt.humidity_hz > 0.0) ||
        (opt.position_hz > 0.0) ||
        (opt.wind_hz > 0.0);

    if (!anyEnabled)
    {
        err = "No streams enabled. Set at least one of: "
              "--temp-hz, --pressure-hz, --humidity-hz, --position-hz, --wind-hz";
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    weather::Options out{};

    try
    {
        cxxopts::Options opts("weather_tx", "Synthetic sensor transmitter for Chapter 11 external-input testing");

        // Temporary locals for parsing.
        std::string uart;
        std::string udp;
        std::string gen;
        int baud = 115200;

        opts.add_options()
            ("h,help", "Print usage")
            ("uart", "UART device (e.g. /dev/pts/7)", cxxopts::value<std::string>(uart))
            ("baud", "UART baud rate (default 115200)", cxxopts::value<int>(baud)->default_value("115200"))
            ("udp", "UDP endpoint host:port (e.g. 127.0.0.1:9000)", cxxopts::value<std::string>(udp))
            ("temp-hz", "Temperature stream rate (Hz, 0 disables)", cxxopts::value<double>(out.temp_hz)->default_value("0"))
            ("pressure-hz", "Pressure stream rate (Hz, 0 disables)", cxxopts::value<double>(out.pressure_hz)->default_value("0"))
            ("humidity-hz", "Humidity stream rate (Hz, 0 disables)", cxxopts::value<double>(out.humidity_hz)->default_value("0"))
            ("position-hz", "Position stream rate (Hz, 0 disables)", cxxopts::value<double>(out.position_hz)->default_value("0"))
            ("wind-hz", "Wind stream rate (Hz, 0 disables)", cxxopts::value<double>(out.wind_hz)->default_value("0"))
            ("gen", "Generator: constant|sine|random-walk", cxxopts::value<std::string>(gen)->default_value("random-walk"))
            ("seed", "PRNG seed", cxxopts::value<std::uint32_t>(out.seed)->default_value("1"))
            ("duration", "Run duration in seconds (0 runs forever)", cxxopts::value<double>(out.duration_sec)->default_value("0"))
            ("log-every", "Log summary period in seconds", cxxopts::value<double>(out.log_every_sec)->default_value("5"))
            ;

        const auto result = opts.parse(argc, argv);

        if (result.count("help") != 0)
        {
            print_usage(opts);
            return 0;
        }

        if (result.count("uart") != 0)
        {
            out.uart_device = uart;
            out.uart_baud = baud;
        }

        if (result.count("udp") != 0)
        {
            weather::UdpEndpoint ep{};
            if (!parse_udp_endpoint(udp, ep))
            {
                std::cerr << "Invalid --udp endpoint. Expected host:port\n";
                return 2;
            }
            out.udp = ep;
        }

        if (!parse_generator(gen, out.gen))
        {
            std::cerr << "Invalid --gen value: " << gen << "\n";
            std::cerr << "Allowed: constant, sine, random-walk\n";
            return 2;
        }

        std::string err;
        if (!validate_options(out, err))
        {
            std::cerr << err << "\n\n";
            print_usage(opts);
            return 2;
        }

        std::cout << "Configured transmitter.\n";
        if (out.uart_device.has_value())
        {
            std::cout << "Output: UART " << *out.uart_device
                      << " @ " << out.uart_baud << " baud\n";
            std::cout << "Note: when using PTYs via socat, baud rate may not affect throughput.\n";
        }
        else
        {
            std::cout << "Output: UDP " << out.udp->host << ":" << out.udp->port << "\n";
        }

        // Next step:
        // - Construct OutputSink (UART or UDP)
        // - Construct per-stream generators
        // - Run scheduler loop until duration or SIGINT

        std::unique_ptr<weather::ITransmitEndpoint> sink;

        if (out.uart_device.has_value())
        {
            sink = weather::create_uart_transmitter(*out.uart_device, out.uart_baud);
        }
        else
        {
            sink = weather::create_udp_transmitter(out.udp->host, out.udp->port);
        }

        if (!sink)
        {
            std::cerr << "Failed to create transmitter\n";
            return 2;
        }

        weather::TransmitterApp app(out, std::move(sink));
        return app.run();

    }
    catch (const std::exception& e)
    {
        std::cerr << "Argument parsing failed: " << e.what() << "\n";
        return 2;
    }
}
