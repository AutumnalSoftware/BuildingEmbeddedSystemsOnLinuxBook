// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

#pragma once


#include "ImmutableByteView.h"

namespace weather
{
    class ITransmitEndpoint
    {
    public:
        virtual ~ITransmitEndpoint() = default;
        virtual bool send(ImmutableByteView bytes) noexcept = 0;
    };
}
