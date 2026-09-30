// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <array>
#include "hid/page/generic_desktop.hpp"
#include "hid/rdf/descriptor.hpp"
#include "hid/report.hpp"

namespace hid
{
/// @brief  The counted_buffer serves as a report field to transfer variable length data
///         between the host and device, with a maximum size limit.
/// @tparam MAX_SIZE: the maximum number of bytes that can be transferred in a single report
/// @tparam T: the type of the bytes, defaults to std::uint8_t
template <std::size_t MAX_SIZE, typename T = std::uint8_t>
class counted_buffer
{
  public:
    [[nodiscard]] static constexpr std::size_t max_size() { return MAX_SIZE; }

    template <report::type TYPE, UsageType TUsage>
    [[nodiscard]] static constexpr auto descriptor(TUsage usage)
    {
        using namespace hid::page;
        using namespace hid::rdf;
        return rdf::descriptor(
            // clang-format off
            usage(generic_desktop::COUNTED_BUFFER),
            collection::logical(
                report_size(bitfilled::byte_width(max_size()) * 8),
                report_count(1),
                logical_limits<1, bitfilled::byte_width(int(max_size()))>(0, max_size()),
                usage(generic_desktop::BYTE_COUNT),
                main::data_field<TYPE>::absolute_variable(),

                report_size(sizeof(T) * 8),
                report_count(max_size()),
                logical_limits<1, sizeof(T)>(0, std::numeric_limits<T>::max()),
                usage(usage),
                main::data_field<TYPE>::buffered_variable()
            )
            // clang-format on
        );
    }

    [[nodiscard]] auto& buffer() { return buffer_; }
    [[nodiscard]] const auto& buffer() const { return buffer_; }

    [[nodiscard]] std::size_t size() const { return size_; }
    void set_size(std::size_t size) { size_ = std::min(size, max_size()); }

  private:
    bitfilled::packed_integer<std::endian::little, bitfilled::byte_width(max_size()), std::size_t>
        size_{};
    std::array<T, MAX_SIZE> buffer_{};
};

} // namespace hid
