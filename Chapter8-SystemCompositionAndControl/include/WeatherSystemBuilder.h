// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once

#include "BuildStatus.h"
#include "WeatherSystem.h"

class WeatherSystemBuilder
{
public:
    WeatherSystemBuilder() = default;

    BuildStatus build(WeatherSystem& system) const;
};
