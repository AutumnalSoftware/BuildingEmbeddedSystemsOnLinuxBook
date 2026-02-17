// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software
#pragma once

#include <string_view>

#include "MeasurementTypes.h"

namespace weather
{

std::string_view to_string(MeasurementKind k) noexcept;

bool from_string(std::string_view s, MeasurementKind& out) noexcept;

} // namespace weather
