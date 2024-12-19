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
#ifndef __HID_APP_OPAQUE_HPP_
#define __HID_APP_OPAQUE_HPP_

#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"

namespace hid::app::opaque
{
template <unsigned BYTE_SIZE, hid::report::type TYPE, hid::report::id::type REPORT_ID = 0>
struct report : public hid::report::base<TYPE, REPORT_ID>
{
    std::array<uint8_t, BYTE_SIZE> payload{};
};

template <typename TReport, typename TUsage>
static constexpr auto report_descriptor(TUsage use)
{
    using namespace hid::rdf;

    // clang-format off
    return descriptor(
        conditional_report_id<TReport::ID>(),
        report_size(8),
        report_count(sizeof(TReport) - ((TReport::ID > 0) ? 1 : 0)),
        logical_limits<1, 1>(0, 0xff),
        usage_extended(use),
        main::data_field<TReport::type()>::buffered_variable()
    );
    // clang-format on
}

} // namespace hid::app::opaque

#endif // __HID_APP_OPAQUE_HPP_
