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
#ifndef __HID_RDF_UNIT_HPP_
#define __HID_RDF_UNIT_HPP_

#include "hid/rdf/short_item.hpp"

namespace hid::rdf
{
namespace unit
{
using system = global::unit_system;

// TODO: use calculate_code() instead of hardcoded
enum class code : std::uint32_t
{
    NONE = 0,
    LENGTH = 0x00000010,
    MASS = 0x00000100,
    TIME = 0x00001000,
    TEMPERATURE = 0x00010000,
    CURRENT = 0x00100000,
    LUMINOUS_INTENSITY = 0x01000000,

    METER = 0x00000011,
    RADIAN = 0x00000012,
    INCH = 0x00000013,
    DEGREE = 0x00000014,
    GRAM = 0x00000101,
    SLUG = 0x00000103,
    SECOND = 0x00001001,
    KELVIN = 0x00010001,
    FAHRENHEIT = 0x00010003,
    AMPERE = 0x00100001,
    CANDELA = 0x01000001,
    NEWTON = 0x0000E111,
    METER_PER_SECOND = 0x0000F011,
    METER_PER_SECOND2 = 0x0000E011,
    PASCAL = 0x0000E011,
    JOULE = 0x0000E121,
    HERTZ = 0x0000F001,
    DEGREE_PER_SECOND = 0x0000F014,
    DEGREE_PER_SECOND2 = 0x0000E014,
    RADIAN_PER_SECOND = 0x0000F012,
    RADIAN_PER_SECOND2 = 0x0000E012,
    WATT = 0x0000D121,
    AMPERE_PER_SECOND = 0x0010F001,
    COULOMB = 0x00101001,
    FARAD = 0x00204FE1,
    GAUSS = 0x00F0E101,
    HENRY = 0x00E0E121,
    LUX = 0x010000E1,
    OHM = 0x00E0D121,
    SIEMENS = 0x00203FE1,
    TESLA = 0x00F0E101,
    VOLT = 0x00F0D121,
    WEBER = 0x00F0E121,
};

constexpr std::uint32_t calculate_code(system sys = system::NONE, int length_count = 0,
                                       int mass_count = 0, int time_count = 0,
                                       int temperature_count = 0, int current_count = 0,
                                       int luminous_int_count = 0)
{
    return static_cast<std::uint32_t>(static_cast<std::uint32_t>(sys)
                                      << (global::unit_nibble_index::SYSTEM * 4)) |
           ((length_count & 0xf) << (global::unit_nibble_index::LENGTH * 4)) |
           ((mass_count & 0xf) << (global::unit_nibble_index::MASS * 4)) |
           ((time_count & 0xf) << (global::unit_nibble_index::TIME * 4)) |
           ((temperature_count & 0xf) << (global::unit_nibble_index::TEMPERATURE * 4)) |
           ((current_count & 0xf) << (global::unit_nibble_index::CURRENT * 4)) |
           ((luminous_int_count & 0xf) << (global::unit_nibble_index::LUMINOUS_INTENSITY * 4));
}

template <typename TItem>
constexpr std::int32_t get_exponent(const TItem& exp)
{
    // stored on 4 bytes
    std::uint32_t uval = exp.value_unsigned() & 0xf;
    std::int32_t value = static_cast<std::int32_t>(uval);
    // sign extend
    if ((uval >> 3) > 0)
    {
        value |= 0xffffffff << 3;
    }
    return value;
}

template <byte_type DATA_SIZE>
class unit_item : public short_item<DATA_SIZE>
{
    using base_t = short_item<DATA_SIZE>;

  public:
    constexpr unit_item(std::uint32_t flags)
        : base_t(global::tag::UNIT, flags)
    {}
};

class exponent_item : public short_item<1>
{
  public:
    constexpr exponent_item(int exp = 0)
        : short_item(global::tag::UNIT_EXPONENT, static_cast<byte_type>(exp & 0xf))
    {}
};

/// @brief  Template for exact units. Each unit is defined by its code and its base exponent
///         (as some of the default SI units have non-zero base exponent).
///         Creating a derived object will therefore contain a unit and a unit exponent item.
/// @tparam CODE_: The unit's code
/// @tparam BASE_EXP_: The unit's base exponent
template <code CODE_, std::int8_t BASE_EXP_ = 0>
class base : public array<1 +
                          ((static_cast<std::uint32_t>(CODE_) > 0xffff)
                               ? 4
                               : ((static_cast<std::uint32_t>(CODE_) > 0xff) ? 2 : 1)) +
                          sizeof(exponent_item)>
{
    static constexpr std::size_t UNIT_CODE_SIZE =
        ((static_cast<std::uint32_t>(CODE_) > 0xffff)
             ? 4
             : ((static_cast<std::uint32_t>(CODE_) > 0xff) ? 2 : 1));
    static constexpr std::size_t SIZE = 1 + UNIT_CODE_SIZE + sizeof(exponent_item);

    using base_t = array<SIZE>;

  public:
    static constexpr code CODE = CODE_;
    static constexpr int BASE_EXPONENT = BASE_EXP_;

    /// @brief Create items defining an exact unit.
    /// @param relative_exponent: relative ten's exponent to be used for the variable field
    constexpr base(std::int8_t relative_exponent = 0)
        : base_t((unit_item<UNIT_CODE_SIZE>(static_cast<std::uint32_t>(CODE)),
                  exponent_item(BASE_EXPONENT + relative_exponent)))
    {}
};

using none = base<code::NONE>;
using centimeter = base<code::METER>;
using meter = base<code::METER, 2>;
using radian = base<code::RADIAN>;
using degree = base<code::DEGREE>;
using gram = base<code::GRAM>;
using kilogram = base<code::GRAM, 3>;
using second = base<code::SECOND>;
using kelvin = base<code::KELVIN>;
using fahrenheit = base<code::FAHRENHEIT>;
using ampere = base<code::AMPERE>;
using candela = base<code::CANDELA>;
using newton = base<code::NEWTON, 2 + 3>;
using meter_per_second = base<code::METER_PER_SECOND, 2>;
using meter_per_second2 = base<code::METER_PER_SECOND2, 2>;
using pascal_ = base<code::METER_PER_SECOND2, 3 - 2>;
using joule = base<code::JOULE, 3 + 2 * 2>;
using hertz = base<code::HERTZ>;
using radian_per_second = base<code::RADIAN_PER_SECOND>;
using radian_per_second2 = base<code::RADIAN_PER_SECOND2>;
using degree_per_second = base<code::DEGREE_PER_SECOND>;
using degree_per_second2 = base<code::DEGREE_PER_SECOND2>;
using watt = base<code::WATT, 3 + 2 * 2>;
using ampere_per_second = base<code::AMPERE_PER_SECOND>;
using coulomb = base<code::COULOMB>;
using farad = base<code::FARAD, -3 - 2 * 2>;
using gauss = base<code::GAUSS, 3>;
using henry = base<code::HENRY, 3 + 2 * 2>;
using lux = base<code::LUX, -2 * 2>;
using ohm = base<code::OHM, 3 + 2 * 2>;
using siemens = base<code::SIEMENS, -3 - 2 * 2>;
using tesla = base<code::TESLA, 3>;
using volt = base<code::VOLT, 3 + 2 * 2>;
using weber = base<code::WEBER, 3 + 2 * 2>;
} // namespace unit

} // namespace hid::rdf

#endif // __HID_RDF_UNIT_HPP_
