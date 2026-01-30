// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "nmea/NMEAStatus.h"
#include "nmea/NMEATokenizer.h"

namespace weather
{
    // Sequential typed extraction over tokenized NMEA data fields.
    //
    // Required reads fail if:
    //  - the field is missing (sentence has too few fields), OR
    //  - the field exists but is empty.
    //
    // Optional reads fail only if the field is missing. If the field is empty,
    // the optional is cleared and Status::Ok is returned.
    class ExtractionStream
    {
    public:
        explicit ExtractionStream(const Tokenizer& tokens);

        std::size_t index() const noexcept;
        bool atEnd() const noexcept;
        Status lastStatus() const noexcept;

        Status skipField();

        Status readString(std::string_view& out);
        Status readChar(char& out);
        Status readInt(int& out);
        Status readDouble(double& out);

        Status readOptionalString(std::optional<std::string_view>& out);
        Status readOptionalChar(std::optional<char>& out);
        Status readOptionalInt(std::optional<int>& out);
        Status readOptionalDouble(std::optional<double>& out);

    private:
        Status requireField(std::string_view& out);
        Status optionalField(std::optional<std::string_view>& out);

        static Status parseInt(std::string_view s, int& out, std::size_t fieldIndex, std::string_view ctx) noexcept;
        static Status parseDouble(std::string_view s, double& out, std::size_t fieldIndex, std::string_view ctx) noexcept;

        const Tokenizer& m_tokens;
        Tokenizer::FieldIterator m_it;
        Tokenizer::FieldIterator m_end;
        std::size_t m_index = 0;
        Status m_last = Status::Ok();
    };
} // namespace nmea
