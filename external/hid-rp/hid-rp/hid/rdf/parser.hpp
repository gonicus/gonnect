// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <optional>
#include "hid/rdf/descriptor_view.hpp"
#include "hid/usage.hpp"

namespace hid::rdf
{
/// @brief Efficient storage for global items during descriptor parsing.
class global_item_store
{
  public:
    constexpr global_item_store()
        : items_()
    {}

    constexpr void add_item(const short_item_buffer& new_item)
    {
        auto tag = new_item.global_tag();
        HID_RP_ASSERT(is_tag_stored(tag), ex_item_unknown);
        item_by_tag(tag) = new_item;
    }

    void add_item(const item& new_item)
    {
        auto tag = new_item.global_tag();
        HID_RP_ASSERT(is_tag_stored(tag), ex_item_unknown);
        item_by_tag(tag) = new_item;
    }

    [[nodiscard]] constexpr bool has_item(global::tag tag) const
    {
        return is_tag_stored(tag) and (item_by_tag(tag).type() == item_type::GLOBAL);
    }

    [[nodiscard]] constexpr const short_item_buffer* get_item(global::tag tag) const
    {
        return has_item(tag) ? &item_by_tag(tag) : nullptr;
    }

  private:
    // due to the numbering of global tags, we can efficiently index them in an array
    static_assert(static_cast<byte_type>(global::tag::USAGE_PAGE) == 0);

    // zero-initialized items will have non-global type (used in has_item())
    static_assert(static_cast<byte_type>(item_type::GLOBAL) != 0);

    [[nodiscard]] constexpr static bool is_tag_stored(global::tag tag)
    {
        return (tag <= global::tag::REPORT_COUNT);
    }
    [[nodiscard]] constexpr const short_item_buffer& item_by_tag(global::tag tag) const
    {
        return items_[static_cast<byte_type>(tag)];
    }
    [[nodiscard]] constexpr short_item_buffer& item_by_tag(global::tag tag)
    {
        return items_[static_cast<byte_type>(tag)];
    }

    constexpr static byte_type ITEMS_COUNT = static_cast<byte_type>(global::tag::REPORT_COUNT) + 1;
    std::array<short_item_buffer, ITEMS_COUNT> items_;
};

/// @brief Base class for parsing HID report descriptors. It manages the global state,
/// while subclasses must interpret the the items section at each main item, see
/// @ref parse_collection_begin, @ref parse_collection_end, and @ref parse_report_data_field.
template <typename TIterator, typename TItem = typename TIterator::value_type>
class parser
{
  public:
    using item_type = TItem;
    using items_view_type = items_view_base<TIterator>;
    using descriptor_view_type = descriptor_view_base<TIterator>;

    parser(const parser&) = delete;
    parser& operator=(const parser&) = delete;
    parser(parser&&) = delete;
    parser& operator=(parser&&) = delete;

    /// @brief The control return value allows early termination of parsing,
    ///        in case all relevant information has been gathered early.
    enum class control : std::uint8_t
    {
        CONTINUE = 0, ///< continue parsing
        BREAK = 1,    ///< stop further parsing
    };

    struct report_data_field_params
    {
        report::id id{0};
        std::size_t size{};
        std::size_t count{};
    };

    template <typename T>
    struct limits
    {
        T min;
        T max;
    };

    constexpr virtual ~parser() = default;

    /// @brief  The override of this method is meant to handle the collection begins of the
    /// descriptor.
    /// @param  collection: the type of collection
    /// @param  global_state: the current global items state
    /// @param  main_section: the span of items between the previous and this main item, for local
    /// items parsing
    /// @param  tlc_number: the Top Level Collection index, where this item is found (incremented
    /// before this method is called)
    /// @return CONTINUE to continue the parsing until the next main item,
    ///         or BREAK to terminate it early
    constexpr virtual control
    parse_collection_begin([[maybe_unused]] main::collection_type collection,
                           [[maybe_unused]] const global_item_store& global_state,
                           [[maybe_unused]] const items_view_type& main_section,
                           [[maybe_unused]] unsigned tlc_number)
    {
        return control::CONTINUE;
    }

    /// @brief  The override of this method is meant to handle the collection ends of the
    /// descriptor.
    /// @param  global_state: the current global items state
    /// @param  main_section: the span of items between the previous and this main item, for local
    /// items parsing
    /// @param  tlc_number: the Top Level Collection index, where this item is found
    /// @return CONTINUE to continue the parsing until the next main item,
    ///         or BREAK to terminate it early
    constexpr virtual control
    parse_collection_end([[maybe_unused]] const global_item_store& global_state,
                         [[maybe_unused]] const items_view_type& main_section,
                         [[maybe_unused]] unsigned tlc_number)
    {
        return control::CONTINUE;
    }

    /// @brief  The override of this function is meant to handle the main data field items
    ///         (INPUT/OUTPUT/FEATURE) of the descriptor.
    /// @param  main_item: the current main item that needs parsing
    /// @param  global_state: the current global items state
    /// @param  main_section: the span of items between the previous and this main item, for local
    /// items parsing
    /// @param  tlc_number: the Top Level Collection index, where this item is found
    /// @return CONTINUE to continue the parsing until the next main item,
    ///         or BREAK to terminate it early
    constexpr virtual control
    parse_report_data_field([[maybe_unused]] const item_type& main_item,
                            [[maybe_unused]] const global_item_store& global_state,
                            [[maybe_unused]] const items_view_type& main_section,
                            [[maybe_unused]] unsigned tlc_number)
    {
        return control::CONTINUE;
    }

    /// @brief  Extracts the complete usage ID using the relevant parsing context.
    /// @param  usage_item: the USAGE / USAGE_MINIMUM / USAGE_MAXIMUM type local item
    /// @param  global_state: the global items state at the current main item
    /// @return The complete usage ID
    constexpr static usage_t get_usage(const item_type& usage_item,
                                       const global_item_store& global_state)
    {
        if (usage_item.data_size() == sizeof(usage_t)) [[unlikely]]
        {
            return usage_t(usage_item.value_unsigned());
        }
        else
        {
            const auto* page = global_state.get_item(global::tag::USAGE_PAGE);
            HID_RP_ASSERT(page != nullptr, ex_usage_page_missing);
            auto page_id = page->value_unsigned();
            HID_RP_ASSERT(page_id > 0, ex_usage_page_zero);
            HID_RP_ASSERT(page_id <= std::numeric_limits<page_id_t>::max(), ex_usage_page_oor);
            return usage_t(page_id, usage_item.value_unsigned());
        }
    }

    /// @brief Verifies the correctness of the delimiters in a section.
    /// Errors are reported using HID_RP_ASSERT.
    /// @param main_section: the span of items between the previous and this main item
    /// @return true if delimiters were found, false otherwise
    constexpr static bool check_delimiters(const items_view_type& main_section)
    {
        bool open = false;
        bool found = false;
        for (const item_type& item : main_section)
        {
            if (item.type() != rdf::item_type::LOCAL)
            {
                continue;
            }
            if (item.has_tag(tag::DELIMITER))
            {
                auto delimiter = item.value_unsigned();
                HID_RP_ASSERT(delimiter <= 1, ex_delimiter_invalid);
                HID_RP_ASSERT(open != delimiter, ex_delimiter_nesting);
                open = delimiter;
                found = true;
                continue;
            }
            if (open and !item.has_tag(tag::USAGE) and !item.has_tag(tag::USAGE_MINIMUM) and
                !item.has_tag(tag::USAGE_MAXIMUM))
            {
                HID_RP_ASSERT(false, ex_delimiter_invalid_content);
            }
        }
        HID_RP_ASSERT(open == false, ex_delimiter_unmatched);
        return found;
    }

    /// @brief Extracts the report data field parameters from the global item store.
    /// @param global_state: the global items state at the current main item
    /// @return The report data field parameters, including ID, size, and count
    constexpr static auto get_report_data_field_params(const global_item_store& global_state)
    {
        report_data_field_params params{};
        const auto* report_id_item = global_state.get_item(global::tag::REPORT_ID);
        if (report_id_item != nullptr)
        {
            params.id = report_id_item->value_unsigned();
            HID_RP_ASSERT(params.id >= report::id::min(), ex_report_id_zero);
            HID_RP_ASSERT(params.id <= report::id::max(), ex_report_id_excess);
        }
        const auto* report_size_item = global_state.get_item(global::tag::REPORT_SIZE);
        HID_RP_ASSERT(report_size_item != nullptr, ex_report_size_missing);
        params.size = report_size_item->value_unsigned();
        HID_RP_ASSERT(params.size > 0, ex_report_size_zero);

        const auto* report_count_item = global_state.get_item(global::tag::REPORT_COUNT);
        HID_RP_ASSERT(report_count_item != nullptr, ex_report_count_missing);
        params.count = report_count_item->value_unsigned();
        HID_RP_ASSERT(params.count > 0, ex_report_count_zero);
        return params;
    }

    /// @brief Extracts the logical limits from the global item store.
    /// @param global_state: the global items state at the current main item
    /// @return The logical limits as a pair of signed integers
    constexpr static auto get_logical_limits_signed(const global_item_store& global_state)
    {
        const auto* min_item = global_state.get_item(global::tag::LOGICAL_MINIMUM);
        HID_RP_ASSERT(min_item != nullptr, ex_logical_min_missing);
        const auto* max_item = global_state.get_item(global::tag::LOGICAL_MAXIMUM);
        HID_RP_ASSERT(max_item != nullptr, ex_logical_max_missing);
        auto lim = limits<std::int32_t>{min_item->value_signed(), max_item->value_signed()};
        HID_RP_ASSERT(lim.min <= lim.max, ex_logical_max_oob);
        return lim;
    }

    /// @brief Extracts the logical limits from the global item store for unsigned values.
    /// @param global_state: the global items state at the current main item
    /// @return The logical limits as a pair of unsigned integers
    [[nodiscard]] constexpr static auto
    get_logical_limits_unsigned(const global_item_store& global_state)
    {
        const auto* min_item = global_state.get_item(global::tag::LOGICAL_MINIMUM);
        HID_RP_ASSERT(min_item != nullptr, ex_logical_min_missing);
        const auto* max_item = global_state.get_item(global::tag::LOGICAL_MAXIMUM);
        HID_RP_ASSERT(max_item != nullptr, ex_logical_max_missing);
        auto lim = limits<std::uint32_t>{min_item->value_unsigned(), max_item->value_unsigned()};
        HID_RP_ASSERT(lim.min <= lim.max, ex_logical_limits_crossed);
        return lim;
    }

    /// @brief Extracts the physical limits from the global item store.
    /// @param global_state: the global items state at the current main item
    /// @return The physical limits as a pair of signed integers, or std::nullopt if not defined
    constexpr static std::optional<limits<std::int32_t>>
    get_physical_limits(const global_item_store& global_state)
    {
        const auto* min_item = global_state.get_item(global::tag::PHYSICAL_MINIMUM);
        const auto* max_item = global_state.get_item(global::tag::PHYSICAL_MAXIMUM);
        if ((min_item != nullptr) and (max_item != nullptr))
        {
            auto lim = limits<std::int32_t>{min_item->value_signed(), max_item->value_signed()};
            HID_RP_ASSERT(lim.min <= lim.max, ex_physical_limits_crossed);
            return lim;
        }
        HID_RP_ASSERT((min_item == nullptr) and (max_item == nullptr), ex_physical_limit_missing);
        return std::nullopt;
    }

    /// @brief Computes the maximum depth of the global stack in the descriptor view.
    /// @param desc_view: the HID report descriptor's view
    /// @return The maximum depth of the global stack
    constexpr static std::size_t global_stack_depth(descriptor_view_type desc_view)
    {
        std::size_t max_depth = 0;
        std::size_t depth = 0;
        for (const item_type& item : desc_view)
        {
            if (item.has_tag(global::tag::PUSH))
            {
                depth++;
                max_depth = std::max(max_depth, depth);
            }
            else if (item.has_tag(global::tag::POP))
            {
                HID_RP_ASSERT(depth > 0, ex_pop_unmatched);
                depth--;
            }
        }
        return max_depth;
    }

    /// @brief This method parses the HID report descriptor,
    /// storing the global items in a fixed-size stack.
    /// @param desc_view: the HID report descriptor's view
    /// @return the iterator to the first item that was not processed
    constexpr TIterator parse_items(descriptor_view_type desc_view)
    {
        HID_RP_ASSERT(desc_view.has_valid_bounds(), ex_invalid_bounds);

        constexpr std::size_t max_global_stack_depth = 5;
        HID_RP_ASSERT(global_stack_depth(desc_view) < max_global_stack_depth,
                      ex_global_stack_overflow);

        global_item_store global_stack[max_global_stack_depth]; // NOLINT
        return fixed_stack_parse(desc_view, std::span(global_stack, max_global_stack_depth));
    }

    /// @brief This method parses the HID report descriptor, using a fixed-size stack for global
    /// items.
    /// @param desc_view: the HID report descriptor's view
    /// @param global_stack: the stack for global items, which must be large enough to hold the
    /// maximum depth
    /// @return the iterator to the first item that was not processed
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    constexpr TIterator fixed_stack_parse(const descriptor_view_type& desc_view,
                                          std::span<global_item_store> global_stack)
    {
        std::size_t global_stack_depth = 0;
        auto last_section_begin = desc_view.end();
        unsigned collection_depth = 0;
        unsigned tlc_number = 0;
        auto last_main_item = desc_view.begin();

        for (auto item_iter = desc_view.begin(); item_iter != desc_view.end(); ++item_iter)
        {
            const item_type& this_item = *item_iter;

            // pick up new section after the last one
            if (last_section_begin == desc_view.end())
            {
                last_section_begin = item_iter;
            }

            switch (this_item.type())
            {
            case rdf::item_type::MAIN:
            {
                items_view_type last_section{last_section_begin, item_iter};
                control ctrl = control::BREAK;
                last_main_item = item_iter;

                // main items is where the magic happens, this is application specific
                // the local item processing is left for this external parser
                switch (this_item.main_tag())
                {
                case main::tag::INPUT:
                case main::tag::OUTPUT:
                case main::tag::FEATURE:
                    HID_RP_ASSERT(collection_depth > 0, ex_collection_missing);
                    ctrl = parse_report_data_field(this_item, global_stack[global_stack_depth],
                                                   last_section, tlc_number);
                    break;

                case main::tag::COLLECTION:
                {
                    auto coll_type = static_cast<main::collection_type>(this_item.value_unsigned());
                    collection_depth++;
                    if (collection_depth == 1)
                    {
                        tlc_number++;
                    }
                    else
                    {
                        HID_RP_ASSERT(coll_type != main::collection_type::APPLICATION,
                                      ex_collection_nested_application);
                    }
                    ctrl = parse_collection_begin(coll_type, global_stack[global_stack_depth],
                                                  last_section, tlc_number);
                    break;
                }

                case main::tag::END_COLLECTION:
                    HID_RP_ASSERT(collection_depth > 0, ex_collection_end_unmatched);
                    collection_depth--;
                    ctrl = parse_collection_end(global_stack[global_stack_depth], last_section,
                                                tlc_number);
                    break;

                default:
                    HID_RP_ASSERT(false, ex_item_unknown);
                    break;
                }
                if (ctrl != control::CONTINUE)
                {
                    return ++item_iter;
                }

                // when this section is processed, mark the section begin marker as invalid
                last_section_begin = desc_view.end();
                break;
            }

            case rdf::item_type::GLOBAL:
                switch (this_item.global_tag())
                {
                case global::tag::PUSH:
                    HID_RP_ASSERT(not this_item.has_data(), ex_push_nonempty);
                    HID_RP_ASSERT((global_stack_depth + 1) < global_stack.size(),
                                  ex_global_stack_overflow);

                    // the current state is kept, backed up on the stack
                    global_stack[global_stack_depth + 1] = global_stack[global_stack_depth];
                    global_stack_depth++;
                    break;

                case global::tag::POP:
                    HID_RP_ASSERT(not this_item.has_data(), ex_pop_nonempty);
                    HID_RP_ASSERT(global_stack_depth > 0, ex_pop_unmatched);

                    // the last backup is restored
                    global_stack_depth--;
                    break;

                default:
                    HID_RP_ASSERT(this_item.template tag<global::tag>() <=
                                      global::tag::REPORT_COUNT,
                                  ex_item_unknown);
                    global_stack[global_stack_depth].add_item(this_item);
                    break;
                }
                break;

            case rdf::item_type::LOCAL:
                // processed by higher level
                break;

            default:
                HID_RP_ASSERT(this_item.is_short() == false, ex_item_unknown);
                HID_RP_ASSERT(this_item.is_short() == true, ex_item_long);
                break;
            }
        }

        HID_RP_ASSERT(global_stack_depth == 0, ex_push_unmatched);
        HID_RP_ASSERT(collection_depth == 0, ex_collection_begin_unmatched);

        return (last_main_item == desc_view.end()) ? desc_view.end() : ++last_main_item;
    }

  protected:
    constexpr parser() = default;
};

/// @brief  This function performs the minimal useful HID report descriptor parsing task:
///         Finding the usage ID of the top level application.
/// @tparam TIterator: iterator type of the descriptor view
/// @param  desc_view: the HID report descriptor's view
/// @return the complete usage ID of the first top-level collection
/// @throws if errors are encountered during parsing, a @ref parser_exception is raised
template <typename TIterator>
constexpr usage_t get_application_usage_id(const descriptor_view_base<TIterator>& desc_view)
{
    /// @brief Internal class that implements the parsing logic for the specific task.
    struct application_usage_id_parser : public parser<TIterator>
    {
        using base = parser<TIterator>;
        using items_view_type = base::items_view_type;
        using descriptor_view_type = base::descriptor_view_type;
        using control = base::control;

        constexpr application_usage_id_parser(const descriptor_view_type& desc_view)
            : parser<TIterator>()
        {
            base::parse_items(desc_view);
        }

        constexpr control parse_collection_begin([[maybe_unused]] main::collection_type collection,
                                                 const global_item_store& global_state,
                                                 const items_view_type& main_section,
                                                 [[maybe_unused]] unsigned tlc_number) override
        {
            collection_depth_++;
            if (collection_depth_ == 1)
            {
                for (auto& it : main_section)
                {
                    if (it.has_tag(tag::USAGE))
                    {
                        // TODO: ensure that this is the only usage in the collection
                        usage_ = base::get_usage(it, global_state);
                        break;
                    }
                }
                HID_RP_ASSERT(usage_ != nullusage, ex_usage_missing);
            }
            return end_collection_ ? control::BREAK : control::CONTINUE;
        }
        constexpr control
        parse_collection_end([[maybe_unused]] const global_item_store& global_state,
                             [[maybe_unused]] const items_view_type& main_section,
                             [[maybe_unused]] unsigned tlc_number) override
        {
            collection_depth_--;
            if (collection_depth_ == 0)
            {
                return control::BREAK;
            }
            return control::CONTINUE;
        }

        usage_t usage_{nullusage};
        unsigned collection_depth_{};
        bool end_collection_{};
    };

    auto usage = application_usage_id_parser(desc_view).usage_;
    // this assert could hit due to either no collection, or the usage value being actually 0
    HID_RP_ASSERT(usage != nullusage, ex_collection_missing);
    return usage;
}

} // namespace hid::rdf
