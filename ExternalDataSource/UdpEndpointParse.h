// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software
#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

namespace weather
{

// Parses "host:port" into any endpoint type that has:
//   std::string host;
//   std::uint16_t port;
template <typename Endpoint>
inline bool parse_udp_endpoint(const std::string& s, Endpoint& ep) noexcept
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

} // namespace weather
