// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <iterator>
#include <span>
#include "hid/rdf/item.hpp"
#include "make_static.hpp"

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

    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    constexpr reinterpret_iterator()
        : ptr_()
    {}
    constexpr reinterpret_iterator(const byte_type* data)
        : ptr_(data)
    {}
    reinterpret_iterator(pointer ptr) // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        : ptr_(reinterpret_cast<decltype(ptr_)>(ptr))
    {}
    reinterpret_iterator& operator++()
    {
        ptr_ += (*this)->size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
    constexpr bool operator==(const reinterpret_iterator& rhs) const = default;
    constexpr bool operator!=(const reinterpret_iterator& rhs) const = default;

  private:
    pointer ptr()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<pointer>(ptr_);
    }

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

    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    constexpr copy_iterator()
        : ptr_()
    {}
    constexpr copy_iterator(const byte_type* data)
        : ptr_(data)
    {}
    copy_iterator(pointer ptr) // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        : copy_iterator(reinterpret_cast<decltype(ptr_)>(ptr))
    {}
    constexpr copy_iterator& operator++()
    {
        ptr_ += (*this)->size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
    short_item_buffer copy_;
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

    [[nodiscard]] constexpr const byte_type* data() const { return begin_; }
    [[nodiscard]] constexpr auto size() const { return static_cast<std::size_t>(end_ - begin_); }
    [[nodiscard]] constexpr iterator begin() { return begin_; }
    [[nodiscard]] constexpr const_iterator begin() const { return begin_; }
    [[nodiscard]] constexpr iterator end() { return end_; }
    [[nodiscard]] constexpr const_iterator end() const { return end_; }

    [[nodiscard]] constexpr auto to_span() const
    {
        return std::span<const byte_type>(data(), size());
    }

    /// @brief  Verifies that the view has correct bounds, all items are intact and complete.
    ///         This is the first check that needs to be done on a new HID report descriptor
    ///         (usually done by the OS itself).
    /// @return true if the view is valid, false otherwise
    [[nodiscard]] constexpr bool has_valid_bounds() const
    {
        for (auto it = this->begin(); it != this->end(); ++it)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if ((it.ptr_ + (*it).size()) > end_)
            {
                return false;
            }
        }
        return true;
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
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        : base(data, data + size)
    {}
    template <typename TArray>
    constexpr descriptor_view_base(const TArray& arr)
        : base(arr.data(), arr.data() + arr.size())
    {}
    template <typename TIter>
    constexpr descriptor_view_base(const TIter begin, const TIter end)
        : base(std::addressof(*begin), std::addressof(*begin) + std::distance(begin, end))
    {}
    /// @brief  This method constructs a @ref hid::rdf::descriptor_view_base object from a rvalue
    ///         descriptor, producing a static lvalue of it in the process.
    /// @tparam Data: the descriptor array, acquired e.g. from a @ref hid::rdf::descriptor call
    template <auto Data>
    static constexpr auto from_descriptor()
    {
        return descriptor_view_base(make_static<Data>());
    }
};

/// @brief HID report descriptor view, use for runtime descriptor parsing.
using descriptor_view = descriptor_view_base<reinterpret_iterator>;
static_assert(std::ranges::range<descriptor_view>);

/// @brief HID report descriptor view, use for compile-time descriptor parsing.
using ce_descriptor_view = descriptor_view_base<copy_iterator>;
static_assert(std::ranges::range<ce_descriptor_view>);

} // namespace hid::rdf
