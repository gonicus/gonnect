/// @file
///
/// @author Benedek Kupper
/// @date   2024
///
/// @copyright
///         This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
///         If a copy of the MPL was not distributed with this file, You can obtain one at
///         https://mozilla.org/MPL/2.0/.
///
#ifndef __SIZED_UNSIGNED_HPP_
#define __SIZED_UNSIGNED_HPP_

#include <cstdint>
#include <type_traits>

/// @brief  Lookup unsigned integer of matching size
template <std::size_t SIZE, class T = void>
struct sized_unsigned
{};

template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 1>>
{
    typedef std::uint8_t type;
};
template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 2>>
{
    typedef std::uint16_t type;
};
template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 4>>
{
    typedef std::uint32_t type;
};
template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 8>>
{
    typedef std::uint64_t type;
};

template <std::size_t SIZE>
using sized_unsigned_t = typename sized_unsigned<SIZE>::type;

template <typename T>
concept IntegerConvertable = std::is_convertible_v<T, sized_unsigned_t<sizeof(T)>>;

#endif // __SIZED_UNSIGNED_HPP_
