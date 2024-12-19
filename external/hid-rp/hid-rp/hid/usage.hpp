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
#ifndef __HID_USAGE_HPP_
#define __HID_USAGE_HPP_

#include <cstdint>
#include <compare>
#include <limits>

namespace hid
{
using page_id_t = std::uint16_t;
using usage_id_t = std::uint16_t;

namespace page
{
/// @brief Each usage page type needs the specialization of this template class
///        for this library to function correctly.
template <typename T>
struct info
{
    constexpr static page_id_t page_id = 0;
    constexpr static usage_id_t max_usage_id = 0;
    constexpr static const char* name = "invalid";
};
} // namespace page

/// @brief This concept matches types with valid page information,
///        which is necessary for determining the full usage value.
template <typename T>
concept UsageType = (sizeof(T) <= sizeof(usage_id_t)) and (page::info<T>::page_id > 0);

namespace rdf::global
{
template <UsageType T>
constexpr std::size_t usage_page_size()
{
    return page::info<T>::page_id > std::numeric_limits<std::uint8_t>::max() ? 2 : 1;
}
} // namespace rdf::global

/// @brief The usage_t class holds the entire usage information in one class,
///        combining the page ID and usage ID into a single 32-bit variable.
class usage_t
{
  public:
    using type = std::uint32_t;

    constexpr explicit usage_t(type value)
        : value_(value)
    {}
    constexpr usage_t(page_id_t page, usage_id_t u)
        : value_((page << 16) | u)
    {}
    template <UsageType T>
    constexpr usage_t(T u)
        : value_((page::info<T>::page_id << 16) | static_cast<type>(u))
    {}
    constexpr operator type&() { return value_; }
    constexpr operator type() const { return value_; }
    constexpr page_id_t page_id() const { return value_ >> 16; }
    constexpr usage_id_t id() const { return value_ & std::numeric_limits<usage_id_t>::max(); }

    constexpr auto operator<=>(const usage_t&) const = default;

  private:
    type value_;
};

/// @brief Type and variable that expresses a never valid null usage state.
class nullusage_t : public usage_t
{
  public:
    constexpr nullusage_t()
        : usage_t(0)
    {}
};
constexpr nullusage_t nullusage;

namespace page
{
// alias definition to simplify descriptor definition
using hid::nullusage;
} // namespace page

} // namespace hid

#endif // __HID_RDF_USAGE_HPP_
