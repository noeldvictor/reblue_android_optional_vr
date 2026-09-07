/**
 * @brief Owned ordinary 2D material sampling, independent of device/fetch state.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace bd::gpu::scene {
enum class MaterialSampleFilter : uint8_t { Nearest, Linear };
enum class MaterialSampleAddress : uint8_t { Wrap, Mirror, Clamp, Unknown };
enum class MaterialFilterField : uint8_t { Min, Mag, Mip };
struct NativeSamplerFilters {
  MaterialSampleFilter min = MaterialSampleFilter::Linear, mag = MaterialSampleFilter::Linear;
  MaterialSampleFilter mip = MaterialSampleFilter::Linear;
  bool operator==(const NativeSamplerFilters &) const = default;
};
using NativeSamplerFilterPass = std::array<std::optional<NativeSamplerFilters>, 5>;
struct NativeSamplerAddress {
  MaterialSampleAddress u = MaterialSampleAddress::Unknown, v = MaterialSampleAddress::Unknown;
  bool operator==(const NativeSamplerAddress &) const = default;
};
// Ordinary node entry resets channels0/1/2/4. Channel3 inherits, not a wrap default.
inline std::array<NativeSamplerAddress, 5> MaterialSamplerEntry() {
  constexpr auto wrap = MaterialSampleAddress::Wrap;
  return {{{wrap, wrap}, {wrap, wrap}, {wrap, wrap}, {}, {wrap, wrap}}};
}
struct NativeMaterialSampler2D {
  NativeSamplerFilters filters;
  MaterialSampleAddress u, v;
  bool operator==(const NativeMaterialSampler2D &) const = default;
};
using NativeMaterialSamplers = std::array<std::optional<NativeMaterialSampler2D>, 5>;
inline NativeMaterialSamplers ComposeMaterialSamplers(
    const std::array<NativeSamplerAddress, 5> &addresses, const NativeSamplerFilterPass &filters) {
  NativeMaterialSamplers result;
  for (size_t i = 0; i < result.size(); ++i)
    if (filters[i] && addresses[i].u != MaterialSampleAddress::Unknown && addresses[i].v != MaterialSampleAddress::Unknown)
      result[i] = NativeMaterialSampler2D{*filters[i], addresses[i].u, addresses[i].v};
  return result;
}
// Single current publication; no history or source lookup. Late setters update
// only a known current slot. Unknown production cannot invent the other fields.
class NativeSamplerFilterPublication {
  std::optional<NativeSamplerFilterPass> pass_;
  uint32_t frame_ = 0, view_ = 0;
public:
  void Publish(NativeSamplerFilterPass pass, uint32_t frame, uint32_t view) {
    pass_ = pass; frame_ = frame; view_ = view;
  }
  void Reset() { pass_.reset(); }
  std::optional<NativeSamplerFilterPass> Read(uint32_t frame, uint32_t view) const {
    return frame == frame_ && view == view_ ? pass_ : std::nullopt;
  }
  void Set(uint32_t frame, uint32_t view, uint32_t slot, MaterialFilterField field,
           std::optional<MaterialSampleFilter> value) {
    if (!pass_ || slot >= pass_->size()) return;
    if (frame != frame_ || view != view_) { Reset(); return; }
    auto &filters = (*pass_)[slot];
    if (!value) { filters.reset(); return; }
    if (!filters) return;
    switch (field) {
    case MaterialFilterField::Min: filters->min = *value; break;
    case MaterialFilterField::Mag: filters->mag = *value; break;
    case MaterialFilterField::Mip: filters->mip = *value; break;
    }
  }
};
} // namespace bd::gpu::scene
