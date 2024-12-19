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
#ifndef __HID_RDF_EXCEPTION_HPP_
#define __HID_RDF_EXCEPTION_HPP_

#include "hid/rdf/constants.hpp"

#ifndef HID_RDF_ASSERT
#if defined(__EXCEPTIONS) // TODO: add other toolchains
#define HID_RDF_ASSERT(CONDITION, EXCEPTION)                                                       \
    {                                                                                              \
        if (!(CONDITION))                                                                          \
        {                                                                                          \
            using namespace hid::rdf;                                                              \
            throw(EXCEPTION());                                                                    \
        }                                                                                          \
    }
#else
#define HID_RDF_ASSERT(CONDITION, EXCEPTION) (void)sizeof(CONDITION)
#endif
#endif

namespace hid::rdf
{
class exception
{
  public:
    using string_type = const char*;

    string_type what() const { return name_; }

  protected:
    constexpr exception(string_type name)
        : name_(name)
    {}

  private:
    string_type const name_;
};

struct ex_item_invalid_tag_type : public exception
{
    constexpr ex_item_invalid_tag_type()
        : exception("invalid item tag type")
    {}
};

class parser_exception : public exception
{
  public:
    using code_type = std::uint16_t;

  protected:
    template <typename TTag, typename TSubcode>
    constexpr parser_exception(string_type name, TTag tag, TSubcode subcode)
        : parser_exception(name, static_cast<code_type>(subcode), static_cast<code_type>(tag),
                           static_cast<code_type>(match_type<TTag>()))
    {
        static_assert(match_type<TTag>() != item_type::RESERVED);
    }

    constexpr parser_exception(string_type name, code_type subcode)
        : parser_exception(name, subcode, 0, 0)
    {}

  private:
    code_type const code_;

    static constexpr code_type ERROR_FLAG = 1 << 15;
    static constexpr code_type VENDOR_DEFINED_FLAG = 1 << 14;

    constexpr parser_exception(string_type name, code_type subcode, code_type tag,
                               code_type tag_type)
        : exception(name),
          code_(ERROR_FLAG | ((static_cast<code_type>(subcode) & 0xcf) << 8) |
                (static_cast<code_type>(tag) << 4) | (static_cast<code_type>(tag_type) << 2))
    {}
};

struct ex_report_id_zero : public parser_exception
{
    constexpr ex_report_id_zero()
        : parser_exception("report ID is zero", global::tag::REPORT_ID, 0)
    {}
};

struct ex_report_id_excess : public parser_exception
{
    constexpr ex_report_id_excess()
        : parser_exception("report ID exceeds valid range", global::tag::REPORT_ID, 1)
    {}
};

struct ex_report_id_missing : public parser_exception
{
    constexpr ex_report_id_missing()
        : parser_exception("report ID missing for a subset of main data items",
                           global::tag::REPORT_ID, 2)
    {}
};

struct ex_report_crossing_tlc_bounds : public parser_exception
{
    constexpr ex_report_crossing_tlc_bounds()
        : parser_exception("data elements of the same report are bound to different TLCs",
                           global::tag::REPORT_ID, 3)
    {}
};

struct ex_item_long : public parser_exception
{
    constexpr ex_item_long()
        : parser_exception("unexpected long item", 0x3e)
    {}
};

struct ex_item_unknown : public parser_exception
{
    constexpr ex_item_unknown()
        : parser_exception("unknown item tag", 0x3f)
    {}
};

struct ex_invalid_bounds : public parser_exception
{
    constexpr ex_invalid_bounds()
        : parser_exception("descriptor has invalid bounds", 0x01)
    {}
};

struct ex_global_stack_overflow : public parser_exception
{
    constexpr ex_global_stack_overflow()
        : parser_exception("global item stack overflow", global::tag::PUSH, 10)
    {}
};

struct ex_push_unmatched : public parser_exception
{
    constexpr ex_push_unmatched()
        : parser_exception("unmatched push item", global::tag::PUSH, 0)
    {}
};

struct ex_push_nonempty : public parser_exception
{
    constexpr ex_push_nonempty()
        : parser_exception("push item contains data", global::tag::PUSH, 1)
    {}
};

struct ex_pop_unmatched : public parser_exception
{
    constexpr ex_pop_unmatched()
        : parser_exception("unmatched pop item", global::tag::POP, 0)
    {}
};

struct ex_pop_nonempty : public parser_exception
{
    constexpr ex_pop_nonempty()
        : parser_exception("pop item contains data", global::tag::POP, 1)
    {}
};

struct ex_report_size_zero : public parser_exception
{
    constexpr ex_report_size_zero()
        : parser_exception("report size must be non-zero", global::tag::REPORT_SIZE, 0)
    {}
};

struct ex_report_size_missing : public parser_exception
{
    constexpr ex_report_size_missing()
        : parser_exception("report size missing", global::tag::REPORT_SIZE, 1)
    {}
};

struct ex_report_count_zero : public parser_exception
{
    constexpr ex_report_count_zero()
        : parser_exception("report count must be non-zero", global::tag::REPORT_COUNT, 0)
    {}
};

struct ex_report_count_missing : public parser_exception
{
    constexpr ex_report_count_missing()
        : parser_exception("report count missing", global::tag::REPORT_COUNT, 1)
    {}
};

struct ex_logical_min_missing : public parser_exception
{
    constexpr ex_logical_min_missing()
        : parser_exception("logical minimum must be defined before any main items",
                           global::tag::LOGICAL_MINIMUM, 1)
    {}
};

struct ex_logical_min_oob : public parser_exception
{
    constexpr ex_logical_min_oob()
        : parser_exception("logical minimum must be within bounds of report size",
                           global::tag::LOGICAL_MINIMUM, 0)
    {}
};

struct ex_logical_max_missing : public parser_exception
{
    constexpr ex_logical_max_missing()
        : parser_exception("logical maximum must be defined before any main items",
                           global::tag::LOGICAL_MAXIMUM, 1)
    {}
};

struct ex_logical_max_oob : public parser_exception
{
    constexpr ex_logical_max_oob()
        : parser_exception("logical maximum must be within bounds of report size",
                           global::tag::LOGICAL_MAXIMUM, 0)
    {}
};

struct ex_logical_limits_crossed : public parser_exception
{
    constexpr ex_logical_limits_crossed()
        : parser_exception("logical min must be less than or equal to max",
                           global::tag::LOGICAL_MAXIMUM, 3)
    {}
};

struct ex_usage_page_zero : public parser_exception
{
    constexpr ex_usage_page_zero()
        : parser_exception("usage page must be non-zero", global::tag::USAGE_PAGE, 0)
    {}
};

struct ex_usage_page_oor : public parser_exception
{
    constexpr ex_usage_page_oor()
        : parser_exception("usage page must not exceed 0xffff", global::tag::USAGE_PAGE, 1)
    {}
};

struct ex_usage_page_missing : public parser_exception
{
    constexpr ex_usage_page_missing()
        : parser_exception("usage page must be defined before any main items",
                           global::tag::USAGE_PAGE, 2)
    {}
};

struct ex_usage_missing : public parser_exception
{
    constexpr ex_usage_missing()
        : parser_exception("usage must be defined before any main items", local::tag::USAGE, 2)
    {}
};

struct ex_collection_missing : public parser_exception
{
    constexpr ex_collection_missing()
        : parser_exception("all other main items must be enclosed by a collection",
                           main::tag::COLLECTION, 10)
    {}
};

struct ex_collection_nested_application : public parser_exception
{
    constexpr ex_collection_nested_application()
        : parser_exception("application collection item inside another collection",
                           main::tag::COLLECTION, 2)
    {}
};

struct ex_collection_begin_unmatched : public parser_exception
{
    constexpr ex_collection_begin_unmatched()
        : parser_exception("unmatched collection item", main::tag::COLLECTION, 0)
    {}
};

struct ex_collection_end_unmatched : public parser_exception
{
    constexpr ex_collection_end_unmatched()
        : parser_exception("unmatched end collection item", main::tag::END_COLLECTION, 0)
    {}
};

struct ex_report_total_size_invalid : public parser_exception
{
    constexpr ex_report_total_size_invalid()
        : parser_exception("all report total sizes must be byte-aligned", main::tag::END_COLLECTION,
                           0)
    {}
};

} // namespace hid::rdf

#endif // __HID_RDF_EXCEPTION_HPP_
