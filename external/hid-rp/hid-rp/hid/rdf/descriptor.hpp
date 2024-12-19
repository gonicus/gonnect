/// @file
///
/// @author Benedek Kupper
/// @date   2022
///
/// @copyright
///         This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
///         If a copy of the MPL was not distributed with this file, You can obtain one at
///         https://mozilla.org/MPL/2.0/.
///
#ifndef __HID_RDF_DESCRIPTOR_HPP_
#define __HID_RDF_DESCRIPTOR_HPP_

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
{
    static_assert(sizeof...(items) > 0);
    return (items, ...);
}

} // namespace hid::rdf

#endif // __HID_RDF_DESCRIPTOR_HPP_
