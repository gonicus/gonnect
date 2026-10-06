// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <array>
#include "hid/rdf/constants.hpp"
#include "hid/rdf/exception.hpp"

namespace hid::rdf
{
/// @brief This base class represents the single byte HID report descriptor item header.
class alignas(1) item_header
{
  public:
    [[nodiscard]] constexpr item_type type() const
    {
        return static_cast<item_type>((prefix_ >> 2) & 0x3);
    }

    /// @note   Long item format is specified, but their use isn't.
    ///         Long items store their properties in subsequent bytes (see @ref long_item)
    [[nodiscard]] constexpr bool is_long() const { return prefix_ == LONG_ITEM_PREFIX; }
    [[nodiscard]] constexpr bool is_short() const { return !is_long(); }

    [[nodiscard]] constexpr bool is_type_valid() const { return type() != item_type::RESERVED; }

    [[nodiscard]] constexpr global::tag global_tag() const
    {
        HID_RP_ASSERT(type() == item_type::GLOBAL, ex_item_invalid_tag_type);
        return static_cast<global::tag>(short_tag());
    }
    [[nodiscard]] constexpr local::tag local_tag() const
    {
        HID_RP_ASSERT(type() == item_type::LOCAL, ex_item_invalid_tag_type);
        return static_cast<local::tag>(short_tag());
    }
    [[nodiscard]] constexpr main::tag main_tag() const
    {
        HID_RP_ASSERT(type() == item_type::MAIN, ex_item_invalid_tag_type);
        return static_cast<main::tag>(short_tag());
    }
    [[nodiscard]] constexpr tag unified_tag() const
    {
        HID_RP_ASSERT(is_short(), ex_item_invalid_tag_type);
        return static_cast<tag>(prefix_ >> 2);
    }
    template <typename TTag>
    [[nodiscard]] constexpr bool has_tag(TTag tag) const
    {
        if constexpr (std::is_same_v<TTag, hid::rdf::tag>)
        {
            return unified_tag() == tag;
        }
        else
        {
            return (match_type<TTag>() == type()) and (static_cast<TTag>(short_tag()) == tag);
        }
    }

    [[nodiscard]] constexpr static std::uint32_t get_unsigned_value(const item_header* header,
                                                                    const byte_type* ptr)
    {
        HID_RP_ASSERT(!header->is_long(), ex_item_long);

        std::uint32_t value = 0;
        for (byte_type i = header->short_data_size(); i > 0;)
        {
            i--;
            value =
                (value << 8) | ptr[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        return value;
    }

    [[nodiscard]] constexpr static std::int32_t get_signed_value(const item_header* header,
                                                                 const byte_type* ptr)
    {
        std::uint32_t uval = get_unsigned_value(header, ptr);
        auto value = static_cast<std::int32_t>(uval);
        const auto size = header->short_data_size();
        if ((size > 0) and (size < 4))
        {
            // sign extend
            auto shift = (8 * size) - 1;
            auto check = uval >> shift;
            if (check > 0)
            {
                value |= static_cast<std::int32_t>(0xffffffff << shift);
            }
        }
        return value;
    }

    [[nodiscard]] constexpr static bool is_signed_value(std::int32_t sval)
    {
        return (sval & (1 << (8 * sizeof(sval) - 1))) != 0;
    }

    [[nodiscard]] constexpr const char* tag_name() const
    {
        switch (unified_tag())
        {
        case tag::INPUT:
            return "Input";
        case tag::OUTPUT:
            return "Output";
        case tag::FEATURE:
            return "Feature";
        case tag::COLLECTION:
            return "Collection";
        case tag::END_COLLECTION:
            return "End Collection";
        case tag::USAGE_PAGE:
            return "Usage Page";
        case tag::LOGICAL_MINIMUM:
            return "Logical Minimum";
        case tag::LOGICAL_MAXIMUM:
            return "Logical Maximum";
        case tag::PHYSICAL_MINIMUM:
            return "Physical Minimum";
        case tag::PHYSICAL_MAXIMUM:
            return "Physical Maximum";
        case tag::UNIT_EXPONENT:
            return "Unit Exponent";
        case tag::UNIT:
            return "Unit";
        case tag::REPORT_SIZE:
            return "Report Size";
        case tag::REPORT_ID:
            return "Report ID";
        case tag::REPORT_COUNT:
            return "Report Count";
        case tag::PUSH:
            return "Push";
        case tag::POP:
            return "Pop";
        case tag::USAGE:
            return "Usage";
        case tag::USAGE_MINIMUM:
            return "Usage Minimum";
        case tag::USAGE_MAXIMUM:
            return "Usage Maximum";
        case tag::DESIGNATOR_INDEX:
            return "Designator Index";
        case tag::DESIGNATOR_MINIMUM:
            return "Designator Minimum";
        case tag::DESIGNATOR_MAXIMUM:
            return "Designator Maximum";
        case tag::STRING_INDEX:
            return "String Index";
        case tag::STRING_MINIMUM:
            return "String Minimum";
        case tag::STRING_MAXIMUM:
            return "String Maximum";
        case tag::DELIMITER:
            return "Delimiter";
        default:
            return nullptr;
        }
    }
    constexpr operator bool() const { return prefix_ != 0; }

  protected:
    template <typename TTag>
    [[nodiscard]] constexpr bool is_correct_type() const
    {
        return match_type<TTag>() == type();
    }

    [[nodiscard]] constexpr byte_type short_tag() const
    {
        return static_cast<byte_type>(prefix_ >> 4);
    }

    [[nodiscard]] constexpr byte_type short_data_size() const
    {
        auto size = prefix_ & 3;
        if (size == 3)
        {
            size = 4;
        }
        return size;
    }

    [[nodiscard]] constexpr bool equals(const item_header& other) const
    {
        return prefix_ == other.prefix_;
    }

    [[nodiscard]] constexpr const byte_type& header() const { return prefix_; }

    constexpr item_header(byte_type prefix)
        : prefix_(prefix)
    {}
    /// @brief Default constructing creates an unknown MAIN item with no data
    constexpr item_header()
        : item_header(0)
    {}

  private:
    template <byte_type DATA_SIZE>
    friend class short_item;

    constexpr static byte_type LONG_ITEM_PREFIX = 0xfe;

    byte_type prefix_;
};

/// @brief This class provides a flat view of an existing HID report descriptor item.
///        Items are variable in length, therefore cannot be represented by any single type.
class alignas(1) item : public item_header
{
    /// @brief This class is a simple layout definition of long items.
    struct alignas(1) long_item
    {
        byte_type prefix_;
        byte_type data_size_;
        byte_type tag_;
        byte_type data_[0]; // NOLINT
    };
    static_assert(sizeof(long_item) == 3);

  public:
    using types = item_type;

    template <typename TTag>
    [[nodiscard]] TTag tag() const
    {
        HID_RP_ASSERT(is_correct_type<TTag>(), ex_item_invalid_tag_type);
        if (is_short()) [[likely]]
        {
            return static_cast<TTag>(short_tag());
        }
        else
        {
            return static_cast<TTag>(as_long().tag_);
        }
    }

    [[nodiscard]] constexpr byte_type data_size() const
    {
        if (is_short()) [[likely]]
        {
            return short_data_size();
        }
        else
        {
            return as_long().data_size_;
        }
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        if (is_short()) [[likely]]
        {
            return sizeof(item_header) + short_data_size();
        }
        else
        {
            return sizeof(as_long()) + as_long().data_size_;
        }
    }

    [[nodiscard]] constexpr bool has_data() const { return data_size() > 0; }

    [[nodiscard]] constexpr const byte_type* data() const
    {
        return is_short() ? data_ : as_long().data_;
    }

    [[nodiscard]] std::uint32_t value_unsigned() const
    {
        HID_RP_ASSERT(is_short(), ex_item_long);
        return get_unsigned_value(this, data_);
    }

    [[nodiscard]] std::int32_t value_signed() const
    {
        HID_RP_ASSERT(is_short(), ex_item_long);
        return get_signed_value(this, data_);
    }

    bool operator==(const item& rhs)
    {
        if (this->size() != rhs.size())
        {
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return std::equal(&header(), &header() + this->size(), &rhs.header());
    }

    bool operator!=(const item& rhs) { return !(*this == rhs); }

    item(const item&) = delete;
    item& operator=(const item&) = delete;
    item(item&&) = delete;
    item& operator=(item&&) = delete;
    ~item() = default;

  private:
    [[nodiscard]] const long_item& as_long() const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<const long_item*>(this);
    }

    // the actual contents of this class don't cover the possible entire extent of the item,
    // so constructing or copying is senseless
    item() = default;

    friend class short_item_buffer;

    byte_type data_[0]; // NOLINT
};

/// @brief This class is used to store a copy of an existing HID report descriptor short item.
///        It's emphasis on constexpr is in order to be used as a helper class to implement
///        iterating through an HID report descriptor in compile-time.
class alignas(1) short_item_buffer : public item_header
{
  public:
    constexpr short_item_buffer()
        : data_buffer_()
    {
        HID_RP_ASSERT(is_short(), ex_item_long);
    }

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    constexpr short_item_buffer(const byte_type* data)
        : item_header(data[0]), data_buffer_()
    {
        HID_RP_ASSERT(is_short(), ex_item_long);
        std::copy(data + 1, data + 1 + data_size(), data_buffer_.begin());
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    template <typename TTag>
    [[nodiscard]] constexpr TTag tag() const
    {
        HID_RP_ASSERT(is_correct_type<TTag>(), ex_item_invalid_tag_type);
        return static_cast<TTag>(short_tag());
    }

    [[nodiscard]] constexpr byte_type data_size() const { return short_data_size(); }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return sizeof(item_header) + short_data_size();
    }

    [[nodiscard]] constexpr bool has_data() const { return data_size() > 0; }

    [[nodiscard]] constexpr const byte_type* data() const { return data_buffer_.data(); }

    [[nodiscard]] constexpr std::uint32_t value_unsigned() const
    {
        return get_unsigned_value(this, data_buffer_.data());
    }

    [[nodiscard]] constexpr std::int32_t value_signed() const
    {
        return get_signed_value(this, data_buffer_.data());
    }

    constexpr bool operator==(const short_item_buffer& rhs)
    {
        return this->equals(rhs) and
               std::equal(data_buffer_.begin(), data_buffer_.end(), rhs.data_buffer_.begin());
    }

    constexpr bool operator!=(const short_item_buffer& rhs) { return !(*this == rhs); }

    constexpr short_item_buffer(const item& it)
        : short_item_buffer(&it.header())
    {}
    constexpr short_item_buffer& operator=(const item& it)
    {
        *this = short_item_buffer(it);
        return *this;
    }

  private:
    std::array<byte_type, 4> data_buffer_;
};

} // namespace hid::rdf
