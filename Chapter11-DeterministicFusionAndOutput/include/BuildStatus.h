// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include <ostream>

enum class BuildError
{
    Ok = 0,
    NoPipelinesConfigured,
    GpsThreadUnspecified,
    EnvThreadUnspecified
};

inline const char* to_string(BuildError e) noexcept
{
    switch (e)
    {
    case BuildError::Ok: return "Ok";
    case BuildError::NoPipelinesConfigured: return "NoPipelinesConfigured";
    case BuildError::GpsThreadUnspecified: return "GpsThreadUnspecified";
    case BuildError::EnvThreadUnspecified: return "EnvThreadUnspecified";
    default: return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, BuildError e)
{
    os << to_string(e);
    return os;
}

struct BuildStatus
{
    BuildError error { BuildError::Ok };
    const char* message { "" };

    static BuildStatus success() noexcept
    {
        return { BuildError::Ok, "" };
    }

    static BuildStatus fail(BuildError e, const char* msg) noexcept
    {
        return { e, msg };
    }

    explicit operator bool() const noexcept
    {
        return error == BuildError::Ok;
    }
};
