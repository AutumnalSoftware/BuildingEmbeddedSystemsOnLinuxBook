// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Mark Wilson

#include "nmea/NMEASentenceFramer.h"

namespace nmea
{
    void SentenceFramer::push(std::string_view bytes)
    {
        m_buf.append(bytes.data(), bytes.size());
    }

    std::optional<std::string> SentenceFramer::pop()
    {
        const std::size_t start = m_buf.find('$');
        if (start == std::string::npos)
        {
            m_buf.clear();
            return std::nullopt;
        }
        if (start > 0)
        {
            m_buf.erase(0, start);
        }

        const std::size_t end = m_buf.find('\n');
        if (end == std::string::npos)
        {
            return std::nullopt;
        }

        std::string line = m_buf.substr(0, end);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        m_buf.erase(0, end + 1);

        if (line.empty())
        {
            return std::nullopt;
        }
        return line;
    }

    void SentenceFramer::reset()
    {
        m_buf.clear();
    }
} // namespace nmea
