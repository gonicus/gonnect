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
#ifndef __HID_RDF_REPORT_PROTOCOL_HPP_
#define __HID_RDF_REPORT_PROTOCOL_HPP_

#include <algorithm>
#include <array>
#include "hid/rdf/global_items.hpp"
#include "hid/rdf/parser.hpp"

namespace hid
{
struct report_protocol_properties
{
    using descriptor_view_type = rdf::ce_descriptor_view;
    using size_type = std::uint16_t;

    size_type max_input_size{};
    size_type max_output_size{};
    size_type max_feature_size{};
    report::id::type max_input_id{};
    report::id::type max_output_id{};
    report::id::type max_feature_id{};

    constexpr size_type max_report_size() const
    {
        return std::max(max_input_size, std::max(max_output_size, max_feature_size));
    }
    constexpr size_type max_report_size(report::type type) const
    {
        switch (type)
        {
        case report::type::INPUT:
            return max_input_size;
        case report::type::OUTPUT:
            return max_output_size;
        case report::type::FEATURE:
            return max_feature_size;
        default:
            return 0;
        }
    }

    constexpr bool uses_report_ids() const { return max_report_id() >= report::id::min(); }
    constexpr report::id::type max_report_id() const
    {
        return std::max(max_input_id, std::max(max_output_id, max_feature_id));
    }
    constexpr report::id::type max_report_id(report::type type) const
    {
        switch (type)
        {
        case report::type::INPUT:
            return max_input_id;
        case report::type::OUTPUT:
            return max_output_id;
        case report::type::FEATURE:
            return max_feature_id;
        default:
            return 0;
        }
    }

    /// @brief Define the report protocol properties, with no report ID use.
    /// @param max_input_size: The size of the longest INPUT report in bytes, including the report
    /// ID (if used)
    /// @param max_output_size: The size of the longest OUTPUT report in bytes, including the report
    /// ID (if used)
    /// @param max_feature_size: The size of the longest FEATURE report in bytes, including the
    /// report ID (if used)
    /// @param max_report_id: The highest used report ID (or 0 if IDs are not used)
    constexpr report_protocol_properties(size_type max_input_size, size_type max_output_size,
                                         size_type max_feature_size)
        : max_input_size(max_input_size),
          max_output_size(max_output_size),
          max_feature_size(max_feature_size)
    {}

    /// @brief Define the report protocol properties manually.
    /// @param desc_view: View of the HID report descriptor
    /// @param max_input_size: The size of the longest INPUT report in bytes, including the report
    /// ID (if used)
    /// @param max_output_size: The size of the longest OUTPUT report in bytes, including the report
    /// ID (if used)
    /// @param max_feature_size: The size of the longest FEATURE report in bytes, including the
    /// report ID (if used)
    /// @param max_report_id: The highest used report ID (or 0 if IDs are not used)
    constexpr report_protocol_properties(size_type max_input_size, size_type max_output_size,
                                         size_type max_feature_size, report::id::type max_input_id,
                                         report::id::type max_output_id,
                                         report::id::type max_feature_id)
        : max_input_size(max_input_size),
          max_output_size(max_output_size),
          max_feature_size(max_feature_size),
          max_input_id(max_input_id),
          max_output_id(max_output_id),
          max_feature_id(max_feature_id)
    {}

    /// @brief Define the report protocol properties by parsing the descriptor in compile-time.
    /// @param desc_view: View of the HID report descriptor
    consteval report_protocol_properties(const descriptor_view_type& desc_view)
        : report_protocol_properties(parser(desc_view).max_report_size(report::type::INPUT),
                                     parser(desc_view).max_report_size(report::type::OUTPUT),
                                     parser(desc_view).max_report_size(report::type::FEATURE),
                                     parser(desc_view).max_report_id(report::type::INPUT),
                                     parser(desc_view).max_report_id(report::type::OUTPUT),
                                     parser(desc_view).max_report_id(report::type::FEATURE))
    {}

    /// @brief This class parses the HID report descriptor, gathering all report size
    ///        and TLC assignment information, and verifying that the descriptor describes
    ///        a valid HID protocol.
    class parser : public rdf::parser<descriptor_view_type::iterator>
    {
      public:
        using base = rdf::parser<descriptor_view_type::iterator>;
        using item_type = base::item_type;
        using items_view_type = base::items_view_type;
        using control = base::control;

        constexpr parser(const descriptor_view_type& desc_view)
            : base()
        {
            base::parse_items(desc_view);

            // when finished, run a final check that each report has byte size
            for (const auto& sizes : report_bit_sizes_)
            {
                for (const auto& size : sizes)
                {
                    HID_RDF_ASSERT(size % 8 == 0, ex_report_total_size_invalid);
                }
            }
        }

        constexpr size_type max_report_size(report::type type) const
        {
            if (!uses_report_ids())
            {
                return bit_size(type, 0) / 8;
            }
            else
            {
                auto& report_sizes = bit_sizes_by_type(type);
                auto begin = report_sizes.begin();
                auto max_size = *std::max_element(++begin, report_sizes.end()) / 8;
                if (max_size > 0)
                {
                    return sizeof(report::id) + max_size;
                }
                else
                {
                    return 0;
                }
            }
        }

        constexpr bool uses_report_ids() const { return max_report_id() > 0; }

        constexpr report::id::type max_report_id() const
        {
            return *std::max_element(max_report_ids_.begin(), max_report_ids_.end());
        }

        constexpr report::id::type max_report_id(report::type type) const
        {
            return max_report_ids_[static_cast<size_t>(type) - 1];
        }

      private:
        constexpr control parse_report_data_field(
            const item_type& main_item, const rdf::global_item_store& global_state,
            [[maybe_unused]] const items_view_type& main_section, unsigned tlc_count) override
        {
            using namespace hid::rdf;
            report::type rtype = main::tag_to_report_type(main_item.main_tag());

            // get report ID (or use 0 if not present)
            report::id::type report_id = 0;
            const auto* report_id_item = global_state.get_item(global::tag::REPORT_ID);
            if (report_id_item != nullptr)
            {
                report_id = report_id_item->value_unsigned();

                // report ID verification
                HID_RDF_ASSERT(report_id >= report::id::min(), ex_report_id_zero);
                HID_RDF_ASSERT(report_id <= report::id::max(), ex_report_id_excess);
                if (!uses_report_ids())
                {
                    for (auto& sizes : report_bit_sizes_)
                    {
                        HID_RDF_ASSERT(sizes[0] == 0, ex_report_id_missing);
                    }
                }
                auto& rid = max_report_ids_[static_cast<size_t>(rtype) - 1];
                rid = std::max(rid, report_id);
            }

            // get the items defining the size of this/these report data elements
            const auto* report_size_item = global_state.get_item(global::tag::REPORT_SIZE);
            HID_RDF_ASSERT(report_size_item != nullptr, ex_report_size_missing);
            auto report_size = report_size_item->value_unsigned();
            HID_RDF_ASSERT(report_size > 0, ex_report_size_zero);

            const auto* report_count_item = global_state.get_item(global::tag::REPORT_COUNT);
            HID_RDF_ASSERT(report_count_item != nullptr, ex_report_count_missing);
            auto report_count = report_count_item->value_unsigned();
            HID_RDF_ASSERT(report_count > 0, ex_report_count_zero);

            // increase size of this report
            bit_size(rtype, report_id) += report_size * report_count;

            // verify that the report doesn't cross TLC boundary
            auto& report_tlc_index = tlc_index(rtype, report_id);
            if (report_tlc_index == 0)
            {
                // first piece of the report, assign to TLC now
                report_tlc_index = tlc_count;
            }
            else
            {
                HID_RDF_ASSERT(report_tlc_index == tlc_count, ex_report_crossing_tlc_bounds);
            }
            return control::CONTINUE;
        }

        constexpr size_type& bit_size(report::type rt, report::id::type id)
        {
            return bit_sizes_by_type(rt)[id];
        }
        constexpr const size_type& bit_size(report::type rt, report::id::type id) const
        {
            return bit_sizes_by_type(rt)[id];
        }
        constexpr std::array<size_type, report::id::max()>& bit_sizes_by_type(report::type rt)
        {
            return report_bit_sizes_[static_cast<report::id::type>(rt) - 1];
        }
        constexpr const std::array<size_type, report::id::max()>&
        bit_sizes_by_type(report::type rt) const
        {
            return report_bit_sizes_[static_cast<report::id::type>(rt) - 1];
        }

        constexpr unsigned& tlc_index(report::type rt, report::id::type id)
        {
            return tlc_indexes_by_type(rt)[id];
        }
        constexpr const unsigned& tlc_index(report::type rt, report::id::type id) const
        {
            return tlc_indexes_by_type(rt)[id];
        }
        constexpr std::array<unsigned, report::id::max()>& tlc_indexes_by_type(report::type rt)
        {
            return report_tlc_indexes_[static_cast<report::id::type>(rt) - 1];
        }
        constexpr const std::array<unsigned, report::id::max()>&
        tlc_indexes_by_type(report::type rt) const
        {
            return report_tlc_indexes_[static_cast<report::id::type>(rt) - 1];
        }

        std::array<std::array<size_type, report::id::max()>, 3> report_bit_sizes_{};
        std::array<std::array<unsigned, report::id::max()>, 3> report_tlc_indexes_{};
        std::array<report::id::type, 3> max_report_ids_{};
    };
};

/// @brief This class holds the necessary information about the specific HID report protocol
///        that the device implements. This information is used by the transport layer
///        to establish a proper size data channel between the host and the device.
///        This class can gather all parameters from the HID report descriptor at compile-time,
///        and also performs verification of the descriptor (compiler error when the descriptor is
///        faulty).
struct report_protocol : public report_protocol_properties
{
    descriptor_view_type descriptor;

    /// @brief Define the report protocol manually.
    /// @param desc_view: View of the HID report descriptor
    /// @param args: Arguments for report_protocol_properties constructor
    template <typename... TArgs>
    constexpr report_protocol(const descriptor_view_type& desc_view, TArgs&&... args)
        : report_protocol_properties(std::forward<TArgs>(args)...), descriptor(desc_view)
    {}

    /// @brief Define the report protocol by parsing the descriptor in compile-time.
    /// @param desc_view: View of the HID report descriptor
    consteval report_protocol(const descriptor_view_type& desc_view)
        : report_protocol_properties(desc_view), descriptor(desc_view)
    {}
};
} // namespace hid

#endif // __HID_RDF_REPORT_PROTOCOL_HPP_
