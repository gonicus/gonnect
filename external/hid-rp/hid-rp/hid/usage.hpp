// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace hid
{
using page_id_t = std::uint16_t;
using usage_id_t = std::uint16_t;

namespace page
{
// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes,cppcoreguidelines-avoid-const-or-ref-data-members)
struct [[nodiscard]] info
{
  private:
    const char* (*const get_usage_name_)(hid::usage_id_t id);

  public:
    const char* const page_name;
    const page_id_t page_id;
    const usage_id_t max_usage_id;
    const usage_id_t ius_mask;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    constexpr info(page_id_t page, usage_id_t max, const char* name,
                   const char* (*get_name)(hid::usage_id_t id) = nullptr,
                   usage_id_t inline_usage_switch_mask = 0)
        : get_usage_name_(get_name),
          page_name(name),
          page_id(page),
          max_usage_id(max),
          ius_mask(inline_usage_switch_mask)
    {}

    [[nodiscard]] const char* get_usage_name(usage_id_t id) const
    {
        if (get_usage_name_ != nullptr)
        {
            return get_usage_name_(id);
        }
        return nullptr;
    }

    constexpr bool operator==(const info& other) const { return page_id == other.page_id; }
    constexpr bool operator!=(const info& other) const = default;
    [[nodiscard]] constexpr bool valid_page() const { return page_id != 0; }
};
// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes,cppcoreguidelines-avoid-const-or-ref-data-members)

/// @brief Each usage page type needs the specialization of this template method
///        for this library to function correctly.
template <typename T>
constexpr auto get_info()
{
    return info(0, 0, "unknown");
}

} // namespace page

/// @brief This concept matches types with valid page information,
///        which is necessary for determining the full usage value.
template <typename T>
concept UsageType = (sizeof(T) <= sizeof(usage_id_t)) and (page::get_info<T>().page_id > 0);

/// @brief This concept matches usage pages that support inline usage switch purpose IDs.
///        Only the sensor page is known to be affected.
template <typename T>
concept InlineSwitchableUsageType = UsageType<T> and (page::get_info<T>().ius_mask > 0);

namespace page
{
// NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
template <InlineSwitchableUsageType T>
constexpr auto operator|(T lhs, T rhs)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) |
                          static_cast<std::underlying_type_t<T>>(rhs));
}
// NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)

template <InlineSwitchableUsageType T>
constexpr auto get_base_usage(T usage)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(usage) &
                          ~page::get_info<T>().ius_mask);
}
template <InlineSwitchableUsageType T>
constexpr auto get_inline_switch(T usage)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(usage) &
                          page::get_info<T>().ius_mask);
}
} // namespace page

namespace rdf::global
{
template <UsageType T>
constexpr std::size_t usage_page_size()
{
    return page::get_info<T>().page_id > std::numeric_limits<std::uint8_t>::max() ? 2 : 1;
}
} // namespace rdf::global

/// @brief The usage_t class holds the entire usage information in one class,
///        combining the page ID and usage ID into a single 32-bit variable.
class usage_t
{
  public:
    using type = std::uint32_t;

    template <typename... T>
    struct is_same_usage_types : std::true_type
    {};

    template <typename T, typename... U>
    struct is_same_usage_types<T, U...> : std::bool_constant<(std::is_same_v<T, U> and ...)>
    {};

    constexpr explicit usage_t(type value)
        : value_(value)
    {}
    constexpr usage_t(page_id_t page, usage_id_t usage)
        : value_((static_cast<type>(page) << 16) | usage)
    {}
    template <UsageType T>
    constexpr usage_t(T usage)
        : value_((static_cast<type>(page::get_info<T>().page_id) << 16) | static_cast<type>(usage))
    {}
    [[nodiscard]] constexpr operator type&() { return value_; }
    [[nodiscard]] constexpr operator type() const { return value_; }
    [[nodiscard]] constexpr page_id_t page_id() const { return value_ >> 16; }
    [[nodiscard]] constexpr usage_id_t id() const
    {
        return value_ & std::numeric_limits<usage_id_t>::max();
    }

    constexpr auto operator<=>(const usage_t&) const = default;

    template <UsageType T>
    [[nodiscard]] constexpr bool has_page() const
    {
        return page_id() == page::get_info<T>().page_id;
    }
    template <UsageType T>
    [[nodiscard]] constexpr bool operator==(T usage) const
    {
        return has_page<T>() and (id() == static_cast<usage_id_t>(usage));
    }

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
