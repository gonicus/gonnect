// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "hid/page/button.hpp"
#include "hid/page/consumer.hpp"
#include "hid/page/generic_desktop.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"
#include "hid/report_bitset.hpp"

namespace hid::app::mouse
{
/// @brief  A mouse input report, conforming to the HID boot protocol,
///         with an option to increase the number of buttons in the report.
/// @tparam REPORT_ID: the report ID of the input report, 0 for boot protocol
/// @tparam BUTTONS_COUNT: the number of buttons in the report, 3 for boot protocol
template <uint8_t REPORT_ID = 0, std::size_t BUTTONS_COUNT = 3>
struct pointer_report : public hid::report::base<hid::report::type::INPUT, REPORT_ID>
{
    using buttons_t = hid::report_bitset_range<page::button(1), page::button(BUTTONS_COUNT)>;

    buttons_t buttons;
    std::int8_t x{};
    std::int8_t y{};

    constexpr void reset_movement()
    {
        x = 0;
        y = 0;
    }

    static constexpr boot::mode boot_mode()
    {
        if constexpr ((REPORT_ID == 0) and (BUTTONS_COUNT == 3))
        {
            return boot::mode::MOUSE;
        }
        else
        {
            return boot::mode::NONE;
        }
    }

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;

        // clang-format off
        return rdf::descriptor(
            conditional_report_id<REPORT_ID>(),
            usage_page<button>(),
            buttons_t::template descriptor<hid::report::type::INPUT>(),
            usage_page<generic_desktop>(),
            usage(generic_desktop::X),
            usage(generic_desktop::Y),
            logical_limits<1, 1>(-127, 127),
            report_count(2),
            report_size(8),
            input::relative_variable()
        );
        // clang-format on
    }
};

// Boot protocol has predefined report layout
struct boot_report final : pointer_report<0, 3>
{
    using pointer_report::pointer_report;
};

template <uint8_t REPORT_ID = 0, std::size_t BUTTONS_COUNT = 3>
[[nodiscard]] constexpr auto app_report_descriptor()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        usage_page<generic_desktop>(),
        usage(generic_desktop::MOUSE),
        collection::application(
            usage(generic_desktop::POINTER),
            collection::physical(
                pointer_report<REPORT_ID, BUTTONS_COUNT>::descriptor()
            )
        )
    );
    // clang-format on
}

/// @defgroup mouse_high_res_scroll Mouse High Resolution Scrolling
/// @brief    Utilities for using high-resolution scrolling in mouse applications
/// @details  High-resolution scrolling allows a host OS to activate this feature on a mouse.
///           The specification is written by Microsoft:
///           https://learn.microsoft.com/en-us/previous-versions/windows/hardware/design/dn613912(v=vs.85)
///           Windows (since Vista) and Linux (since 5.0) have support, not without issues though
/// @warning  This feature has the following problems on Linux:
///           - The mouse must use report IDs, otherwise the kernel fails to enable it,
///             see: https://bugzilla.kernel.org/show_bug.cgi?id=220144
///           - (Until 6.19) The resolution multiplier isn't set on the device after a
///             suspend/resume cycle, see: https://bugzilla.kernel.org/show_bug.cgi?id=203421
/// @warning  This feature has the following problems on Windows:
///           - The resolution multiplier isn't set on the device after a Fast startup boot,
///             or after a suspend/resume cycle.
///             A possible workaround is using MSOS descriptors in the USB device, and overwrite the
///             multiplier to high resolution when the host fetches the Windows-specific
///             descriptors.
/// @{

[[nodiscard]] constexpr uint8_t resolution_multiplier_bit_size()
{
    // https://github.com/qmk/qmk_firmware/issues/17585#issuecomment-1238023671
    return 2;
}

/// @brief  Defines the descriptor items for a HID control to associate a resolution multiplier
///         value with it in a HID feature report.
/// @note   This descriptor block is assuming the current usage page is generic_desktop
/// @note   This item only takes two bits in the feature report, no byte padding
/// @tparam MULTIPLIER_MAX: the maximum value of the resolution multiplier (valid range is 1-120)
/// @return the HID report descriptor items
template <uint8_t MULTIPLIER_MAX>
[[nodiscard]] constexpr auto resolution_multiplier()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        push_globals(),
        usage(generic_desktop::RESOLUTION_MULTIPLIER),
        logical_limits<1, 1>(0, 1),
        physical_limits<1, 1>(1, MULTIPLIER_MAX),
        report_count(1),
        report_size(resolution_multiplier_bit_size()),
        feature::absolute_variable(),
        pop_globals()
    );
    // clang-format on
}

/// @brief  Defines a HID feature report to store the resolution multiplier values for vertical and
///         horizontal scrolling.
/// @tparam MULTIPLIER_MAX: the maximum value of the resolution multiplier (valid range is 1-120)
/// @tparam REPORT_ID: the report ID of the feature report, don't use 0 for Linux compatibility:
///         https://bugzilla.kernel.org/show_bug.cgi?id=220144
template <uint8_t MULTIPLIER_MAX, uint8_t REPORT_ID>
struct resolution_multiplier_report
    : public hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    std::uint8_t resolutions{};

    constexpr void reset() { resolutions = 0; }
    [[nodiscard]] bool high_resolution() const { return resolutions != 0; }

    [[nodiscard]] constexpr uint8_t vertical_scroll_multiplier() const
    {
        return ((resolutions & 0x01) != 0) ? MULTIPLIER_MAX : 1;
    }
    [[nodiscard]] constexpr uint8_t horizontal_scroll_multiplier() const
    {
        return ((resolutions & 0x04) != 0) ? MULTIPLIER_MAX : 1;
    }
};

/// @brief  Creates the descriptor block for high resolution scrolling, that is to be inserted
///         into a mouse pointer collection.
/// @note   This descriptor block is assuming the current usage page is generic_desktop
/// @tparam MAX_SCROLL: the maximum scroll value for Wheel and AC Pan usages
/// @tparam MULTIPLIER_MAX: the maximum value of the resolution multiplier (valid range is 1-120)
/// @return the descriptor block
template <int16_t MAX_SCROLL, uint8_t MULTIPLIER_MAX>
[[nodiscard]] constexpr auto high_resolution_scrolling()
{
    using namespace hid::page;
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        collection::logical(
            usage(generic_desktop::WHEEL),
            logical_limits<byte_width(MAX_SCROLL)>(-MAX_SCROLL, MAX_SCROLL),
            report_count(1),
            report_size(byte_width(MAX_SCROLL) * 8),
            input::relative_variable(),
            resolution_multiplier<MULTIPLIER_MAX>()
        ),
        collection::logical(
            usage_extended(consumer::AC_PAN),
            // skip repeating the same global items as the Wheel usage
            input::relative_variable(),
            resolution_multiplier<MULTIPLIER_MAX>()
        ),
        feature::byte_padding<resolution_multiplier_bit_size() * 2>()
    );
    // clang-format on
}

/// @}

/// @brief   A bitset to store the motion wakeup state of a mouse, as a report field.
/// @details The motion wakeup state is a single bit in a feature report, that the host can set
///          to allow the device to send USB remote wake-up signal on motion
///          (vs only on button press).
using motion_wakeup_bit = report_bitset<page::generic_desktop::MOTION_WAKEUP>;

} // namespace hid::app::mouse
