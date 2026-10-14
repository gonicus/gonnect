// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "hid/page/generic_desktop.hpp"
#include "hid/page/leds.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"
#include "hid/report_bitset.hpp"

namespace hid::app::call_state_control
{

template <std::uint8_t REPORT_ID = 0>
struct input_report : hid::report::base<hid::report::type::INPUT, REPORT_ID>
{
    using controls_t = hid::report_bitset<page::generic_desktop::CALL_MUTE_TOGGLE>;
    controls_t controls;

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        // clang-format off
        return rdf::descriptor(
            conditional_report_id<REPORT_ID>(),
            controls_t::template descriptor<report::type::INPUT>()
        );
        // clang-format on
    }
};

template <std::uint8_t REPORT_ID = 0>
struct output_report : hid::report::base<hid::report::type::OUTPUT, REPORT_ID>
{
    using indicators_t = hid::report_bitset<page::generic_desktop::CALL_ACTIVE_LED,
                                            page::generic_desktop::CALL_MUTE_LED>;
    indicators_t indicators;

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        // clang-format off
        return rdf::descriptor(
            conditional_report_id<REPORT_ID>(),
            indicators_t::template descriptor<report::type::OUTPUT>()
        );
        // clang-format on
    }
};

template <std::uint8_t REPORT_ID = 0>
[[nodiscard]] constexpr auto app_report_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::CALL_STATE_MANAGEMENT_CONTROL),
        collection::application(
            conditional_report_id<REPORT_ID>(),
            input_report<>::descriptor(),
            output_report<>::descriptor()
        )
    );
    // clang-format on
}

} // namespace hid::app::call_state_control
