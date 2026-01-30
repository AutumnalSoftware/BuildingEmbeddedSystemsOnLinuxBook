// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#include "nmea/NMEAExtractionStream.h"

#include <charconv>
#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace weather
{
    ExtractionStream::ExtractionStream(const Tokenizer& tokens)
        : m_tokens(tokens)
        , m_it(tokens.begin())
        , m_end(tokens.end())
    {
    }

    std::size_t ExtractionStream::index() const noexcept { return m_index; }
    bool ExtractionStream::atEnd() const noexcept { return m_it == m_end; }
    Status ExtractionStream::lastStatus() const noexcept { return m_last; }

    Status ExtractionStream::skipField()
    {
        if (m_it == m_end)
        {
            m_last = { ErrorCode::FieldMissing, m_index, m_tokens.sentence() };
            return m_last;
        }
        ++m_it;
        ++m_index;
        m_last = Status::Ok();
        return m_last;
    }

    Status ExtractionStream::requireField(std::string_view& out)
    {
        if (m_it == m_end)
        {
            m_last = { ErrorCode::FieldMissing, m_index, m_tokens.sentence() };
            return m_last;
        }

        out = *m_it;
        if (out.empty())
        {
            m_last = { ErrorCode::FieldEmpty, m_index, m_tokens.sentence() };
            return m_last;
        }

        ++m_it;
        ++m_index;

        m_last = Status::Ok();
        return m_last;
    }

    Status ExtractionStream::optionalField(std::optional<std::string_view>& out)
    {
        if (m_it == m_end)
        {
            m_last = { ErrorCode::FieldMissing, m_index, m_tokens.sentence() };
            return m_last;
        }

        const std::string_view v = *m_it;
        ++m_it;
        ++m_index;

        if (v.empty())
        {
            out.reset();
            m_last = Status::Ok();
            return m_last;
        }

        out = v;
        m_last = Status::Ok();
        return m_last;
    }

    Status ExtractionStream::readString(std::string_view& out) { return requireField(out); }

    Status ExtractionStream::readChar(char& out)
    {
        std::string_view s;
        Status st = requireField(s);
        if (!st.ok()) return st;

        if (s.size() != 1)
        {
            m_last = { ErrorCode::InvalidChar, st.fieldIndex, m_tokens.sentence() };
            return m_last;
        }

        out = s[0];
        m_last = Status::Ok();
        return m_last;
    }

    Status ExtractionStream::parseInt(std::string_view s, int& out, std::size_t fieldIndex, std::string_view ctx) noexcept
    {
        int value = 0;
        auto res = std::from_chars(s.data(), s.data() + s.size(), value);
        if (res.ec == std::errc::invalid_argument || res.ptr != s.data() + s.size())
        {
            return { ErrorCode::InvalidInteger, fieldIndex, ctx };
        }
        if (res.ec == std::errc::result_out_of_range)
        {
            return { ErrorCode::IntegerOutOfRange, fieldIndex, ctx };
        }
        out = value;
        return Status::Ok();
    }

    Status ExtractionStream::readInt(int& out)
    {
        std::string_view s;
        Status st = requireField(s);
        if (!st.ok()) return st;

        m_last = parseInt(s, out, st.fieldIndex, m_tokens.sentence());
        return m_last;
    }

    Status ExtractionStream::parseDouble(std::string_view s, double& out, std::size_t fieldIndex, std::string_view ctx) noexcept
    {
#if defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L)
        {
            double value = 0.0;
            auto res = std::from_chars(s.data(), s.data() + s.size(), value);
            if (res.ec == std::errc() && res.ptr == s.data() + s.size() && std::isfinite(value))
            {
                out = value;
                return Status::Ok();
            }
        }
#endif
        if (s.size() > 63)
        {
            return { ErrorCode::InvalidDouble, fieldIndex, ctx };
        }

        char buf[64];
        for (std::size_t i = 0; i < s.size(); ++i) buf[i] = s[i];
        buf[s.size()] = '\0';

        char* parseEnd = nullptr;
        errno = 0;
        const double value = std::strtod(buf, &parseEnd);

        if (errno != 0 || parseEnd == buf) return { ErrorCode::InvalidDouble, fieldIndex, ctx };
        if (static_cast<std::size_t>(parseEnd - buf) != s.size()) return { ErrorCode::InvalidDouble, fieldIndex, ctx };
        if (!std::isfinite(value)) return { ErrorCode::InvalidDouble, fieldIndex, ctx };

        out = value;
        return Status::Ok();
    }

    Status ExtractionStream::readDouble(double& out)
    {
        std::string_view s;
        Status st = requireField(s);
        if (!st.ok()) return st;

        m_last = parseDouble(s, out, st.fieldIndex, m_tokens.sentence());
        return m_last;
    }

    Status ExtractionStream::readOptionalString(std::optional<std::string_view>& out) { return optionalField(out); }

    Status ExtractionStream::readOptionalChar(std::optional<char>& out)
    {
        std::optional<std::string_view> s;
        Status st = optionalField(s);
        if (!st.ok()) return st;

        if (!s.has_value())
        {
            out.reset();
            m_last = Status::Ok();
            return m_last;
        }

        if (s->size() != 1)
        {
            m_last = { ErrorCode::InvalidChar, (m_index > 0 ? m_index - 1 : 0), m_tokens.sentence() };
            return m_last;
        }

        out = (*s)[0];
        m_last = Status::Ok();
        return m_last;
    }

    Status ExtractionStream::readOptionalInt(std::optional<int>& out)
    {
        std::optional<std::string_view> s;
        Status st = optionalField(s);
        if (!st.ok()) return st;

        if (!s.has_value())
        {
            out.reset();
            m_last = Status::Ok();
            return m_last;
        }

        int value = 0;
        Status conv = parseInt(*s, value, (m_index > 0 ? m_index - 1 : 0), m_tokens.sentence());
        if (!conv.ok())
        {
            m_last = conv;
            return m_last;
        }

        out = value;
        m_last = Status::Ok();
        return m_last;
    }

    Status ExtractionStream::readOptionalDouble(std::optional<double>& out)
    {
        std::optional<std::string_view> s;
        Status st = optionalField(s);
        if (!st.ok()) return st;

        if (!s.has_value())
        {
            out.reset();
            m_last = Status::Ok();
            return m_last;
        }

        double value = 0.0;
        Status conv = parseDouble(*s, value, (m_index > 0 ? m_index - 1 : 0), m_tokens.sentence());
        if (!conv.ok())
        {
            m_last = conv;
            return m_last;
        }

        out = value;
        m_last = Status::Ok();
        return m_last;
    }
} // namespace nmea
