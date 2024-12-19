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
#ifndef __HID_RDF_CONSTANTS_HPP_
#define __HID_RDF_CONSTANTS_HPP_

#include "hid/report.hpp"

namespace hid::rdf
{
using byte_type = std::uint8_t;

enum class item_type : byte_type
{
    MAIN = 0,
    GLOBAL = 1,
    LOCAL = 2,
    RESERVED = 3
};

namespace main
{
enum class tag : byte_type
{
    INPUT = 0x8,
    OUTPUT = 0x9,
    FEATURE = 0xb,
    COLLECTION = 0xa,
    END_COLLECTION = 0xc,
};

enum class collection_type : byte_type
{
    PHYSICAL = 0,
    APPLICATION = 1,
    LOGICAL = 2,
    REPORT = 3,
    NAMED_ARRAY = 4,
    USAGE_SWITCH = 5,
    USAGE_MODIFIER = 6,
    // reserved 0x07 - 0x7f
    VENDOR_DEFINED_BEGIN = 0x80,
};

namespace data_field_flag
{
constexpr std::uint16_t DATA = 0;
constexpr std::uint16_t CONSTANT = 0x01;

constexpr std::uint16_t ARRAY = 0;
constexpr std::uint16_t VARIABLE = 0x02;

constexpr std::uint16_t ABSOLUTE = 0;
constexpr std::uint16_t RELATIVE = 0x04;

constexpr std::uint16_t WRAP = 0x08;

constexpr std::uint16_t NONLINEAR = 0x10;

constexpr std::uint16_t NO_PREFERRED = 0x20;

constexpr std::uint16_t NULL_STATE = 0x40;

constexpr std::uint16_t VOLATILE = 0x80;

constexpr std::uint16_t BUFFERED_BYTES = 0x100;
}; // namespace data_field_flag

constexpr static report::type tag_to_report_type(tag t)
{
    if (t == tag::INPUT)
    {
        return report::type::INPUT;
    }
    else if (t == tag::OUTPUT)
    {
        return report::type::OUTPUT;
    }
    else // if (t == tag::FEATURE)
    {
        return report::type::FEATURE;
    }
}

template <hid::report::type TYPE_>
constexpr static main::tag report_type_to_tag()
{
    static_assert((std::integral_constant<bool, TYPE_ == report::type::INPUT>::value) or
                  (std::integral_constant<bool, TYPE_ == report::type::OUTPUT>::value) or
                  (std::integral_constant<bool, TYPE_ == report::type::FEATURE>::value));
    if (std::integral_constant<bool, TYPE_ == report::type::INPUT>::value)
    {
        return main::tag::INPUT;
    }
    else if (std::integral_constant<bool, TYPE_ == report::type::OUTPUT>::value)
    {
        return main::tag::OUTPUT;
    }
    else // if (std::integral_constant<bool, TYPE_ == report::type::FEATURE>::value)
    {
        return main::tag::FEATURE;
    }
}
} // namespace main

namespace global
{
enum class tag : byte_type
{
    USAGE_PAGE = 0x0,
    LOGICAL_MINIMUM = 0x1,
    LOGICAL_MAXIMUM = 0x2,
    PHYSICAL_MINIMUM = 0x3,
    PHYSICAL_MAXIMUM = 0x4,
    UNIT_EXPONENT = 0x5,
    UNIT = 0x6,
    REPORT_SIZE = 0x7,
    REPORT_ID = 0x8,
    REPORT_COUNT = 0x9,
    PUSH = 0xa,
    POP = 0xb,
};

enum class unit_system : byte_type
{
    NONE = 0,
    SI_LINEAR = 1,
    SI_ROTATION = 2,
    ENGLISH_LINEAR = 3,
    ENGLISH_ROTATION = 4,
};

enum unit_nibble_index
{
    SYSTEM = 0,
    LENGTH = 1,
    MASS = 2,
    TIME = 3,
    TEMPERATURE = 4,
    CURRENT = 5,
    LUMINOUS_INTENSITY = 6,
};
} // namespace global

namespace local
{
enum class tag : byte_type
{
    USAGE = 0x0,
    USAGE_MINIMUM = 0x1,
    USAGE_MAXIMUM = 0x2,
    DESIGNATOR_INDEX = 0x3,
    DESIGNATOR_MINIMUM = 0x4,
    DESIGNATOR_MAXIMUM = 0x5,
    STRING_INDEX = 0x7,
    STRING_MINIMUM = 0x8,
    STRING_MAXIMUM = 0x9,
    DELIMITER = 0xa,
};
}

/// @brief  Matches the tag type information to type code.
/// @tparam TTag: HID item tag type
/// @return HID item tag type translated to type code
template <typename TTag>
constexpr item_type match_type()
{
    if constexpr (std::is_same<TTag, main::tag>::value)
    {
        return item_type::MAIN;
    }
    else if constexpr (std::is_same<TTag, global::tag>::value)
    {
        return item_type::GLOBAL;
    }
    else if constexpr (std::is_same<TTag, local::tag>::value)
    {
        return item_type::LOCAL;
    }
    else
    {
        return item_type::RESERVED;
    }
}

} // namespace hid::rdf

#endif // __HID_RDF_CONSTANTS_HPP_
