/**
 * @brief Ordered, owned material cutoffs, independent of device/shader state.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_data.h"

namespace bd::gpu::scene {
struct NativeMaterialAlphaInputs {
  uint32_t direct_reference = 0, sorted_reference = 0;
  std::optional<uint32_t> object_reference;
};
// bdSceneNodeDrawSingle resolves an unset reference at EVERY primitive, before
// suppression. The first nonzero default sticks until another control resets
// it. The object override affects the final setter, not that running value.
inline bool ComposeMaterialAlphaReferences(std::span<const NativeMaterialRange> ranges,
    std::span<const NativePrimitivePolicy> policies, const NativeMaterialAlphaInputs &inputs,
    std::vector<uint32_t> &out) {
  if (ranges.empty() || ranges.size() > 4096 || ranges.size() != policies.size()) return false;
  std::optional<uint32_t> current = 0;
  std::vector<uint32_t> values;
  values.reserve(ranges.size());
  for (size_t n = 0; n < ranges.size(); ++n) {
    if (!policies[n].routing_known) return false;
    if (ranges[n].resets_alpha_reference) current = ranges[n].alpha_reference;
    if (!current) return false;
    // A suppressed sorted primitive still resolves the sorted default before
    // skipping its draw. Do not infer this from the emitted/deferred counts.
    if (!*current) current = policies[n].sorted ? inputs.sorted_reference : inputs.direct_reference;
    // Sorted entries copy the running reference before the direct-only object
    // override setter (822809A4..D0); do not apply that setter to a list packet.
    values.push_back(policies[n].deferred ? *current : inputs.object_reference.value_or(*current));
  }
  out = std::move(values);
  return true;
}
} // namespace bd::gpu::scene
