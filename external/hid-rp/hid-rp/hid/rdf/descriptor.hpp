// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "hid/rdf/global_items.hpp"
#include "hid/rdf/local_items.hpp"
#include "hid/rdf/main_items.hpp"

namespace hid::rdf
{
/// @brief  This function puts the input HID report descriptor elements into a single descriptor.
/// @tparam sz: deduced template parameter
/// @param  items: either individual items, or item collections
/// @return The HID report descriptor as an std::array
template <std::size_t... sz>
constexpr auto descriptor(array<sz>... items)
    requires(sizeof...(items) > 0)
{
    return (items, ...);
}

} // namespace hid::rdf
