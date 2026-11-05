// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "hid/page/lighting_and_illumination.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"

// Windows documentation:
// https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/dynamic-lighting-devices
namespace hid::app::lamparray
{

enum struct kind : std::uint8_t
{
    UNDEFINED = 0x00,
    KEYBOARD = 0x01,        // A LampArray is part of a keyboard/keypad device
    MOUSE = 0x02,           // A LampArray is part of a mouse
    GAME_CONTROLLER = 0x03, // A LampArray is part of a game-controller. (e.g. gamepad, flightstick,
                            // sailing simulation device)
    PERIPHERAL = 0x04,      // A LampArray is part of a general peripheral/accessory (e.g. speakers,
                            // mousepad, microphone, webcam)
    SCENE = 0x05,           // A LampArray illuminates a room/performance-stage/area
                            // (e.g. room light-bulbs, spotlights, washlights, strobelights,
                            // booth-strips, billboard/sign, camera-flash)
    NOTIFICATION = 0x06,    // A LampArray is part of a notification device
    CHASSIS = 0x07,         // A LampArray is part of an internal PC case component (e.g. RAM-stick,
                            // motherboard, fan)
    WEARABLE = 0x08,        // A LampArray is embedded in a wearable accessory (audio-headset,
                            // wristband, watch, shoes)
    FURNITURE = 0x09,       // A LampArray is embedded in a piece of funiture (e.g. chair, desk,
                            // bookcase)
    ART = 0x0A,             // A LampArray is embedded in an artwork (e.g. painting, sculpture)
};

enum struct purposes : std::uint8_t
{
    NONE = 0x00,
    CONTROL = 0x01,      // Control Lamp (e.g. button/key/slider etc…)
    ACCENT = 0x02,       // Accent Lamp that doesn’t interact with the user (e.g. case fan LED,
                         // illuminated side panels on a keyboard)
    BRANDING = 0x04,     // Device branding (e.g. Manufacturer logo)
    STATUS = 0x08,       // Status Lamp (e.g. unread email, CPU temperature)
    ILLUMINATION = 0x10, // Illuminates an object that is outside of the LampArray (e.g. stage
                         // spotlight, car headlights, camera flash)
    PRESENTATION = 0x20, // A Lamp the user directly looks at (e.g. within an artwork or costume)
};

enum struct update_flags : std::uint8_t
{
    NONE = 0x00,
    COMPLETE = 0x01, // Signals that this was the last update in a batch of updates.
                     // Device should now process all precceding messages
                     // as a single update to Lamp state.
};

inline purposes operator|(purposes a, purposes b)
{
    return static_cast<purposes>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}
inline purposes& operator|=(purposes& a, purposes b)
{
    a = a | b;
    return a;
}
inline purposes operator&(purposes a, purposes b)
{
    return static_cast<purposes>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}
inline purposes& operator&=(purposes& a, purposes b)
{
    a = a & b;
    return a;
}

template <std::uint8_t REPORT_ID>
struct lamp_array_attributes_report
    : public hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    packed_integer<2> lamp_count;
    struct
    {
        packed_integer<4> width;
        packed_integer<4> height;
        packed_integer<4> depth;
    } bounding_box;                        // all in micrometers, regardless of unit in descriptor
    packed_integer<4> min_update_interval; // in microseconds, regardless of unit in descriptor
    lamparray::kind kind{};
    std::array<std::byte, 3> _padding{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using lighting = hid::page::lighting_and_illumination;

        // clang-format off
        return rdf::descriptor(
            report_id(REPORT_ID),
            usage(lighting::LAMP_ARRAY_ATTRIBUTES_REPORT),
            collection::logical(
                usage(lighting::LAMP_COUNT),
                logical_limits<1, 4>(0, std::numeric_limits<std::uint16_t>::max()),
                report_size(16),
                report_count(1),
                feature::absolute_constant(),
                usage(lighting::BOUNDING_BOX_WIDTH_UM),
                usage(lighting::BOUNDING_BOX_HEIGHT_UM),
                usage(lighting::BOUNDING_BOX_DEPTH_UM),
                usage(lighting::MINIMAL_UPDATE_INTERVAL_US),
                usage(lighting::LAMP_ARRAY_KIND),
                logical_limits<1, 4>(0, std::numeric_limits<std::int32_t>::max()),
                report_size(32),
                report_count(5),
                feature::absolute_constant()
            )
        );
        // clang-format on
    }
};

template <std::uint8_t REPORT_ID, std::size_t LAMP_ID_SIZE = 2>
struct lamp_attributes_request_report
    : public hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    packed_integer<LAMP_ID_SIZE> lamp_id{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using lighting = hid::page::lighting_and_illumination;
        // clang-format off
        return rdf::descriptor(
            report_id(REPORT_ID),
            usage(lighting::LAMP_ATTRIBUTES_REQUEST_REPORT),
            collection::logical(
                usage(lighting::LAMP_ID),
                logical_limits<1, LAMP_ID_SIZE * 2>(0, std::numeric_limits<sized_unsigned_t<LAMP_ID_SIZE>>::max()),
                report_size(LAMP_ID_SIZE * 8),
                report_count(1),
                feature::absolute_variable()
            )
        );
        // clang-format on
    }
};

template <std::uint8_t REPORT_ID, std::size_t LAMP_ID_SIZE = 2>
struct lamp_attributes_response_report
    : public hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    packed_integer<LAMP_ID_SIZE> lamp_id{};
    struct
    {
        packed_integer<4> x;
        packed_integer<4> y;
        packed_integer<4> z;
    } position;                       // all in micrometers, regardless of unit in descriptor
    packed_integer<4> update_latency; // in microseconds, regardless of unit in descriptor
    lamparray::purposes purposes{};
    std::array<std::byte, 3> _padding{};
    uint8_t red_level_count{};
    uint8_t green_level_count{};
    uint8_t blue_level_count{};
    uint8_t intensity_level_count{};
    uint8_t is_programmable{};
    uint8_t input_binding{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using lighting = hid::page::lighting_and_illumination;

        // clang-format off
        return rdf::descriptor(
            report_id(REPORT_ID),
            usage(lighting::LAMP_ATTRIBUTES_RESPONSE_REPORT),
            collection::logical(
                usage(lighting::LAMP_ID),
                logical_limits<1, LAMP_ID_SIZE * 2>(0, std::numeric_limits<sized_unsigned_t<LAMP_ID_SIZE>>::max()),
                report_size(LAMP_ID_SIZE * 8),
                report_count(1),
                feature::absolute_variable(),
                usage(lighting::POSITION_X_UM),
                usage(lighting::POSITION_Y_UM),
                usage(lighting::POSITION_Z_UM),
                usage(lighting::UPDATE_LATENCY_US),
                usage(lighting::LAMP_PURPOSES),
                logical_limits<1, 4>(0, std::numeric_limits<std::int32_t>::max()),
                report_size(32),
                report_count(5),
                feature::absolute_variable(),
                usage(lighting::RED_LEVEL_COUNT),
                usage(lighting::GREEN_LEVEL_COUNT),
                usage(lighting::BLUE_LEVEL_COUNT),
                usage(lighting::INTENSITY_LEVEL_COUNT),
                usage(lighting::PROGRAMMABLE),
                usage(lighting::INPUT_BINDING),
                logical_limits<1, 2>(0, std::numeric_limits<std::uint8_t>::max()),
                report_size(8),
                report_count(6),
                feature::absolute_variable()
            )
        );
        // clang-format on
    }
};

struct rgbi_tuple
{
    packed_integer<1> red;
    packed_integer<1> green;
    packed_integer<1> blue;
    packed_integer<1> intensity;
};

template <std::uint8_t REPORT_ID, std::size_t MAX_LAMP_COUNT, std::size_t LAMP_ID_SIZE = 2>
struct lamp_multi_update_report : public hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    packed_integer<byte_width(MAX_LAMP_COUNT)> lamp_count{};
    lamparray::update_flags update_flags{};
    std::array<packed_integer<LAMP_ID_SIZE>, MAX_LAMP_COUNT> lamp_ids{};
    std::array<rgbi_tuple, MAX_LAMP_COUNT> values{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using lighting = hid::page::lighting_and_illumination;

        // clang-format off
        constexpr auto rgbi_usages =
            rdf::descriptor(
                usage(lighting::RED_UPDATE_CHANNEL),
                usage(lighting::GREEN_UPDATE_CHANNEL),
                usage(lighting::BLUE_UPDATE_CHANNEL),
                usage(lighting::INTENSITY_UPDATE_CHANNEL));

        return rdf::descriptor(
            report_id(REPORT_ID),
            usage(lighting::LAMP_MULTI_UPDATE_REPORT),
            collection::logical(
                usage(lighting::LAMP_COUNT),
                logical_limits<1, byte_width(int(MAX_LAMP_COUNT))>(0, MAX_LAMP_COUNT),
                report_size(byte_width(MAX_LAMP_COUNT) * 8),
                report_count(1),
                feature::absolute_variable(),
                usage(lighting::LAMP_UPDATE_FLAGS),
                logical_limits<1, 1>(0, 1),
                report_size(8),
                feature::absolute_variable(),
                usage(lighting::LAMP_ID),
                logical_limits<1, LAMP_ID_SIZE * 2>(0, std::numeric_limits<sized_unsigned_t<LAMP_ID_SIZE>>::max()),
                report_size(LAMP_ID_SIZE * 8),
                report_count(MAX_LAMP_COUNT),
                feature::absolute_variable(),
                rgbi_usages.repeat<MAX_LAMP_COUNT>(),
                logical_limits<1, 2>(0, std::numeric_limits<std::uint8_t>::max()),
                report_size(8),
                report_count(MAX_LAMP_COUNT * 4),
                feature::absolute_variable()
            )
        );
        // clang-format on
    }
};

template <std::uint8_t REPORT_ID, std::size_t LAMP_ID_SIZE = 2>
struct lamp_range_update_report : public hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    lamparray::update_flags update_flags{};
    packed_integer<LAMP_ID_SIZE> lamp_id_start{};
    packed_integer<LAMP_ID_SIZE> lamp_id_end{};
    rgbi_tuple value{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using lighting = hid::page::lighting_and_illumination;
        // clang-format off
        return rdf::descriptor(
            report_id(REPORT_ID),
            usage(lighting::LAMP_RANGE_UPDATE_REPORT),
            collection::logical(
                usage(lighting::LAMP_UPDATE_FLAGS),
                logical_limits<1, 1>(0, 1),
                report_size(8),
                report_count(1),
                feature::absolute_variable(),
                usage(lighting::LAMP_ID_START),
                usage(lighting::LAMP_ID_END),
                logical_limits<1, LAMP_ID_SIZE * 2>(0, std::numeric_limits<sized_unsigned_t<LAMP_ID_SIZE>>::max()),
                report_size(LAMP_ID_SIZE * 8),
                report_count(2),
                feature::absolute_variable(),
                usage(lighting::RED_UPDATE_CHANNEL),
                usage(lighting::GREEN_UPDATE_CHANNEL),
                usage(lighting::BLUE_UPDATE_CHANNEL),
                usage(lighting::INTENSITY_UPDATE_CHANNEL),
                logical_limits<1, 2>(0, 255),
                report_size(8),
                report_count(4),
                feature::absolute_variable()
            )
        );
        // clang-format on
    }
};

template <std::uint8_t REPORT_ID>
struct control_report : public hid::report::base<hid::report::type::FEATURE, REPORT_ID>
{
    bool autonomous_mode{};

    [[nodiscard]] static constexpr auto descriptor()
    {
        using namespace hid::page;
        using namespace hid::rdf;
        using lighting = hid::page::lighting_and_illumination;

        // clang-format off
        return rdf::descriptor(
            report_id(REPORT_ID),
            usage(lighting::LAMP_ARRAY_CONTROL_REPORT),
            collection::logical(
                logical_limits<1, 1>(0, 1),
                usage(lighting::AUTONOMOUS_MODE),
                report_size(8),
                report_count(1),
                feature::absolute_variable()
            )
        );
        // clang-format on
    }
};

} // namespace hid::app::lamparray
