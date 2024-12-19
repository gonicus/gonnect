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
#ifndef __HID_RDF_SHORT_ITEM_HPP_
#define __HID_RDF_SHORT_ITEM_HPP_

#include "hid/rdf/item.hpp"

namespace hid::rdf
{
/// @brief A byte array that concatenates with another using the comma operator.
template <std::size_t SIZE>
class array : public std::array<byte_type, SIZE>
{
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
};

/// @brief This class stores exactly one HID report descriptor item.
template <byte_type DATA_SIZE>
class short_item : public array<1 + DATA_SIZE>
{
    static_assert((DATA_SIZE <= 4) and (DATA_SIZE != 3));

    using base_t = array<1 + DATA_SIZE>;

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
    constexpr short_item(TTag tag, TData data)
        : short_item(tag)
    {
        auto d = static_cast<std::uint32_t>(data);
        for (byte_type i = 0; i < DATA_SIZE; ++i)
        {
            (*this)[1 + i] = static_cast<byte_type>(d);
            d >>= 8;
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
        else
        {
#if __cplusplus > 201703L
            return std::equal(this->begin() + 1, this->end(), rhs.data());
#else
            for (std::size_t i = 0; i < this->data_size(); ++i)
            {
                if ((*this)[1 + i] != rhs.data()[i])
                {
                    return false;
                }
            }
            return true;
#endif
        }
    }

    template <typename T>
    constexpr bool operator!=(const short_item_buffer& rhs)
    {
        return !(*this == rhs);
    }
};

} // namespace hid::rdf

#endif // __HID_RDF_SHORT_ITEM_HPP_
