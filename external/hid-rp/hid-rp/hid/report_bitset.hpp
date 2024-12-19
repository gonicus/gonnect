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
#ifndef __HID_REPORT_BITSET_HPP_
#define __HID_REPORT_BITSET_HPP_

#include <array>
#include <cassert>
#include "sized_unsigned.hpp"

namespace hid
{

/// @brief  This class stores usages in a bitset, as a report field.
/// @tparam T the usage type
/// @tparam MIN the minimum usage to be stored on a bit
/// @tparam MAX the maximum usage to be stored on a bit
template <typename T, T MIN, T MAX>
class report_bitset
{
    using numeric_type = sized_unsigned_t<sizeof(T)>;

  public:
    constexpr static T min() { return MIN; }
    constexpr static T max() { return MAX; }
    static_assert(min() <= max());
    constexpr static std::size_t size()
    {
        return static_cast<std::size_t>(max()) - static_cast<std::size_t>(min()) + 1;
    }

    constexpr bool in_range(T usage) const { return (usage >= min()) and (usage <= max()); }
    constexpr bool set(T usage, bool value = true)
    {
        if (in_range(usage))
        {
            auto n = static_cast<numeric_type>(usage) - static_cast<numeric_type>(min());
            if (value)
            {
                bits_[n / 8] |= 1 << (n % 8);
            }
            else
            {
                bits_[n / 8] &= ~(1 << (n % 8));
            }
            return true;
        }
        else
        {
            return false;
        }
    }
    constexpr void reset() { bits_.fill(0); }
    constexpr bool reset(T usage) { return set(usage, false); }
    constexpr bool flip(T usage) { return set(usage, !test(usage)); }
    bool test(T usage) const
    {
        if (in_range(usage))
        {
            auto n = static_cast<numeric_type>(usage) - static_cast<numeric_type>(min());
            return bits_[n / 8] & (1 << (n % 8));
        }
        assert(false);
        return false;
    }
    constexpr report_bitset() = default;
    constexpr bool operator==(const report_bitset&) const = default;
    constexpr bool operator!=(const report_bitset&) const = default;

  private:
    std::array<std::uint8_t, (size() + 7) / 8> bits_{};
};

} // namespace hid

#endif // __HID_REPORT_BITSET_HPP_
