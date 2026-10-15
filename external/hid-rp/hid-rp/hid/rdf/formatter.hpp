// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <format>
#include <ostream>
#include "hid/page/all.hpp"
#include "hid/rdf/descriptor_view.hpp"
#include "hid/rdf/main_items.hpp"
#include "hid/rdf/parser.hpp"
#include "hid/rdf/unit.hpp"

/// @brief  Formatter specialization for runtime usage types.
template <>
struct std::formatter<hid::usage_t>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        const auto* pos = ctx.begin();
        // if the format's first symbol is p, the usage page is added
        if ((*pos == 'p') or (*pos == 'P'))
        {
            add_page_ = true;
            ctx.advance_to(pos + 1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        // the rest will be used for numeric formatting, if the usage is not named
        return numeric_.parse(ctx);
    }

    template <typename FormatContext>
    FormatContext::iterator format(const hid::usage_t& usage, FormatContext& ctx) const
    {
        auto info = hid::page::get_page_info(usage.page_id());
        // first the page is printed, if requested
        if (add_page_)
        {
            if (info.valid_page())
            {
                format_to(ctx.out(), "{} / ", info.page_name);
            }
            else
            {
                format_to(ctx.out(), "{}({:#06x}) / ", info.page_name, usage.page_id());
            }
        }
        // second the base usage is printed
        auto value = usage.id() & ~info.ius_mask;
        if (const char* name = info.get_usage_name(value); name)
        {
            vformat_to(ctx.out(), name, make_format_args(value));
        }
        else
        {
            numeric_.format(value, ctx);
        }
        // finally the inline usage switch is printed, if any
        if (const char* name = info.get_usage_name(usage.id() & info.ius_mask); name)
        {
            format_to(ctx.out(), " {}", name);
        }
        return ctx.out();
    }

  private:
    std::formatter<hid::usage_id_t> numeric_;
    bool add_page_{};
};

inline std::ostream& operator<<(std::ostream& os, const hid::usage_t& v)
{
    os << std::format("Usage({:p})", v);
    return os;
}

/// @brief  Formatter specialization for compile-time usage types.
/// @tparam T: usage concept type
template <hid::UsageType T>
struct std::formatter<T> : std::formatter<hid::usage_t>
{};

template <hid::UsageType T>
inline std::ostream& operator<<(std::ostream& os, const T& v)
{
    os << std::format("Usage({:p})", v);
    return os;
}

/// @brief  Formatter specialization for HID main item field types.
template <>
struct std::formatter<hid::rdf::main::field_type>
{
    constexpr static auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    FormatContext::iterator format(const hid::rdf::main::field_type& field,
                                   FormatContext& ctx) const
    {
        using namespace hid::rdf;
        format_to(ctx.out(), "{},{},{}",
                  (field & main::data_field_flag::CONSTANT ? "Constant" : "Data"),
                  (field & main::data_field_flag::VARIABLE ? "Variable" : "Array"),
                  (field & main::data_field_flag::RELATIVE ? "Relative" : "Absolute"));
        if (field & main::data_field_flag::WRAP)
        {
            format_to(ctx.out(), ",Wrap");
        }
        if (field & main::data_field_flag::NONLINEAR)
        {
            format_to(ctx.out(), ",Nonlinear");
        }
        if (field & main::data_field_flag::NO_PREFERRED)
        {
            format_to(ctx.out(), ",NoPreferred");
        }
        if (field & main::data_field_flag::NULL_STATE)
        {
            format_to(ctx.out(), ",NullState");
        }
        if (field & main::data_field_flag::VOLATILE)
        {
            format_to(ctx.out(), ",Volatile");
        }
        if (field & main::data_field_flag::BUFFERED_BYTES)
        {
            format_to(ctx.out(), ",BufferedBytes");
        }
        return ctx.out();
    }
};

/// @brief  Formatter specialization for HID collection types.
template <>
struct std::formatter<hid::rdf::main::collection_type> : std::formatter<string_view>
{
    template <typename FormatContext>
    FormatContext::iterator format(const hid::rdf::main::collection_type& collection,
                                   FormatContext& ctx) const
    {
        using namespace hid::rdf;
        const char* name = nullptr;
        switch (collection)
        {
        case main::collection_type::PHYSICAL:
            name = "Physical";
            break;
        case main::collection_type::APPLICATION:
            name = "Application";
            break;
        case main::collection_type::LOGICAL:
            name = "Logical";
            break;
        case main::collection_type::REPORT:
            name = "Report";
            break;
        case main::collection_type::NAMED_ARRAY:
            name = "Named Array";
            break;
        case main::collection_type::USAGE_SWITCH:
            name = "Usage Switch";
            break;
        case main::collection_type::USAGE_MODIFIER:
            name = "Usage Modifier";
            break;
        default:
            break;
        }
        if (name)
        {
            return std::formatter<string_view>::format(name, ctx);
        }
        return format_to(ctx.out(), "Unknown({:#x})",
                         static_cast<std::underlying_type_t<main::collection_type>>(collection));
    }
};

/// @brief  Formatter specialization for HID unit codes.
template <>
struct std::formatter<hid::rdf::unit::code> : std::formatter<string_view>
{
    template <typename FormatContext>
    FormatContext::iterator format(const hid::rdf::unit::code& unit_code, FormatContext& ctx) const
    {
        using namespace hid::rdf::unit;
        const char* name = nullptr;
        switch (unit_code)
        {
        case code::CENTIMETER:
            name = "(centi)meter";
            break;
        case code::RADIAN:
            name = "radian";
            break;
        case code::INCH:
            name = "inch";
            break;
        case code::DEGREE:
            name = "degree";
            break;
        case code::GRAM:
            name = "gram";
            break;
        case code::SLUG:
            name = "slug";
            break;
        case code::SECOND:
            name = "second";
            break;
        case code::KELVIN:
            name = "kelvin";
            break;
        case code::FAHRENHEIT:
            name = "fahrenheit";
            break;
        case code::AMPERE:
            name = "ampere";
            break;
        case code::CANDELA:
            name = "candela";
            break;
        case code::NEWTON:
            name = "newton";
            break;
        case code::METER_PER_SECOND:
            name = "meter per second";
            break;
        case code::METER_PER_SECOND2:
            name = "meter per second squared";
            break;
        case code::PASCAL:
            name = "pascal";
            break;
        case code::JOULE:
            name = "joule";
            break;
        case code::HERTZ:
            name = "hertz";
            break;
        case code::DEGREE_PER_SECOND:
            name = "degree per second";
            break;
        case code::DEGREE_PER_SECOND2:
            name = "degree per second squared";
            break;
        case code::RADIAN_PER_SECOND:
            name = "radian per second";
            break;
        case code::RADIAN_PER_SECOND2:
            name = "radian per second squared";
            break;
        case code::WATT:
            name = "watt";
            break;
        case code::AMPERE_PER_SECOND:
            name = "ampere per second";
            break;
        case code::COULOMB:
            name = "coulomb";
            break;
        case code::FARAD:
            name = "farad";
            break;
        case code::HENRY:
            name = "henry";
            break;
        case code::LUX:
            name = "lux";
            break;
        case code::OHM:
            name = "ohm";
            break;
        case code::SIEMENS:
            name = "siemens";
            break;
        case code::TESLA:
            name = "tesla";
            break;
        case code::VOLT:
            name = "volt";
            break;
        case code::WEBER:
            name = "weber";
            break;
        default:
            break;
        }
        if (name)
        {
            return std::formatter<string_view>::format(name, ctx);
        }
        return format_to(ctx.out(), "unknown({:#x})",
                         static_cast<std::underlying_type_t<hid::rdf::unit::code>>(unit_code));
    }
};

/// @brief Formatter specialization for HID report descriptors.
/// @tparam TIterator: descriptor view iterator type
/// ( @ref hid::rdf::reinterpret_iterator or @ref hid::rdf::copy_iterator)
template <typename TIterator>
struct std::formatter<hid::rdf::descriptor_view_base<TIterator>>
{
    using descriptor_view_type = hid::rdf::descriptor_view_base<TIterator>;

    constexpr auto parse(std::format_parse_context& ctx)
    {
#if 0
        if ((ctx.begin() != ctx.end()) and
            (std::from_chars(ctx.begin(), ctx.end(), width_).ec != std::errc{}))
        {
            throw std::format_error("Invalid format specifier");
        }
#endif
        return ctx.begin();
    }

    template <typename FormatContext>
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    FormatContext::iterator format(const descriptor_view_type& desc, FormatContext& ctx) const
    {
        using namespace hid::rdf;
        // A parsing formatter is used to manage the descriptor collection indentation,
        // and the proper usage page tracking. When parsing fails,
        // the remaining items are printed without indentation and without usage page info.
        struct parsing_formatter : public hid::rdf::parser<TIterator>
        {
            using base = hid::rdf::parser<TIterator>;
            using item_type = base::item_type;
            using items_view_type = base::items_view_type;
            using control = base::control;
            using base::get_usage;

            FormatContext& ctx_;
            unsigned width_;
            unsigned collection_depth_{};
            TIterator it_;

            parsing_formatter(const descriptor_view_type& desc_view, FormatContext& ctx,
                              unsigned width)
                : base(), ctx_(ctx), width_(width), it_(desc_view.begin())
            {
                // if bounds are invalid, exit early
                HID_RP_ASSERT(desc_view.has_valid_bounds(), ex_invalid_bounds);
#if defined(__EXCEPTIONS) or defined(_CPPUNWIND)
                try
#endif
                {
                    it_ = base::parse_items(desc_view);
                }
#if defined(__EXCEPTIONS) or defined(_CPPUNWIND)
                catch (const hid::rdf::exception& e)
                {
                    format_to(ctx_.out(), "Structural error: {}", e.what());
                }
#endif
                // the remaining part (i.e. after final EndCollection or after error)
                // is printed without usage page tracking and proper indentation
                collection_depth_ = 0;
                format_items(items_view_type{it_, desc_view.end()});
            }

            void format_items(const items_view_type& main_section,
                              const global_item_store& global_state = {},
                              std::uint16_t main_data_field = 0)
            {
                for (const auto& item : main_section)
                {
                    auto value_signed = item.value_signed();
                    auto value_unsigned = item.value_unsigned();
                    auto* tag_name = item.tag_name();

                    format_to(
                        ctx_.out(), "{}",
                        std::string(static_cast<std::string::size_type>(width_) * collection_depth_,
                                    ' '));
                    switch (item.unified_tag())
                    {
                    case tag::INPUT:
                    case tag::OUTPUT:
                    case tag::FEATURE:
                        format_to(ctx_.out(), "{}({})\n", tag_name,
                                  hid::rdf::main::field_type(main_data_field));
                        break;
                    case tag::COLLECTION:
                        format_to(ctx_.out(), "{}({})\n", tag_name,
                                  hid::rdf::main::collection_type(value_unsigned));
                        break;
                    case tag::USAGE_PAGE:
                        if (auto info = hid::page::get_page_info(value_unsigned); info.valid_page())
                        {
                            format_to(ctx_.out(), "{}({})\n", tag_name, info.page_name);
                        }
                        else
                        {
                            format_to(ctx_.out(), "{}({:#06x})\n", tag_name, value_unsigned);
                        }
                        break;
                    case tag::UNIT:
                        format_to(ctx_.out(), "{}({})\n", tag_name,
                                  *((hid::rdf::unit::code*)&value_unsigned));
                        break;
                    case tag::UNIT_EXPONENT:
                        value_signed = unit::get_exponent(item);
                        [[fallthrough]];
                    case tag::LOGICAL_MINIMUM:
                    case tag::PHYSICAL_MINIMUM:
                    case tag::PHYSICAL_MAXIMUM:
                        format_to(ctx_.out(), "{}({})\n", tag_name, value_signed);
                        break;
                    case tag::LOGICAL_MAXIMUM:
                        if (main_data_field & main::data_field_flag::VARIABLE)
                        {
                            format_to(ctx_.out(), "{}({})\n", tag_name, value_signed);
                            break;
                        }
                        [[fallthrough]];
                    case tag::REPORT_SIZE:
                    case tag::REPORT_ID:
                    case tag::REPORT_COUNT:
                    case tag::DESIGNATOR_INDEX:
                    case tag::DESIGNATOR_MINIMUM:
                    case tag::DESIGNATOR_MAXIMUM:
                    case tag::STRING_INDEX:
                    case tag::STRING_MINIMUM:
                    case tag::STRING_MAXIMUM:
                        format_to(ctx_.out(), "{}({})\n", tag_name, value_unsigned);
                        break;
                    case tag::END_COLLECTION:
                    case tag::PUSH:
                    case tag::POP:
                        format_to(ctx_.out(), "{}()\n", tag_name);
                        break;
                    case tag::USAGE:
                    case tag::USAGE_MINIMUM:
                    case tag::USAGE_MAXIMUM:
#if defined(__EXCEPTIONS) or defined(_CPPUNWIND)
                        try
#endif
                        {
                            auto usage = get_usage(item, global_state);
                            if (item.data_size() == 4)
                            {
                                format_to(ctx_.out(), "{}({:p})\n", tag_name, usage);
                            }
                            else
                            {
                                format_to(ctx_.out(), "{}({})\n", tag_name, usage);
                            }
                        }
#if defined(__EXCEPTIONS) or defined(_CPPUNWIND)
                        catch (const hid::rdf::exception& e)
                        {
                            format_to(ctx_.out(), "{} error: {} ({})\n", tag_name, e.what(),
                                      value_unsigned);
                            continue;
                        }
#endif
                        break;
                    case tag::DELIMITER:
                        if (value_unsigned == 0)
                        {
                            format_to(ctx_.out(), "Delimiter(Open)\n");
                        }
                        else if (value_unsigned == 1)
                        {
                            format_to(ctx_.out(), "Delimiter(Close)\n");
                        }
                        else
                        {
                            format_to(ctx_.out(), "Delimiter({})\n", value_unsigned);
                        }
                        break;
                    default:
                        format_to(ctx_.out(), "Unknown({:#04x})\n",
                                  std::bit_cast<uint8_t>((item_header)item));
                        break;
                    }
                }
            }

            control parse_collection_begin([[maybe_unused]] main::collection_type collection,
                                           const global_item_store& global_state,
                                           const items_view_type& main_section,
                                           [[maybe_unused]] unsigned tlc_count) override
            {
                it_ = ++main_section.end(); // include the main item in the section as well
                format_items({main_section.begin(), it_}, global_state);
                collection_depth_++;
                return control::CONTINUE;
            }
            control parse_collection_end(const global_item_store& global_state,
                                         const items_view_type& main_section,
                                         [[maybe_unused]] unsigned tlc_number) override
            {
                collection_depth_--;
                it_ = ++main_section.end(); // include the main item in the section as well
                format_items({main_section.begin(), it_}, global_state);
                return control::CONTINUE;
            }
            control parse_report_data_field([[maybe_unused]] const item_type& main_item,
                                            const global_item_store& global_state,
                                            const items_view_type& main_section,
                                            [[maybe_unused]] unsigned tlc_count) override
            {
                it_ = ++main_section.end(); // include the main item in the section as well
                format_items({main_section.begin(), it_}, global_state, main_item.value_unsigned());
                return control::CONTINUE;
            }
        };

        parsing_formatter fmt{desc, ctx, width_};
        return ctx.out();
    }

  private:
    unsigned width_{4};
};
