// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <bit>
#include "hid/rdf/item.hpp"

namespace hid::rdf
{

/// @brief A byte array that concatenates with another using the comma operator.
template <std::size_t SIZE>
class [[nodiscard]] array : public std::array<byte_type, SIZE>
{
    template <std::size_t Repeats, std::size_t M, std::size_t... I>
    static constexpr hid::rdf::array<M * Repeats>
    repeat_array_impl(const hid::rdf::array<M>& src,
                      [[maybe_unused]] std::index_sequence<I...> parts)
    {
        return {src[I % M]...};
    }

  public:
    template <std::size_t SIZE_2>
    constexpr array<SIZE + SIZE_2> operator,(array<SIZE_2> a2)
    {
        array<SIZE + SIZE_2> concat = {0};
        for (std::size_t i = 0; i < SIZE; ++i)
        {
            concat[i] = (*this)[i];
        }
        for (std::size_t i = 0; i < SIZE_2; ++i)
        {
            concat[SIZE + i] = a2[i];
        }
        return concat;
    }

    template <std::size_t Repeats>
    constexpr hid::rdf::array<SIZE * Repeats> repeat() const
    {
        return repeat_array_impl<Repeats, SIZE>(*this, std::make_index_sequence<SIZE * Repeats>{});
    }
};

/// @brief This class stores exactly one HID report descriptor item.
template <byte_type DATA_SIZE>
class short_item : public array<1 + DATA_SIZE>
{
    static_assert((DATA_SIZE <= 4) and (DATA_SIZE != 3));

    using base_t = array<1 + DATA_SIZE>;
    using base_t::data;

  public:
    template <typename TTag>
    constexpr short_item(TTag tag)
        : base_t({static_cast<byte_type>((static_cast<byte_type>(tag) << 4) |
                                         (static_cast<byte_type>(match_type<TTag>()) << 2) |
                                         ((DATA_SIZE == 4) ? 3 : (DATA_SIZE & 3)))})
    {
        static_assert(match_type<TTag>() != item_type::RESERVED);
    }

    template <typename TTag, typename TData>
    constexpr short_item(TTag tag, TData val)
        : short_item(tag)
    {
        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        auto value = static_cast<std::uint32_t>(val);
        for (byte_type i = 0; i < DATA_SIZE; ++i)
        {
            (*this)[1 + i] = static_cast<byte_type>(value);
            value >>= 8;
        }
    }

    template <typename T>
    bool operator==(const item& rhs)
    {
        return std::equal(this->begin(), this->end(), &rhs.header());
    }

    template <typename T>
    bool operator!=(const item& rhs)
    {
        return !(*this == rhs);
    }

    template <typename T>
    constexpr bool operator==(const short_item_buffer& rhs)
    {
        if ((*this)[0] != rhs.header())
        {
            return false;
        }
        return std::equal(this->begin() + 1, this->end(), rhs.data());
    }

    template <typename T>
    constexpr bool operator!=(const short_item_buffer& rhs)
    {
        return !(*this == rhs);
    }

    [[nodiscard]] constexpr std::uint32_t value_unsigned() const
    {
        auto header = std::bit_cast<item_header>(*data());
        return item_header::get_unsigned_value(&header, data() + 1);
    }

    [[nodiscard]] constexpr std::int32_t value_signed() const
    {
        auto header = std::bit_cast<item_header>(*data());
        return item_header::get_signed_value(&header, data() + 1);
    }
};

} // namespace hid::rdf
