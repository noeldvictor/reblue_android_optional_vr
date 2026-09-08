/**
 * @brief Render-only pose timing and interpolation, independent of source lanes.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_transform.h"
#include <cstdint>
#include <span>

namespace bd::gpu::scene {
struct NativePosePhase {
  uint64_t tick = 0, frame = 0;
  float alpha = 0;
  bool interpolate = false;
  bool operator==(const NativePosePhase &) const = default;
};

// Preserve the existing render-only matrix blend and palette discontinuity
// contract, but apply it coherently to the entire owned pose, not 24 matrices
// in an emulated shader-upload stack. Root and attached native nodes share it.
inline bool NativePoseCanBlend(std::span<const RenderMatrix> previous,
                              std::span<const RenderMatrix> current) {
  if (previous.empty() || previous.size() != current.size()) return false;
  for (size_t n = 0; n < current.size(); ++n) {
    const auto &a = previous[n], &b = current[n];
    if (a[3] != 0 || a[7] != 0 || a[11] != 0 || a[15] != 1 ||
        b[3] != 0 || b[7] != 0 || b[11] != 0 || b[15] != 1) return false;
    for (size_t c = 0; c < 16; ++c) {
      const float delta = b[c] - a[c];
      if (!std::isfinite(a[c]) || !std::isfinite(b[c]) ||
          !std::isfinite(delta) || std::abs(delta) > 1.5f) return false;
    }
  }
  return true;
}
} // namespace bd::gpu::scene
