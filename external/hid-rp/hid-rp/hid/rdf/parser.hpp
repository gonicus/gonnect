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
#ifndef __HID_RDF_PARSER_HPP_
#define __HID_RDF_PARSER_HPP_

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
        HID_RDF_ASSERT(is_tag_stored(tag), ex_item_unknown);
        item_by_tag(tag) = new_item;
    }

    void add_item(const item& new_item)
    {
        auto tag = new_item.global_tag();
        HID_RDF_ASSERT(is_tag_stored(tag), ex_item_unknown);
        item_by_tag(tag) = new_item;
    }

    constexpr bool has_item(global::tag tag) const
    {
        return is_tag_stored(tag) and (item_by_tag(tag).type() == item_type::GLOBAL);
    }

    constexpr const short_item_buffer* get_item(global::tag tag) const
    {
        if (!has_item(tag))
        {
            return nullptr;
        }
        else
        {
            return &item_by_tag(tag);
        }
    }

  private:
    // due to the numbering of global tags, we can efficiently index them in an array
    static_assert(static_cast<byte_type>(global::tag::USAGE_PAGE) == 0);

    // zero-initialized items will have non-global type (used in has_item())
    static_assert(static_cast<byte_type>(item_type::GLOBAL) != 0);

    constexpr static bool is_tag_stored(global::tag tag)
    {
        return (tag <= global::tag::REPORT_COUNT);
    }

    constexpr const short_item_buffer& item_by_tag(global::tag tag) const
    {
        return items_[static_cast<byte_type>(tag)];
    }
    constexpr short_item_buffer& item_by_tag(global::tag tag)
    {
        return items_[static_cast<byte_type>(tag)];
    }

    constexpr static byte_type ITEMS_COUNT = static_cast<byte_type>(global::tag::REPORT_COUNT) + 1;

    std::array<short_item_buffer, ITEMS_COUNT> items_;
};

/// @brief Base class for parsing HID report descriptors. Derived classes can define
///        the output data of the parsing as member variables, assigning them during parsing.
template <typename TIterator, typename TItem = TIterator::value_type>
class parser
{
  public:
    using item_type = TItem;
    using items_view_type = items_view_base<TIterator>;
    using descriptor_view_type = descriptor_view_base<TIterator>;

    /// @brief The control return value allows early termination of parsing,
    ///        in case all relevant information has been gathered early.
    enum class control
    {
        CONTINUE = 0, ///< continue parsing
        BREAK = 1,    ///< stop further parsing
    };

    /// @brief  Extracts the complete usage ID using the relevant parsing context.
    /// @param  usage_item: the USAGE / USAGE_MINIMUM / USAGE_MAXIMUM type local item
    /// @param  global_state: the global items state at the current main item
    /// @return The complete usage ID
    constexpr static usage_t get_usage(const item_type& usage_item,
                                       const global_item_store& global_state)
    {
        if (usage_item.data_size() == sizeof(usage_t))
        {
            return usage_t(usage_item.value_unsigned());
        }
        else
        {
            const auto* page = global_state.get_item(global::tag::USAGE_PAGE);
            HID_RDF_ASSERT(page != nullptr, ex_usage_page_missing);
            return usage_t(page->value_unsigned(), usage_item.value_unsigned());
        }
    }

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

    constexpr static std::size_t global_stack_depth(descriptor_view_type desc_view)
    {
        std::size_t depth = 0;
        for (const item_type& item : desc_view)
        {
            if (item.has_tag(global::tag::PUSH))
            {
                depth++;
            }
            else if (item.has_tag(global::tag::POP))
            {
                HID_RDF_ASSERT(depth > 0, ex_pop_unmatched);
                depth--;
            }
        }
        return depth;
    }

    constexpr control parse_items(descriptor_view_type desc_view)
    {
        const auto global_stack_size = 1 + global_stack_depth(desc_view);

        // using this global stack logic is hardly justifiable at all
        HID_RDF_ASSERT(global_stack_size <= 4, ex_global_stack_overflow);

#ifdef _MSC_BUILD
        global_item_store global_stack[4];
        return fixed_stack_parse(desc_view, std::span(global_stack));
#else
        global_item_store global_stack[global_stack_size];
        return fixed_stack_parse(desc_view, std::span(global_stack, global_stack_size));
#endif
    }

    template <std::size_t _Extent>
    constexpr control fixed_stack_parse(const descriptor_view_type& desc_view,
                                        std::span<global_item_store, _Extent> global_stack)
    {
        std::size_t global_stack_depth = 0;
        auto last_section_begin = desc_view.end();
        int collection_balance = 0;
        unsigned tlc_number = 0;

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

                // main items is where the magic happens, this is application specific
                // the local item processing is left for this external parser
                switch (this_item.main_tag())
                {
                case main::tag::INPUT:
                case main::tag::OUTPUT:
                case main::tag::FEATURE:
                    HID_RDF_ASSERT(collection_balance > 0, ex_collection_missing);
                    ctrl = parse_report_data_field(this_item, global_stack[global_stack_depth],
                                                   last_section, tlc_number);
                    break;

                case main::tag::COLLECTION:
                {
                    auto coll_type = static_cast<main::collection_type>(this_item.value_unsigned());
                    collection_balance++;
                    if (collection_balance == 1)
                    {
                        tlc_number++;
                    }
                    else
                    {
                        HID_RDF_ASSERT(coll_type != main::collection_type::APPLICATION,
                                       ex_collection_nested_application);
                    }
                    ctrl = parse_collection_begin(coll_type, global_stack[global_stack_depth],
                                                  last_section, tlc_number);
                    break;
                }

                case main::tag::END_COLLECTION:
                    collection_balance--;
                    HID_RDF_ASSERT(not(collection_balance < 0), ex_collection_end_unmatched);
                    ctrl = parse_collection_end(global_stack[global_stack_depth], last_section,
                                                tlc_number);
                    break;

                default:
                    HID_RDF_ASSERT(false, ex_item_unknown);
                    break;
                }
                if (ctrl != control::CONTINUE)
                {
                    return ctrl;
                }

                // when this section is processed, mark the section begin marker as invalid
                last_section_begin = desc_view.end();
                break;
            }

            case rdf::item_type::GLOBAL:
            {
                switch (this_item.global_tag())
                {
                case global::tag::PUSH:
                    HID_RDF_ASSERT(not this_item.has_data(), ex_push_nonempty);
                    HID_RDF_ASSERT((global_stack_depth + 1) < global_stack.size(),
                                   ex_global_stack_overflow);

                    // the current state is kept, backed up on the stack
                    global_stack[global_stack_depth + 1] = global_stack[global_stack_depth];
                    global_stack_depth++;
                    break;

                case global::tag::POP:
                    HID_RDF_ASSERT(global_stack_depth > 0, ex_pop_unmatched);
                    HID_RDF_ASSERT(not this_item.has_data(), ex_pop_nonempty);

                    // the last backup is restored
                    global_stack_depth--;
                    break;

                default:
                    // HID_RDF_ASSERT(this_item.tag<global::tag>() <= global::tag::REPORT_COUNT,
                    // ex_item_unknown);
                    global_stack[global_stack_depth].add_item(this_item);
                    break;
                }
                break;
            }

            case rdf::item_type::LOCAL:
            {
                // processed by higher level
                break;
            }

            default:
            {
                HID_RDF_ASSERT(this_item.is_short() == false, ex_item_unknown);
                HID_RDF_ASSERT(this_item.is_short() == true, ex_item_long);
                break;
            }
            }
        }

        HID_RDF_ASSERT(global_stack_depth == 0, ex_push_unmatched);
        HID_RDF_ASSERT(not(collection_balance > 0), ex_collection_begin_unmatched);

        return control::CONTINUE;
    }

  protected:
    constexpr parser() {}
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

        constexpr control parse_collection_begin(main::collection_type collection,
                                                 const global_item_store& global_state,
                                                 const items_view_type& main_section,
                                                 unsigned tlc_count) override
        {
            for (auto& it : main_section)
            {
                if (it.has_tag(local::tag::USAGE))
                {
                    usage_ = base::get_usage(it, global_state);
                    return control::BREAK;
                }
            }
            HID_RDF_ASSERT(false, ex_usage_missing);
            return control::BREAK;
        }

        usage_t usage_{0};
    };

    auto usage = application_usage_id_parser(desc_view).usage_;
    // this assert could hit due to either no collection, or the usage value being actually 0
    HID_RDF_ASSERT(usage != nullusage, ex_collection_missing);
    return usage;
}
} // namespace hid::rdf

#endif // __HID_RDF_PARSER_HPP_
