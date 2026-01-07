// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace nmea
{
    // Incremental framer for stream transports (UART/TCP).
    // Feed bytes, and pop complete sentences (without trailing CR/LF).
    class SentenceFramer
    {
    public:
        void push(std::string_view bytes);

        // Returns a complete sentence if one is available (without trailing CR/LF).
        std::optional<std::string> pop();

        void reset();

    private:
        std::string m_buf;
    };
} // namespace nmea
