/**
 * @brief Bounded native scheduling of scene-sampling visual primitives.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
#include <stdexcept>

namespace bd::gpu::scene {
inline constexpr uint32_t kDeferredVisualLimit = 512;

// Snapshot failure is a pre-effect refusal. Once it succeeds, failure must
// unwind without replaying any part of the original pass. Callbacks can replace
// entries/counts; reload at the original consumption points, not into a frozen
// list. The scene image owner, not a fixed design canvas, determines the extent.
template <class Port>
bool ExecuteDeferredVisuals(Port &port, uint32_t count) {
  if (!count) return true;
  if (count > kDeferredVisualLimit || !port.Snapshot()) return false;
  port.Begin();
  uint32_t mode = 5; // pass startup establishes the first shader recipe
  for (uint32_t index = 0;; ++index) {
    const auto live_count = port.Count();
    if (live_count > kDeferredVisualLimit)
      throw std::runtime_error("Native deferred visual queue exceeded its bound");
    if (index >= live_count) break;
    const uint32_t requested = port.Flags(index) & 0x40000 ? 5 : 6;
    if (requested != mode) { port.SelectMode(requested); mode = requested; }
    port.Primitive(index, mode == 6);
  }
  port.End();
  port.Clear();
  return true;
}
} // namespace bd::gpu::scene
