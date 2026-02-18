// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "cxxopts.hpp"

struct UdpEndpoint
{
    std::string host;
    std::uint16_t port{};
};

enum class GeneratorKind
{
    Constant,
    Sine,
    RandomWalk
};

struct Options
{
    // Exactly one of these should be set.
    std::optional<std::string> uart_device;
    std::optional<UdpEndpoint> udp;

    // Stream rates (Hz). 0 disables the stream.
    double temp_hz{0.0};
    double pressure_hz{0.0};
    double humidity_hz{0.0};
    double position_hz{0.0};
    double wind_hz{0.0};

    // Behavior
    GeneratorKind gen{GeneratorKind::RandomWalk};
    std::uint32_t seed{1};
    double duration_sec{0.0}; // 0 means run forever
    double log_every_sec{5.0};
};

static void print_usage(const cxxopts::Options& opts)
{
    std::cout << opts.help() << "\n";
}

static bool parse_udp_endpoint(std::string_view s, UdpEndpoint& out)
{
    // Expect "host:port"
    const auto pos = s.rfind(':');
    if (pos == std::string_view::npos)
    {
        return false;
    }

    const auto host = s.substr(0, pos);
    const auto port_str = s.substr(pos + 1);

    if (host.empty() || port_str.empty())
    {
        return false;
    }

    char* end = nullptr;
    const auto port_ul = std::strtoul(std::string(port_str).c_str(), &end, 10);
    if (end == nullptr || *end != '\0')
    {
        return false;
    }
    if (port_ul == 0 || port_ul > 65535)
    {
        return false;
    }

    out.host = std::string(host);
    out.port = static_cast<std::uint16_t>(port_ul);
    return true;
}

static bool parse_generator(std::string_view s, GeneratorKind& out)
{
    if (s == "constant")
    {
        out = GeneratorKind::Constant;
        return true;
    }
    if (s == "sine")
    {
        out = GeneratorKind::Sine;
        return true;
    }
    if (s == "random-walk")
    {
        out = GeneratorKind::RandomWalk;
        return true;
    }
    return false;
}

static bool validate_options(const Options& o, std::string& err)
{
    const bool has_uart = o.uart_device.has_value();
    const bool has_udp = o.udp.has_value();

    if (has_uart == has_udp)
    {
        err = "Specify exactly one of --uart or --udp.";
        return false;
    }

    auto rate_ok = [&](double hz, const char* name) -> bool
    {
        if (hz < 0.0)
        {
            err = std::string("Rate must be >= 0: ") + name;
            return false;
        }
        return true;
    };

    if (!rate_ok(o.temp_hz, "--temp-hz")) return false;
    if (!rate_ok(o.pressure_hz, "--pressure-hz")) return false;
    if (!rate_ok(o.humidity_hz, "--humidity-hz")) return false;
    if (!rate_ok(o.position_hz, "--position-hz")) return false;
    if (!rate_ok(o.wind_hz, "--wind-hz")) return false;

    if (o.duration_sec < 0.0)
    {
        err = "--duration must be >= 0.";
        return false;
    }

    if (o.log_every_sec <= 0.0)
    {
        err = "--log-every must be > 0.";
        return false;
    }

    const bool any_stream =
        (o.temp_hz > 0.0) ||
        (o.pressure_hz > 0.0) ||
        (o.humidity_hz > 0.0) ||
        (o.position_hz > 0.0) ||
        (o.wind_hz > 0.0);

    if (!any_stream)
    {
        err = "No streams enabled. Set at least one of --temp-hz, --pressure-hz, --humidity-hz, --position-hz, --wind-hz.";
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    Options out{};

    try
    {
        cxxopts::Options opts("weather_tx", "Synthetic sensor transmitter for Chapter 11 external-input testing");

        // Temporary locals for parsing.
        std::string uart;
        std::string udp;
        std::string gen;

        opts.add_options()
            ("h,help", "Print usage")
            ("uart", "UART device (e.g. /dev/pts/7)", cxxopts::value<std::string>(uart))
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
        }

        if (result.count("udp") != 0)
        {
            UdpEndpoint ep{};
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

        // Next step:
        // - Construct OutputSink (UART or UDP)
        // - Construct per-stream generators
        // - Run scheduler loop until duration or SIGINT

        std::cout << "Configured transmitter.\n";
        if (out.uart_device.has_value())
        {
            std::cout << "Output: UART " << *out.uart_device << "\n";
        }
        else
        {
            std::cout << "Output: UDP " << out.udp->host << ":" << out.udp->port << "\n";
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Argument parsing failed: " << e.what() << "\n";
        return 2;
    }
}

