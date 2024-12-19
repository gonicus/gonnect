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
#ifndef __HID_APP_KEYBOARD_HPP_
#define __HID_APP_KEYBOARD_HPP_

#include "hid/page/generic_desktop.hpp"
#include "hid/page/keyboard_keypad.hpp"
#include "hid/page/leds.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"
#include "hid/report_array.hpp"
#include "hid/report_bitset.hpp"

namespace hid::app::keyboard
{
template <uint8_t REPORT_ID = 0, std::size_t ROLLOVER_LIMIT = 6>
static constexpr auto keys_input_report_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        conditional_report_id<REPORT_ID>(),
        // modifier byte
        report_size(1),
        report_count(8),
        logical_limits<1, 1>(0, 1),
        usage_page<keyboard_keypad>(),
        usage_limits(keyboard_keypad::KEYBOARD_LEFT_CONTROL, keyboard_keypad::KEYBOARD_RIGHT_GUI),
        input::absolute_variable(),
        // reserved byte
        input::padding(8),
        // key codes
        report_size(8),
        report_count(ROLLOVER_LIMIT),
        logical_limits<1, 1>(0, keyboard_keypad::KEYPAD_HEXADECIMAL),
        usage_limits(nullusage, keyboard_keypad::KEYPAD_HEXADECIMAL),
        input::array()
    );
    // clang-format on
}

template <std::uint8_t REPORT_ID = 0, std::size_t ROLLOVER_LIMIT = 6>
struct keys_input_report : public hid::report::base<hid::report::type::INPUT, REPORT_ID>
{
    hid::report_bitset<page::keyboard_keypad, page::keyboard_keypad::KEYBOARD_LEFT_CONTROL,
                       page::keyboard_keypad::KEYBOARD_RIGHT_GUI>
        modifiers;
    std::uint8_t reserved{};
    hid::report_array<page::keyboard_keypad, ROLLOVER_LIMIT> scancodes;

    constexpr keys_input_report() = default;

    constexpr bool set_key_state(page::keyboard_keypad key, bool pressed)
    {
        using namespace hid::page;

        if (modifiers.set(key, pressed))
        {
            return true;
        }
        if (scancodes.set(key, pressed))
        {
            return true;
        }
        if (pressed)
        {
            // scancodes buffer full, need to raise rollover error
            scancodes.fill(keyboard_keypad::ERROR_ROLLOVER);
        }
        else
        {
            // clear rollover error when a key is released
            scancodes.reset();
        }
        return false;
    }
};
static_assert(sizeof(keys_input_report<0>) == 8);
static_assert(sizeof(keys_input_report<1>) == 9);

template <uint8_t REPORT_ID>
static constexpr auto leds_output_report_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        conditional_report_id<REPORT_ID>(),
        report_size(1),
        report_count(5),
        logical_limits<1, 1>(0, 1),
        usage_page<leds>(),
        usage_limits(leds::NUM_LOCK, leds::KANA),
        output::absolute_variable(),
        output::padding(3)
    );
    // clang-format on
}

template <uint8_t REPORT_ID = 0>
struct output_report : public hid::report::base<hid::report::type::OUTPUT, REPORT_ID>
{
    hid::report_bitset<page::leds, page::leds::NUM_LOCK, page::leds::KANA> leds;
};

template <uint8_t REPORT_ID = 0>
static constexpr auto app_report_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::KEYBOARD),
        collection::application(
            // input keys report
            keys_input_report_descriptor<REPORT_ID>(),

            // LED report
            leds_output_report_descriptor<REPORT_ID>()
        )
    );
    // clang-format on
}

} // namespace hid::app::keyboard

#endif // __HID_APP_KEYBOARD_HPP_
