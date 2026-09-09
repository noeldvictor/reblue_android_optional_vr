/**
 * @brief Shared animated-material boundary; source addresses never enter the native owner.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_uv.h"
#include <algorithm>
#include <bit>

namespace bd::gpu::scene::material_uv_source {
struct Binding { uint32_t table=0, count=0; };
struct Publication {
  Binding binding;
  NativeMaterialUVs material;
  std::array<std::array<float,2>,2> output; // all four floats, even for one record
};
template<class Read>
std::optional<Publication> ReadEyeControl(uint32_t visual, uint32_t controls, Read read,
    const NativeMaterialUVs *previous=nullptr) {
  if (!visual || (visual & 3) || visual > UINT32_MAX-3567 ||
      !controls || (controls & 3) || controls > UINT32_MAX-7) return {};
  const auto table=read(uint64_t(visual)+3560), count=read(uint64_t(visual)+3564);
  if (!table || !*table || (*table & 3) || !count || !*count || *count > 256 ||
      uint64_t(*table)+uint64_t(*count)*152 > uint64_t(UINT32_MAX)+1) return {};
  NativeEyeControl input;
  for (uint32_t axis=0; axis<2; ++axis) {
    const auto gaze=read(uint64_t(controls)+axis*4), origin=read(uint64_t(*table)+76+axis*4);
    const auto minimum=read(uint64_t(*table)+60+axis*4), maximum=read(uint64_t(*table)+68+axis*4);
    if (!gaze || !origin || !minimum || !maximum) return {};
    input.gaze[axis]=std::bit_cast<float>(*gaze); input.origin[axis]=std::bit_cast<float>(*origin);
    input.minimum[axis]=std::bit_cast<float>(*minimum); input.maximum[axis]=std::bit_cast<float>(*maximum);
  }
  const auto values=EvaluateNativeEyeUV(input);
  if (!values) return {};
  Publication result{{*table,*count},{},*values};
  result.material.count=*count;
  for (uint32_t i=0; i<std::min(*count,2u); ++i) {
    const uint64_t record=uint64_t(*table)+i*152;
    const auto selector=read(record+4), channel=read(record+8), enabled=read(record+20);
    if (!selector || !channel || !enabled) return {};
    result.material.entries.push_back({i,*selector,*channel,(*values)[i],*enabled != 0,NativeMaterialUVOrigin::Eye});
  }
  // The bridge validates the earlier writer BEFORE the eye caller changes its
  // output. Keep untouched slots in that same owner, never freeze a prior table.
  if (previous && previous->count == *count && previous->Valid())
    for (const auto &entry : previous->entries) if (entry.slot >= 2) result.material.entries.push_back(entry);
  return result;
}
// Validate before a producer reuses previous values, or at object setup after
// the caller's output copy and later writers. This checks the outgoing adapter;
// it never imports that adapter as native UV input.
template<class Read>
bool Matches(uint32_t visual, Binding binding, const NativeMaterialUVs &material, Read read) {
  if (!material.Valid() || !binding.table || !binding.count || binding.count > 256 ||
      material.count != binding.count ||
      read(uint64_t(visual)+3560) != std::optional(binding.table) ||
      read(uint64_t(visual)+3564) != std::optional(binding.count)) return false;
  for (const auto &entry : material.entries) {
    const uint64_t record=uint64_t(binding.table)+entry.slot*152;
    const auto enabled=read(record+20);
    if (!enabled || (*enabled != 0) != entry.enabled ||
        read(record+4) != std::optional(entry.selector) || read(record+8) != std::optional(entry.channel)) return false;
    for (uint32_t axis=0; axis<2; ++axis)
      if (read(record+28+axis*4) != std::optional(std::bit_cast<uint32_t>(entry.uv[axis]))) return false;
  }
  return true;
}
} // namespace bd::gpu::scene::material_uv_source
