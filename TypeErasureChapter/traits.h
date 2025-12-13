//-----------------------------------------------------------------------------
// Copyright (c) 2025 Mark Wilson
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#pragma once

#include <type_traits>

//
// This file has traits that are not in C++17
//

template <typename T>
struct is_scoped_enum {
    static constexpr bool value = std::is_enum<T>::value && !std::is_convertible<T, int>::value;
};
