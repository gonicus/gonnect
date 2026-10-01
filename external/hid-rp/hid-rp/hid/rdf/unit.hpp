// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "hid/rdf/short_item.hpp"
#include "sized_unsigned.hpp"

namespace hid::rdf::unit
{
using system = global::unit_system;

[[nodiscard]] constexpr std::uint32_t calculate_code(system sys = system::NONE,
                                                     int length_count = 0, int mass_count = 0,
                                                     int time_count = 0, int temperature_count = 0,
                                                     int current_count = 0,
                                                     int luminous_int_count = 0)
{
    return static_cast<std::uint32_t>(
        (static_cast<std::int32_t>(sys) << (global::unit_nibble_index::SYSTEM * 4)) |
        ((length_count & 0xf) << (global::unit_nibble_index::LENGTH * 4)) |
        ((mass_count & 0xf) << (global::unit_nibble_index::MASS * 4)) |
        ((time_count & 0xf) << (global::unit_nibble_index::TIME * 4)) |
        ((temperature_count & 0xf) << (global::unit_nibble_index::TEMPERATURE * 4)) |
        ((current_count & 0xf) << (global::unit_nibble_index::CURRENT * 4)) |
        ((luminous_int_count & 0xf) << (global::unit_nibble_index::LUMINOUS_INTENSITY * 4)));
}

enum class code : std::uint32_t
{
    // clang-format off
    NONE = 0,
    LENGTH              = 0x00000010,
    MASS                = 0x00000100,
    TIME                = 0x00001000,
    TEMPERATURE         = 0x00010000,
    CURRENT             = 0x00100000,
    LUMINOUS_INTENSITY  = 0x01000000,

    CENTIMETER =        calculate_code(system::SI_LINEAR,         1,  0,  0,  0,  0,  0),
    RADIAN =            calculate_code(system::SI_ROTATION,       1,  0,  0,  0,  0,  0),
    INCH =              calculate_code(system::ENGLISH_LINEAR,    1,  0,  0,  0,  0,  0),
    DEGREE =            calculate_code(system::ENGLISH_ROTATION,  1,  0,  0,  0,  0,  0),
    GRAM =              calculate_code(system::SI_LINEAR,         0,  1,  0,  0,  0,  0),
    SLUG =              calculate_code(system::ENGLISH_LINEAR,    0,  1,  0,  0,  0,  0),
    SECOND =            calculate_code(system::SI_LINEAR,         0,  0,  1,  0,  0,  0),
    KELVIN =            calculate_code(system::SI_LINEAR,         0,  0,  0,  1,  0,  0),
    FAHRENHEIT =        calculate_code(system::ENGLISH_LINEAR,    0,  0,  0,  1,  0,  0),
    AMPERE =            calculate_code(system::SI_LINEAR,         0,  0,  0,  0,  1,  0),
    CANDELA =           calculate_code(system::SI_LINEAR,         0,  0,  0,  0,  0,  1),
    NEWTON =            calculate_code(system::SI_LINEAR,         1,  1, -2,  0,  0,  0),
    METER_PER_SECOND =  calculate_code(system::SI_LINEAR,         1,  0, -1,  0,  0,  0),
    METER_PER_SECOND2 = calculate_code(system::SI_LINEAR,         1,  0, -2,  0,  0,  0),
    PASCAL =            calculate_code(system::SI_LINEAR,        -1,  1, -2,  0,  0,  0),
    JOULE =             calculate_code(system::SI_LINEAR,         2,  1, -2,  0,  0,  0),
    HERTZ =             calculate_code(system::SI_LINEAR,         0,  0, -1,  0,  0,  0),
    DEGREE_PER_SECOND = calculate_code(system::ENGLISH_ROTATION,  1,  0, -1,  0,  0,  0),
    DEGREE_PER_SECOND2 =calculate_code(system::ENGLISH_ROTATION,  1,  0, -2,  0,  0,  0),
    RADIAN_PER_SECOND = calculate_code(system::SI_ROTATION,       1,  0, -1,  0,  0,  0),
    RADIAN_PER_SECOND2 =calculate_code(system::SI_ROTATION,       1,  0, -2,  0,  0,  0),
    WATT =              calculate_code(system::SI_LINEAR,         2,  1, -3,  0,  0,  0),
    AMPERE_PER_SECOND = calculate_code(system::SI_LINEAR,         0,  0, -1,  0,  1,  0),
    COULOMB =           calculate_code(system::SI_LINEAR,         0,  0,  1,  0,  1,  0),
    FARAD =             calculate_code(system::SI_LINEAR,        -2, -1,  4,  0,  2,  0),
    HENRY =             calculate_code(system::SI_LINEAR,         2,  1, -2,  0, -2,  0),
    LUX =               calculate_code(system::SI_LINEAR,        -2,  0,  0,  0,  0,  1),
    OHM =               calculate_code(system::SI_LINEAR,         2,  1, -3,  0, -2,  0),
    SIEMENS =           calculate_code(system::SI_LINEAR,        -2, -1,  3,  0,  2,  0),
    TESLA =             calculate_code(system::SI_LINEAR,         0,  1, -2,  0, -1,  0),
    VOLT =              calculate_code(system::SI_LINEAR,         2,  1, -3,  0, -1,  0),
    WEBER =             calculate_code(system::SI_LINEAR,         2,  1, -2,  0, -1,  0),
    // clang-format on
    GAUSS = TESLA,
};

struct nibble
{
    std::int32_t value : 4;
};

/// @brief Returns the exponent needed to convert the unit to SI base units,
/// to convert centimeter to meter and gram to kilogram.
/// @param unit_code: The HID unit code.
/// @return The exponent needed to use the unit as an SI unit.
[[nodiscard]] constexpr std::int8_t get_si_exponent(code unit_code)
{
    nibble length{(std::int32_t)unit_code >> (global::unit_nibble_index::LENGTH * 4)};
    nibble mass{(std::int32_t)unit_code >> (global::unit_nibble_index::MASS * 4)};
    // meter -> centimeter => 2
    // kilogram -> gram => 3
    return length.value * 2 + mass.value * 3; // NOLINT
}

template <typename TItem>
[[nodiscard]] constexpr std::int32_t get_exponent(const TItem& exp)
{
    // TODO: assert that the other data bits are zero
    // stored on 4 bytes
    return nibble{exp.value_signed()}.value;
}

template <code CODE>
constexpr auto unit()
{
    return short_item<byte_width(CODE)>(global::tag::UNIT, static_cast<std::uint32_t>(CODE));
}

template <std::size_t DATA_SIZE>
[[nodiscard]] constexpr auto unit(code flags)
{
    return short_item<DATA_SIZE>(global::tag::UNIT, static_cast<std::uint32_t>(flags));
}

class [[nodiscard]] exponent : public short_item<1>
{
  public:
    constexpr exponent(std::int8_t exp = 0)
        : short_item(global::tag::UNIT_EXPONENT, static_cast<byte_type>(exp & 0xf))
    {}
};

/// @brief  Template for exact units. Each unit is defined by its code and its base exponent
///         (as some of the default SI units have non-zero base exponent).
///         Creating a derived object will therefore contain a unit and a unit exponent item.
/// @tparam CODE_: The unit's code
/// @tparam BASE_EXP_: The unit's base exponent
template <code CODE_, std::int8_t BASE_EXP_ = get_si_exponent(CODE_)>
class [[nodiscard]] base : public array<1 + byte_width(CODE_) + sizeof(exponent)>
{
    static constexpr std::size_t UNIT_CODE_SIZE = byte_width(CODE_);
    static constexpr std::size_t EXPONENT_SIZE = sizeof(exponent);
    static constexpr std::size_t SIZE = (1 + UNIT_CODE_SIZE) + EXPONENT_SIZE;

    using base_t = array<SIZE>;

  public:
    static constexpr code CODE = CODE_;
    static constexpr std::int8_t BASE_EXPONENT = BASE_EXP_;

    static constexpr auto unit_item() { return unit::unit<CODE>(); }

    [[nodiscard]] static constexpr auto exponent_item(std::int8_t relative_exponent = 0)
    {
        return unit::exponent(BASE_EXPONENT + relative_exponent);
    }

    /// @brief Create items defining an exact unit.
    /// @param relative_exponent: relative ten's exponent to be used for the variable field
    [[nodiscard]] constexpr base(std::int8_t relative_exponent = 0)
        : base_t((unit_item(), exponent_item(BASE_EXPONENT + relative_exponent)))
    {}
};

using none = base<code::NONE>;
using centimeter = base<code::CENTIMETER, 0>;
using meter = base<code::CENTIMETER>;
static_assert(meter::BASE_EXPONENT == 2);
using radian = base<code::RADIAN>;
using degree = base<code::DEGREE>;
using gram = base<code::GRAM, 0>;
using kilogram = base<code::GRAM>;
static_assert(kilogram::BASE_EXPONENT == 3);
using second = base<code::SECOND>;
using kelvin = base<code::KELVIN>;
using fahrenheit = base<code::FAHRENHEIT>;
using ampere = base<code::AMPERE>;
using candela = base<code::CANDELA>;
using newton = base<code::NEWTON>;
using meter_per_second = base<code::METER_PER_SECOND>;
using meter_per_second2 = base<code::METER_PER_SECOND2>;
using pascal = base<code::PASCAL>;
using joule = base<code::JOULE>;
using hertz = base<code::HERTZ>;
using radian_per_second = base<code::RADIAN_PER_SECOND>;
using radian_per_second2 = base<code::RADIAN_PER_SECOND2>;
using degree_per_second = base<code::DEGREE_PER_SECOND>;
using degree_per_second2 = base<code::DEGREE_PER_SECOND2>;
using watt = base<code::WATT>;
using ampere_per_second = base<code::AMPERE_PER_SECOND>;
using coulomb = base<code::COULOMB>;
using farad = base<code::FARAD>;
using henry = base<code::HENRY>;
using lux = base<code::LUX>;
using nit = lux;
using ohm = base<code::OHM>;
using siemens = base<code::SIEMENS>;
using tesla = base<code::TESLA>;
using gauss = base<code::TESLA, tesla::BASE_EXPONENT - 4>;
using volt = base<code::VOLT>;
using weber = base<code::WEBER>;

} // namespace hid::rdf::unit
