/**
 * @brief Transitional engine producer for explicit queued graphics bindings.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/draw_bindings.h"

namespace bd::gpu {
// The engine ABI is confined to this producer. Device-owned sets/layout and
// frame-ring buffers survive the queue and GPU fence; offsets are copied.
// A native shader producer supplies its own GraphicsBindings instead.
template <class State> GraphicsBindings EngineGraphicsBindings(const State &state) {
  GraphicsBindings result;
  result.layout = state.pipeline_layout.get();
  result.sets[0] = state.texture_descriptor_set.get();
  result.sets[1] = state.sampler_descriptor_set.get();
  result.set_count = 2;
  if (state.constant_descriptor_set) {
    result.set_count = 3;
    result.sets[2] = state.constant_descriptor_set.get();
    result.dynamic_counts[2] = 3;
    for (uint32_t i = 0; i < 3; ++i) result.offsets[i] = state.constant_dyn_offsets[i];
    if (state.occlusion_counting) {
      result.set_count = 4;
      result.sets[3] = state.occlusion_descriptor_set[state.frame.load()].get();
    }
  }
  return result;
}
} // namespace bd::gpu
