// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Autumnal Software

//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#pragma once

#include <bitset>
#include <iostream>
#include <type_traits>

#include "traits.h"

class Register32Bits {
public:
    //
    // After construction will be valid all zeros, but considered empty.
    //
    Register32Bits() :
        bits(0),
        empty{true}
    {}

    explicit Register32Bits(std::uint32_t value)
        : bits(value)
        , empty(false)
    {
    }


    bool isEmpty() const { return empty; }
    operator bool() const { return isEmpty(); }

    template <typename Enum, typename std::enable_if<is_scoped_enum<Enum>::value, Enum>::type* = nullptr>
    void setBit(Enum pos, bool value) {
        if (empty)
            return;
        bits.set(static_cast<std::size_t>(pos), value);
    }

    void setBit(std::uint32_t pos, bool b) { bits.set(pos, b); }

    template <typename Enum, typename std::enable_if<is_scoped_enum<Enum>::value, Enum>::type* = nullptr>
    bool getBit(Enum pos) const {
        return bits.test(static_cast<std::size_t>(pos));
    }

    // Convert bitset to a numeric value
    uint32_t toUInt() const {
        return static_cast<uint32_t>(bits.to_ulong());
    }

    friend std::ostream& operator<<(std::ostream& os, const Register32Bits& reg) {
        os << reg.bits;
        return os;
    }

private:
    std::bitset<32> bits;
    bool empty{true};
};
