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
#ifndef __HID_RDF_LOCAL_ITEMS_HPP_
#define __HID_RDF_LOCAL_ITEMS_HPP_

#include "hid/rdf/short_item.hpp"
#include "hid/usage.hpp"

namespace hid::rdf
{
template <std::size_t DATA_SIZE = 2, UsageType T>
constexpr auto usage(T value)
{
    constexpr std::size_t ID_SIZE = std::min(sizeof(T), DATA_SIZE);
    static_assert((ID_SIZE > 0) and (ID_SIZE <= sizeof(usage_id_t)));
    return short_item<ID_SIZE>(local::tag::USAGE, static_cast<usage_id_t>(value));
}

/// @note  Extended usage contains the usage page as well, otherwise the global usage page is
/// considered.
///        Extended usage is identified not by tag, but by the data size of the item.
template <UsageType T>
constexpr auto usage_extended(T value)
{
    return short_item<sizeof(usage_t)>(local::tag::USAGE, usage_t(value));
}

/// @note  Usages are local items, there must be a min-max pair each time
///        (hence no definition for usage_min or usage_max).
template <std::size_t DATA_MIN_SIZE = 2, std::size_t DATA_MAX_SIZE = 2, UsageType T>
constexpr auto usage_limits(T min, T max)
{
    constexpr std::size_t MIN_ID_SIZE = std::min(sizeof(T), DATA_MIN_SIZE);
    constexpr std::size_t MAX_ID_SIZE = std::min(sizeof(T), DATA_MAX_SIZE);
    static_assert((MIN_ID_SIZE > 0) and (MIN_ID_SIZE <= sizeof(usage_id_t)));
    static_assert((MAX_ID_SIZE > 0) and (MAX_ID_SIZE <= sizeof(usage_id_t)));
    return short_item<MIN_ID_SIZE>(local::tag::USAGE_MINIMUM, static_cast<usage_id_t>(min)),
           short_item<MAX_ID_SIZE>(local::tag::USAGE_MAXIMUM, static_cast<usage_id_t>(max));
}

template <std::size_t DATA_MAX_SIZE = 2, UsageType T>
constexpr auto usage_limits(nullusage_t min, T max)
{
    constexpr std::size_t MIN_ID_SIZE = 1;
    constexpr std::size_t MAX_ID_SIZE = std::min(sizeof(T), DATA_MAX_SIZE);
    static_assert((MAX_ID_SIZE > 0) and (MAX_ID_SIZE <= sizeof(usage_id_t)));
    return short_item<MIN_ID_SIZE>(local::tag::USAGE_MINIMUM, static_cast<usage_id_t>(min)),
           short_item<MAX_ID_SIZE>(local::tag::USAGE_MAXIMUM, static_cast<usage_id_t>(max));
}

template <UsageType T>
constexpr auto usage_extended_limits(T min, T max)
{
    return short_item<sizeof(usage_t)>(local::tag::USAGE_MINIMUM, usage_t(min)),
           short_item<sizeof(usage_t)>(local::tag::USAGE_MAXIMUM, usage_t(max));
}

template <UsageType T>
constexpr auto usage_extended_limits(nullusage_t min, T max)
{
    auto ui = usage_t(max);
    return short_item<sizeof(usage_t)>(local::tag::USAGE_MINIMUM, usage_t(ui.page_id(), 0)),
           short_item<sizeof(usage_t)>(local::tag::USAGE_MAXIMUM, ui);
}

} // namespace hid::rdf

#endif // __HID_RDF_LOCAL_ITEMS_HPP_
