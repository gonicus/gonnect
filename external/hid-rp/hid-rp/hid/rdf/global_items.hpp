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
#ifndef __HID_RDF_GLOBAL_ITEMS_HPP_
#define __HID_RDF_GLOBAL_ITEMS_HPP_

#include "hid/rdf/exception.hpp"
#include "hid/rdf/short_item.hpp"
#include "hid/rdf/unit.hpp"
#include "hid/usage.hpp"

namespace hid::rdf
{
template <byte_type DATA_SIZE = 4, typename T>
constexpr auto logical_min(T value)
{
    return short_item<DATA_SIZE>(global::tag::LOGICAL_MINIMUM, value);
}

template <byte_type DATA_SIZE = 4, typename T>
constexpr auto logical_max(T value)
{
    return short_item<DATA_SIZE>(global::tag::LOGICAL_MAXIMUM, value);
}

template <byte_type DATA_SIZE = 4, typename TMin, typename TMax>
constexpr auto logical_limits(TMin min, TMax max)
{
    return logical_min<DATA_SIZE>(min), logical_max<DATA_SIZE>(max);
}

template <byte_type DATA_MIN_SIZE, byte_type DATA_MAX_SIZE = DATA_MIN_SIZE, typename TMin,
          typename TMax>
constexpr auto logical_limits(TMin min, TMax max)
{
    return logical_min<DATA_MIN_SIZE>(min), logical_max<DATA_MAX_SIZE>(max);
}

template <byte_type DATA_SIZE = 4, typename T>
constexpr auto physical_min(T value)
{
    return short_item<DATA_SIZE>(global::tag::PHYSICAL_MINIMUM, value);
}

template <byte_type DATA_SIZE = 4, typename T>
constexpr auto physical_max(T value)
{
    return short_item<DATA_SIZE>(global::tag::PHYSICAL_MAXIMUM, value);
}

template <byte_type DATA_SIZE = 4, typename TMin, typename TMax>
constexpr auto physical_limits(TMin min, TMax max)
{
    return physical_min<DATA_SIZE>(min), physical_max<DATA_SIZE>(max);
}

template <byte_type DATA_MIN_SIZE, byte_type DATA_MAX_SIZE = DATA_MIN_SIZE, typename TMin,
          typename TMax>
constexpr auto physical_limits(TMin min, TMax max)
{
    return physical_min<DATA_MIN_SIZE>(min), physical_max<DATA_MAX_SIZE>(max);
}

constexpr auto physical_limits_clear()
{
    return physical_min<0>(0), physical_max<0>(0);
}

class report_id : public short_item<1>
{
  public:
    constexpr report_id(std::uint8_t value)
        : short_item(global::tag::REPORT_ID, value)
    {
        HID_RDF_ASSERT((value > 0), ex_report_id_zero);
    }
};

/// @brief Creates a report ID item only if the template parameter is valid.
template <std::uint8_t REPORT_ID>
constexpr array<(REPORT_ID > 0) ? sizeof(report_id) : 0> conditional_report_id()
{
    array<(REPORT_ID > 0) ? sizeof(report_id) : 0> data{};
    for (unsigned i = 0; i < data.size(); ++i)
    {
        data[i] = report_id(REPORT_ID)[i];
    }
    return data;
}

template <byte_type DATA_SIZE = 1, typename T>
constexpr auto report_count(T value)
{
    return short_item<DATA_SIZE>(global::tag::REPORT_COUNT, value);
}

template <byte_type DATA_SIZE = 1, typename T>
constexpr auto report_size(T value)
{
    return short_item<DATA_SIZE>(global::tag::REPORT_SIZE, value);
}

template <UsageType T>
constexpr auto usage_page()
{
    constexpr std::size_t PAGE_ID_SIZE = global::usage_page_size<T>();
    return short_item<PAGE_ID_SIZE>(global::tag::USAGE_PAGE, page::info<T>::page_id);
}

constexpr auto push_globals()
{
    return short_item<1>(global::tag::PUSH);
}

constexpr auto pop_globals()
{
    return short_item<1>(global::tag::POP);
}

} // namespace hid::rdf

#endif // __HID_RDF_GLOBAL_ITEMS_HPP_
