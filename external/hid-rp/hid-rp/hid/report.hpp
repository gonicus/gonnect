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
#ifndef __HID_REPORT_HPP_
#define __HID_REPORT_HPP_

#include <array>
#include <cstdint>
#include <limits>
#include <variant>
#include <type_traits>

namespace hid
{
enum class protocol : std::uint8_t
{
    REPORT = 0x01, // Default HID protocol
    BOOT = 0x00, // BOOT protocol (either keyboard or mouse, as specified in USB HID class document)
};

namespace report
{
enum class type : std::uint8_t
{
    INPUT = 0x01,   // Report sent by the device
    OUTPUT = 0x02,  // Report sent to the device
    FEATURE = 0x03, // Bidirectional configuration report
};

/// @note  An HID report descriptor either doesn't define report IDs at all,
///        or uses report IDs starting from index 1.
///        When report IDs are used, they are always the first byte of any HID report.
class id
{
  public:
    using type = std::uint8_t;

    constexpr id(type value)
        : value_(value)
    {}
    constexpr static type min() { return 1; }
    constexpr static type max() { return std::numeric_limits<type>::max(); }
    constexpr operator type&() { return value_; }
    constexpr operator type() const { return value_; }
    constexpr bool valid() const { return value_ >= min(); }

  private:
    type value_;
};

/// @brief A report is uniquely identified by two parameters: its type, and its (optional) ID
class selector
{
  public:
    constexpr selector(report::type t, report::id i = 0)
        : storage_{static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(t)}
    {}
    constexpr explicit selector(std::uint16_t raw)
        : storage_{static_cast<std::uint8_t>(raw), static_cast<std::uint8_t>(raw >> 8)}
    {}
    constexpr selector() {}
    constexpr report::type type() const { return static_cast<report::type>(storage_[1]); }
    constexpr report::id id() const { return report::id(storage_[0]); }
    constexpr bool valid() const { return storage_[1] > 0; }
    constexpr void clear() { *this = selector(); }

    constexpr bool operator==(const selector& rhs) const = default;
    constexpr bool operator!=(const selector& rhs) const = default;

  private:
    std::array<std::uint8_t, 2> storage_{};
};

struct id_base
{
    report::id id;
    bool operator==(const id_base& other) const = default;
};

/// @brief Base type for report storage structures.
template <report::type TYPE, id::type REPORT_ID>
struct base : public std::conditional_t<REPORT_ID != 0, id_base, std::monostate>
{
  private:
    using base_t = std::conditional_t<REPORT_ID != 0, id_base, std::monostate>;

  public:
    constexpr static report::type type() { return TYPE; }
    constexpr static bool has_id() { return (REPORT_ID > 0); }
    constexpr static report::id::type ID{REPORT_ID};
    constexpr static report::selector selector() { return report::selector(type(), ID); }

    std::uint8_t* data() { return reinterpret_cast<std::uint8_t*>(this); }
    const std::uint8_t* data() const { return reinterpret_cast<const std::uint8_t*>(this); }

    constexpr base()
        requires(has_id())
        : base_t(REPORT_ID)
    {}
    constexpr base()
        requires(not has_id())
    {}
    bool operator==(const base& other) const = default;
};
static_assert(base<type::INPUT, 0>().selector() == selector(0x100));
static_assert(base<type::OUTPUT, 0x42>().selector() == selector(0x242));

template <class T, report::type TYPE = T::type(), id::type REPORT_ID = T::ID>
concept Data = std::is_base_of<base<TYPE, REPORT_ID>, T>::value;
} // namespace report
} // namespace hid

#endif // __HID_REPORT_HPP_
