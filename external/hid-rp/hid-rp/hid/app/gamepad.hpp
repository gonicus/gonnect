// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "hid/page/generic_desktop.hpp"
#include "hid/page/leds.hpp"
#include "hid/page/simulation.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"
#include "sized_unsigned.hpp"

// https://learn.microsoft.com/en-us/windows/win32/xinput/directinput-and-xusb-devices
// https://source.android.com/static/docs/compatibility/android-cdd.pdf chapter 7.2.6
namespace hid::app::gamepad
{

template <std::int32_t JOYSTICK_MIN, std::int32_t JOYSTICK_MAX>
[[nodiscard]] constexpr auto left_joystick_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::POINTER),
        collection::physical(
            usage(generic_desktop::X),
            usage(generic_desktop::Y),
            logical_limits<byte_width(JOYSTICK_MIN), byte_width(JOYSTICK_MAX)>(JOYSTICK_MIN, JOYSTICK_MAX),
            report_count(2),
            report_size(byte_width(JOYSTICK_MAX) * 8),
            input::absolute_variable()
        )
    );
    // clang-format on
}

template <std::int32_t JOYSTICK_MIN, std::int32_t JOYSTICK_MAX>
[[nodiscard]] constexpr auto right_joystick_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::POINTER),
        collection::physical(
            usage(generic_desktop::Z),
            usage(generic_desktop::RZ),
            logical_limits<byte_width(JOYSTICK_MIN), byte_width(JOYSTICK_MAX)>(JOYSTICK_MIN, JOYSTICK_MAX),
            report_count(2),
            report_size(byte_width(JOYSTICK_MAX) * 8),
            input::absolute_variable()
        )
    );
    // clang-format on
}

template <std::int32_t TRIGGER_MIN, std::int32_t TRIGGER_MAX>
[[nodiscard]] constexpr auto triggers_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<simulation>(),
        usage(simulation::ACCELERATOR),
        usage(simulation::BRAKE),
        logical_limits<byte_width(TRIGGER_MIN), byte_width(TRIGGER_MAX)>(TRIGGER_MIN, TRIGGER_MAX),
        report_count(2),
        report_size(byte_width(TRIGGER_MAX) * 8),
        input::absolute_variable()
    );
    // clang-format on
}

[[nodiscard]] constexpr auto dpad_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::HAT_SWITCH),
        logical_limits<1, 1>(1, 8),
        physical_limits<1, 2>(0, 315),
        unit::degree(),
        report_size(4),
        report_count(1),
        input::absolute_variable(input::flags::NULL_STATE),
        physical_max<1>(0),
        unit::none(),
        input::byte_padding<4>()
    );
    // clang-format on
}

} // namespace hid::app::gamepad
