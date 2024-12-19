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
#ifndef __HID_RDF_DESCRIPTOR_VIEW_HPP_
#define __HID_RDF_DESCRIPTOR_VIEW_HPP_

#include <iterator>
#include <span>
#include "hid/rdf/item.hpp"

namespace hid::rdf
{
/// @brief HID report descriptor iterator, which casts the underlying raw data to an item.
class reinterpret_iterator
{
  public:
    using value_type = item;
    using const_pointer = const value_type*;
    using const_reference = const value_type&;
    using pointer = const value_type*;
    using reference = const value_type&;

    using iterator_category = std::input_iterator_tag;

    constexpr reinterpret_iterator(const byte_type* data)
        : ptr_(data)
    {}
    reinterpret_iterator(pointer ptr)
        : ptr_(reinterpret_cast<decltype(ptr_)>(ptr))
    {}
    reinterpret_iterator& operator++()
    {
        ptr_ += (*this)->size();
        return *this;
    }
    reinterpret_iterator operator++(int)
    {
        reinterpret_iterator retval = *this;
        ++(*this);
        return retval;
    }
    reference operator*() { return *ptr(); }
    pointer operator->() { return ptr(); }
    constexpr bool operator==(const reinterpret_iterator& rhs) const { return ptr_ == rhs.ptr_; }
    constexpr bool operator!=(const reinterpret_iterator& rhs) const { return !(*this == rhs); }

  private:
    pointer ptr() { return reinterpret_cast<pointer>(ptr_); }

    template <typename TIterator>
    friend class items_view_base;

    const byte_type* ptr_;
};

/// @brief HID report descriptor iterator, which copies the actual short (!) item
///        into an internal buffer. This approach is mandatory for constexpr use,
///        but suboptimal in runtime to @ref reinterpret_iterator.
class copy_iterator
{
  public:
    using value_type = short_item_buffer;
    using const_pointer = const value_type*;
    using const_reference = const value_type&;
    using pointer = const value_type*;
    using reference = const value_type&;

    using iterator_category = std::input_iterator_tag;

    constexpr copy_iterator(const byte_type* data)
        : ptr_(data)
    {}
    copy_iterator(pointer ptr)
        : copy_iterator(reinterpret_cast<decltype(ptr_)>(ptr))
    {}
    constexpr copy_iterator& operator++()
    {
        ptr_ += (*this)->size();
        return *this;
    }
    constexpr copy_iterator operator++(int)
    {
        copy_iterator retval = *this;
        ++(*this);
        return retval;
    }
    constexpr reference operator*() { return copy(); }
    constexpr pointer operator->() { return &copy(); }
    constexpr bool operator==(const copy_iterator& rhs) const { return ptr_ == rhs.ptr_; }
    constexpr bool operator!=(const copy_iterator& rhs) const { return !(*this == rhs); }

  private:
    constexpr reference copy()
    {
        copy_ = decltype(copy_)(ptr_);
        return copy_;
    }

    template <typename TIterator>
    friend class items_view_base;

    const byte_type* ptr_;
    short_item_buffer copy_{};
};

/// @brief A view to a section of items in an HID report descriptor.
/// @tparam TIterator Either @ref reinterpret_iterator or @ref copy_iterator
template <typename TIterator>
class items_view_base
{
  public:
    using value_type = TIterator::value_type;
    using const_pointer = const value_type*;
    using const_reference = const value_type&;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator = TIterator;
    using const_iterator = iterator;

    constexpr const byte_type* data() const { return begin_; }
    constexpr std::size_t size() const { return std::distance(begin_, end_); }
    constexpr iterator begin() { return begin_; }
    constexpr const_iterator begin() const { return begin_; }
    constexpr iterator end() { return end_; }
    constexpr const_iterator end() const { return end_; }

    operator std::span<const uint8_t>() const { return {data(), size()}; }

    /// @brief  Verifies that the view has correct bounds, all items are intact and complete.
    ///         This is the first check that needs to be done on a new HID report descriptor
    ///         (usually done by the OS itself).
    /// @return true if the view is valid, false otherwise
    constexpr bool has_valid_bounds() const
    {
        for (auto it = this->begin(); it != this->end(); ++it)
        {
            if ((it.ptr_ + (*it).size()) > end_)
            {
                return false;
            }
        }
        return true;
    }

    /// @brief  Counts the occurrences of a certain tag in the descriptor.
    /// @tparam TTag: tag type
    /// @param  tag: the tag to count
    /// @return number of items having the specified tag
    template <typename TTag>
    constexpr std::size_t tag_count(TTag tag) const
    {
        std::size_t hits = 0;
        for (const value_type& item : (*this))
        {
            if (item.has_tag(tag))
            {
                hits++;
            }
        }
        return hits;
    }

    template <typename TTag, class Compare>
    constexpr iterator tag_value_unsigned_most(TTag tag, Compare comp) const
    {
        iterator result = this->end();
        std::uint32_t most = 0;
        for (iterator it = this->begin(); it != this->end(); ++it)
        {
            const value_type& item = *it;
            if (item.has_tag(tag))
            {
                auto value = item.value_unsigned();
                if ((result == this->end()) or comp(most, value))
                {
                    result = it;
                    most = value;
                }
            }
        }
        return result;
    }

    template <typename TTag, class Compare>
    constexpr iterator tag_value_signed_most(TTag tag, Compare comp) const
    {
        iterator result = this->end();
        std::int32_t most = 0;
        for (iterator it = this->begin(); it != this->end(); ++it)
        {
            const value_type& item = *it;
            if (item.has_tag(tag))
            {
                auto value = item.value_signed();
                if ((result == this->end()) or comp(most, value))
                {
                    result = it;
                    most = value;
                }
            }
        }
        return result;
    }

    constexpr items_view_base(const iterator& begin, const iterator& end)
        : begin_(begin.ptr_), end_(end.ptr_)
    {}

  protected:
    constexpr items_view_base()
        : begin_(nullptr), end_(nullptr)
    {}
    constexpr items_view_base(const byte_type* begin, const byte_type* end)
        : begin_(begin), end_(end)
    {}

  private:
    const byte_type* begin_;
    const byte_type* end_;
};

/// @brief A view of the HID report descriptor, allowing iterating through its items.
/// @tparam TIterator Either @ref reinterpret_iterator or @ref copy_iterator
template <typename TIterator>
class descriptor_view_base : public items_view_base<TIterator>
{
    using base = items_view_base<TIterator>;

  public:
    using value_type = TIterator::value_type;
    using const_pointer = const value_type*;
    using const_reference = const value_type&;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator = TIterator;
    using const_iterator = iterator;

    constexpr descriptor_view_base()
        : base()
    {}
    constexpr descriptor_view_base(const byte_type* data, std::size_t size)
        : descriptor_view_base(data, data + size)
    {}
    template <typename TArray>
    constexpr descriptor_view_base(const TArray& arr)
        : descriptor_view_base(arr.data(), arr.data() + arr.size())
    {}
    template <typename TIter>
    constexpr descriptor_view_base(const TIter begin, const TIter end)
        : descriptor_view_base(std::addressof(*begin),
                               std::addressof(*begin) + std::distance(begin, end))
    {}

  private:
    constexpr descriptor_view_base(const byte_type* begin, const byte_type* end)
        : base(begin, end)
    {}
};

/// @brief HID report descriptor view, use for runtime descriptor parsing.
using descriptor_view = descriptor_view_base<reinterpret_iterator>;

/// @brief HID report descriptor view, use for compile-time descriptor parsing.
using ce_descriptor_view = descriptor_view_base<copy_iterator>;

} // namespace hid::rdf

#endif // __HID_RDF_DESCRIPTOR_VIEW_HPP_
