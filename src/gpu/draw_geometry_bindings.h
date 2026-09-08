/**
 * @brief Complete immediate geometry binding, independent of logical dirty flags.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <plume_render_interface_types.h>
#include <span>

namespace bd::gpu {
// Views are borrowed only for this immediate call; their producer owns the
// underlying resources through the submission fence. Dirty flags govern CPU
// preparation, not physical GPU state after queued/native/instanced draws.
// Validate the range before issuing commands, and never pass null-buffer gaps
// to Plume (they are unused slots, not a request to dereference/unbind null).
template <class Commands>
bool ApplyImmediateGeometryBindings(
    Commands &commands, const plume::RenderPipeline *pipeline, uint32_t first,
    std::span<const plume::RenderVertexBufferView> views,
    std::span<const plume::RenderInputSlot> slots,
    const plume::RenderIndexBufferView *indices) {
  if (!pipeline || first > 16 || views.size() > 16 - first ||
      views.size() != slots.size()) return false;
  commands.setPipeline(pipeline);
  uint32_t i = 0;
  while (i < views.size()) {
    if (!views[i].buffer.ref) { ++i; continue; }
    const uint32_t start = i++;
    while (i < views.size() && views[i].buffer.ref) ++i;
    commands.setVertexBuffers(first + start, views.data() + start, i - start,
                             slots.data() + start);
  }
  if (indices && indices->buffer.ref) commands.setIndexBuffer(indices);
  return true;
}
} // namespace bd::gpu
