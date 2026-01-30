// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace weather
{
    enum class ErrorCode : std::uint8_t
    {
        Ok = 0,

        // Framing / structural
        EmptyInput,
        MissingStartDelimiter,
        MissingChecksumDelimiter,
        MissingChecksumValue,
        InvalidChecksumValue,
        MissingIdentifier,
        InvalidIdentifierLength,

        // Insertion
        AlreadyFinalized,

        // Field access
        FieldMissing,
        FieldEmpty,

        // Conversions
        InvalidInteger,
        IntegerOutOfRange,
        InvalidDouble,
        InvalidChar
    };

    struct Status
    {
        ErrorCode code = ErrorCode::Ok;
        std::size_t fieldIndex = 0;   // which data field failed (0-based), when applicable
        std::string_view context{};   // optional view into the original sentence (for debugging)

        constexpr bool ok() const noexcept { return code == ErrorCode::Ok; }
        static constexpr Status Ok() noexcept { return { ErrorCode::Ok, 0, {} }; }
    };

} // namespace nmea
