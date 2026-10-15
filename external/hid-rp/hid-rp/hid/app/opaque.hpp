// SPDX-License-Identifier: MPL-2.0
#pragma once

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
[[nodiscard]] constexpr auto report_descriptor(TUsage use)
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
