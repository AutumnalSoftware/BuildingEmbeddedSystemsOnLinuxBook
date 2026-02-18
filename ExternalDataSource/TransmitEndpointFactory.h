// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Autumnal Software
#pragma once

#include "ITransmitEndpoint.h"
#include "UdpTransmitEndpoint.h"
#include "UartTransmitEndpoint.h"

#include <memory>
#include <string>

namespace weather
{

inline std::unique_ptr<ITransmitEndpoint> create_udp_transmitter(const std::string& host, int port)
{
    auto ep = std::make_unique<UdpTransmitEndpoint>(host, port);
    if (!ep->open())
    {
        return {};
    }
    return ep;
}

inline std::unique_ptr<ITransmitEndpoint> create_uart_transmitter(const std::string& device, int baud)
{
    auto ep = std::make_unique<UartTransmitEndpoint>(device, baud);
    if (!ep->open())
    {
        return {};
    }
    return ep;
}

}
