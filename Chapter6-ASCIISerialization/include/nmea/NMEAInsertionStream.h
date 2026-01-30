// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "nmea/NMEAStatus.h"

namespace weather
{
    // Builds an NMEA sentence by appending comma-separated fields and a checksum.
    //
    // - Starts with "$" + identifier (exactly 5 chars: talker(2)+type(3))
    // - Each write call appends one field (including empty fields)
    // - finalize() appends "*HH" and optional CRLF
    class InsertionStream
    {
    public:
        explicit InsertionStream(std::string_view identifier);

        Status status() const noexcept;

        Status writeString(std::string_view v);
        Status writeChar(char v);
        Status writeInt(int v);
        Status writeDouble(double v);

        Status writeOptionalString(const std::optional<std::string_view>& v);
        Status writeOptionalChar(const std::optional<char>& v);
        Status writeOptionalInt(const std::optional<int>& v);
        Status writeOptionalDouble(const std::optional<double>& v);

        Status writeEmpty();

        Status finalize(bool appendCRLF = true);

        bool finalized() const noexcept;
        const std::string& sentence() const noexcept;

    private:
        void appendFieldPrefix();
        void setError(ErrorCode code) noexcept;

        static std::uint8_t computeXor(std::string_view betweenDollarAndStar) noexcept;
        static void appendChecksum(std::string& out, std::uint8_t checksum);

        Status m_status = Status::Ok();
        std::string m_sentence;
        bool m_finalized = false;
    };
} // namespace nmea
