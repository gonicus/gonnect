// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "hid/page/consumer.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/page/keyboard_keypad.hpp"
#include "hid/page/leds.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"
#include "hid/report_array.hpp"
#include "hid/report_bitset.hpp"

namespace hid::app::keyboard
{
template <std::uint8_t REPORT_ID = 0, std::size_t ROLLOVER_LIMIT = 6>
struct keys_input_report : hid::report::base<hid::report::type::INPUT, REPORT_ID>
{
    using modifiers_t = hid::report_bitset_range<page::keyboard_keypad::KEYBOARD_LEFT_CONTROL,
                                                 page::keyboard_keypad::KEYBOARD_RIGHT_GUI>;
    using scancodes_t = hid::report_array<page::keyboard_keypad, ROLLOVER_LIMIT>;

    modifiers_t modifiers;
    std::uint8_t reserved{};
    scancodes_t scancodes;

    constexpr keys_input_report() = default;

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return rdf::descriptor(
            conditional_report_id<REPORT_ID>(),
            usage_page<keyboard_keypad>(),
            modifiers_t::template descriptor<report::type::INPUT>(),
            input::padding(8),
            scancodes_t::template descriptor<report::type::INPUT>(keyboard_keypad::KEYPAD_HEXADECIMAL)
        );
        // clang-format on
    }

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

    [[nodiscard]] static constexpr boot::mode boot_mode()
    {
        if constexpr ((REPORT_ID == 0) and (ROLLOVER_LIMIT == 6))
        {
            return boot::mode::KEYBOARD;
        }
        else
        {
            return boot::mode::NONE;
        }
    }
};

// Boot protocol has predefined report layout
struct boot_input_report final : keys_input_report<0, 6>
{
    using keys_input_report::keys_input_report;
};

template <uint8_t REPORT_ID = 0>
struct output_report : hid::report::base<hid::report::type::OUTPUT, REPORT_ID>
{
    using leds_t = hid::report_bitset_range<page::leds::NUM_LOCK, page::leds::KANA>;
    leds_t leds;

    [[nodiscard]] static constexpr boot::mode boot_mode()
        requires(REPORT_ID == 0)
    {
        return boot::mode::KEYBOARD;
    }

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return rdf::descriptor(
            conditional_report_id<REPORT_ID>(),
            usage_page<page::leds>(),
            leds_t::template descriptor<report::type::OUTPUT>()
        );
        // clang-format on
    }
};

// Boot protocol has predefined report layout
struct boot_output_report final : output_report<0>
{
    using output_report::output_report;
};

template <std::uint8_t REPORT_ID = 0, std::size_t... sz>
[[nodiscard]] constexpr auto app_report_descriptor(rdf::array<sz>... items)
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::KEYBOARD),
        collection::application(
            conditional_report_id<REPORT_ID>(),
            keys_input_report<>::descriptor(),
            output_report<>::descriptor(),
            items...
        )
    );
    // clang-format on
}

enum struct form_factor : std::uint8_t
{
    UNKNOWN = 0,
    FULL_SIZE = 1,
    COMPACT = 2,
};

enum struct key_type : std::uint8_t
{
    UNKNOWN = 0,
    FULL_TRAVEL = 1,
    LOW_TRAVEL = 2,
    ZERO_TRAVEL = 3,
};

enum struct layout : std::uint8_t
{
    UNKNOWN = 0,
    _101 = 1,
    _103 = 2,
    _102 = 3,
    _104 = 4,
    _106 = 5,
    VENDOR_SPECIFIC = 6,
};

enum struct input_assist_support : std::uint8_t
{
    NONE = 0,
    PREVIOUS_SUGGESTION = 1,
    NEXT_SUGGESTION = 2,
    PREVIOUS_SUGGESTION_GROUP = 4,
    NEXT_SUGGESTION_GROUP = 8,
    ACCEPT_SUGGESTION = 16,
    CANCEL_SUGGESTION = 32,
};

// https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/keyboard-backlight-implementation-guide
namespace backlight
{
template <std::uint8_t REPORT_ID = 0>
struct input_report : hid::report::base<hid::report::type::INPUT, REPORT_ID>
{
    using controls_t = hid::report_bitset<
        page::consumer::KEYBOARD_BRIGHTNESS_INCREMENT,
        page::consumer::KEYBOARD_BRIGHTNESS_DECREMENT, page::consumer::KEYBOARD_BACKLIGHT_TOGGLE,
        page::consumer::KEYBOARD_BACKLIGHT_AUTO, page::consumer::KEYBOARD_BACKLIGHT_SET_MINIMUM,
        page::consumer::KEYBOARD_BACKLIGHT_SET_MAXIMUM, page::consumer::KEYBOARD_BRIGHTNESS_NEXT,
        page::consumer::KEYBOARD_BRIGHTNESS_PREVIOUS>;

    controls_t controls;

    constexpr input_report() = default;

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
    uint8_t brightness_level{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        // clang-format off
        return rdf::descriptor(
            conditional_report_id<REPORT_ID>(),
            unit::nit(),
            logical_limits<1, 2>(0, std::numeric_limits<std::uint8_t>::max()),
            report_size(8),
            report_count(1),
            usage(consumer::KEYBOARD_BACKLIGHT_SET_LEVEL),
            output::absolute_variable()
        );
        // clang-format on
    }
};

template <std::size_t SIZE, std::uint8_t REPORT_ID = 0>
struct presets_report : hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    std::array<uint8_t, SIZE> presets{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        // clang-format off
        return rdf::descriptor(
            // TODO: these are repeating items if placed after the output_report
            conditional_report_id<REPORT_ID>(),
            unit::nit(),
            logical_limits<1, 2>(0, std::numeric_limits<std::uint8_t>::max()),
            report_size(8),
            report_count(SIZE),
            usage(consumer::KEYBOARD_BACKLIGHT_LEVEL_SUGGESTION),
            feature::absolute_constant()
        );
        // clang-format on
    }
};

template <std::uint8_t REPORT_ID = 0, std::size_t... sz>
[[nodiscard]] constexpr auto report_descriptor(rdf::array<sz>... items)
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<consumer>(),
        usage(consumer::KEYBOARD_BACKLIGHT),
        collection::application(
            conditional_report_id<REPORT_ID>(),
            input_report<>::descriptor(),
            output_report<>::descriptor(),
            items...
        )
    );
    // clang-format on
}

} // namespace backlight

} // namespace hid::app::keyboard
