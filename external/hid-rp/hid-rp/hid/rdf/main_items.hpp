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
#ifndef __HID_RDF_MAIN_ITEMS_HPP_
#define __HID_RDF_MAIN_ITEMS_HPP_

#include "hid/rdf/global_items.hpp"
#include "hid/rdf/short_item.hpp"
#include "hid/report.hpp"

namespace hid::rdf
{
namespace main // internal use
{
class collection_begin_item : public short_item<1>
{
  public:
    constexpr collection_begin_item(collection_type t)
        : short_item(main::tag::COLLECTION, static_cast<byte_type>(t))
    {}
};

class collection_end_item : public short_item<0>
{
  public:
    constexpr collection_end_item()
        : short_item(main::tag::END_COLLECTION)
    {}
};
} // namespace main

namespace collection
{
using type = main::collection_type;

template <std::size_t... sz>
constexpr auto application(array<sz>... items)
{
    static_assert(sizeof...(items) > 0);
    return main::collection_begin_item(type::APPLICATION), (items, ...),
           main::collection_end_item();
}

template <std::size_t... sz>
constexpr auto logical(array<sz>... items)
{
    static_assert(sizeof...(items) > 0);
    return main::collection_begin_item(type::LOGICAL), (items, ...), main::collection_end_item();
}

template <std::size_t... sz>
constexpr auto report(array<sz>... items)
{
    static_assert(sizeof...(items) > 0);
    return main::collection_begin_item(type::REPORT), (items, ...), main::collection_end_item();
}

template <std::size_t... sz>
constexpr auto named_array(array<sz>... items)
{
    static_assert(sizeof...(items) > 0);
    return main::collection_begin_item(type::NAMED_ARRAY), (items, ...),
           main::collection_end_item();
}

template <std::size_t... sz>
constexpr auto physical(array<sz>... items)
{
    static_assert(sizeof...(items) > 0);
    return main::collection_begin_item(type::PHYSICAL), (items, ...), main::collection_end_item();
}

template <std::size_t... sz>
constexpr auto usage_modifier(array<sz>... items)
{
    static_assert(sizeof...(items) > 0);
    return main::collection_begin_item(type::USAGE_MODIFIER), (items, ...),
           main::collection_end_item();
}

template <std::size_t... sz>
constexpr auto usage_switch(array<sz>... items)
{
    static_assert(sizeof...(items) > 0);
    return main::collection_begin_item(type::USAGE_SWITCH), (items, ...),
           main::collection_end_item();
}
} // namespace collection

namespace main // internal use
{
/// @brief The data field flags are split and condensed to this subset that makes most sense,
///        and additional @ref field_flags that can be set independently when needed.
enum field_type : std::uint16_t
{
    ARRAY = data_field_flag::ARRAY,
    BUFFERED_VARIABLE = data_field_flag::VARIABLE | data_field_flag::BUFFERED_BYTES,
    ABSOLUTE_VARIABLE = data_field_flag::VARIABLE | data_field_flag::ABSOLUTE,
    RELATIVE_VARIABLE = data_field_flag::VARIABLE | data_field_flag::RELATIVE,
    ABSOLUTE_CONSTANT =
        data_field_flag::CONSTANT | data_field_flag::VARIABLE | data_field_flag::ABSOLUTE,
    RELATIVE_CONSTANT =
        data_field_flag::CONSTANT | data_field_flag::VARIABLE | data_field_flag::RELATIVE,
    PADDING = data_field_flag::CONSTANT | data_field_flag::ARRAY,
};

/// @brief Additional flags that can be set for fields beyond the base type.
enum field_flags : std::uint16_t
{
    NONE = 0,
    WRAP = data_field_flag::WRAP,
    NONLINEAR = data_field_flag::NONLINEAR,
    NO_PREFERRED = data_field_flag::NO_PREFERRED,
    NULL_STATE = data_field_flag::NULL_STATE,
    VOLATILE = data_field_flag::VOLATILE,
};

template <byte_type DATA_SIZE>
class data_field_item : public short_item<DATA_SIZE>
{
    using base_t = short_item<DATA_SIZE>;

  public:
    constexpr data_field_item(main::tag tag, std::uint16_t flags)
        : base_t(tag, flags)
    {}
};

template <report::type TYPE>
class data_field
{
    constexpr static const main::tag TAG = report_type_to_tag<TYPE>();

  public:
    using flags = field_flags;
    using type = field_type;

    constexpr static auto array()
    {
        // FU-MS:
        // HID spec 1.11, section 6.2.2.4:
        // "An Input item could have a data size of zero (0) bytes. In this case
        // the value of each data bit for the item can be assumed to be zero.
        // This is functionally identical to using a[n] item tag that specifies
        // a 4-byte data item followed by four zero bytes."
        // Linux handles this with 0 size, Windows doesn't...
        return data_field_item<1>(TAG, field_type::ARRAY);
    }

    constexpr static auto buffered_variable()
    {
        return data_field_item<2>(TAG, field_type::BUFFERED_VARIABLE);
    }

    constexpr static auto absolute_variable(flags extra_flags = flags::NONE)
    {
        return data_field_item<1>(TAG, static_cast<std::uint16_t>(extra_flags) |
                                           field_type::ABSOLUTE_VARIABLE);
    }

    constexpr static auto relative_variable(flags extra_flags = flags::NONE)
    {
        return data_field_item<1>(TAG, static_cast<std::uint16_t>(extra_flags) |
                                           field_type::RELATIVE_VARIABLE);
    }

    constexpr static auto absolute_constant(flags extra_flags = flags::NONE)
    {
        return data_field_item<1>(TAG, static_cast<std::uint16_t>(extra_flags) |
                                           field_type::ABSOLUTE_CONSTANT);
    }

    constexpr static auto relative_constant(flags extra_flags = flags::NONE)
    {
        return data_field_item<1>(TAG, static_cast<std::uint16_t>(extra_flags) |
                                           field_type::RELATIVE_CONSTANT);
    }

    /// @brief  Creates not only the data field item, but adds @ref report_count and @ref
    /// report_size
    ///         items as well, in order to define a specific bit padding with a single entry.
    /// @param  bitsize: the number of bits of padding to add as a new data field
    /// @return Minimal set of items to define a padding.
    constexpr static auto padding(byte_type bitsize)
    {
        return report_count<1>(1), report_size<1>(bitsize),
               data_field_item<1>(TAG, field_type::PADDING);
    }

    /// @brief  Creates optional padding if the report bit size isn't byte aligned.
    /// @return Minimal set of items to define byte padding.
    template <std::size_t REPORT_CHUNK_BIT_SIZE>
    constexpr static auto byte_padding() -> rdf::array<
        (REPORT_CHUNK_BIT_SIZE % 8 != 0) ? sizeof(padding(8 - (REPORT_CHUNK_BIT_SIZE % 8))) : 0>
    {
        rdf::array<
            (REPORT_CHUNK_BIT_SIZE % 8 != 0) ? sizeof(padding(8 - (REPORT_CHUNK_BIT_SIZE % 8))) : 0>
            data{};
        for (unsigned i = 0; i < data.size(); ++i)
        {
            data[i] = padding(8 - (REPORT_CHUNK_BIT_SIZE % 8))[i];
        }
        return data;
    }
};
} // namespace main

using input = main::data_field<report::type::INPUT>;

using output = main::data_field<report::type::OUTPUT>;

using feature = main::data_field<report::type::FEATURE>;

} // namespace hid::rdf

#endif // __HID_RDF_MAIN_ITEMS_HPP_
