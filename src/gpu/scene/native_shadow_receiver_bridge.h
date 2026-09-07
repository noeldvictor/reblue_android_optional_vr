/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lighting.h"
#include <cmath>
namespace bd::gpu::scene {
class NativeReceiverColourPublication {
  std::optional<LightingVector> colour_;
  uint32_t visual_ = 0, frame_ = 0, view_ = 0;
public:
  void Reset() { colour_.reset(); }
  void Publish(LightingVector colour, uint32_t visual, uint32_t frame, uint32_t view) {
    Reset();
    if (!visual || view >= 16) return;
    for (float value : colour) if (!std::isfinite(value)) return;
    colour_ = colour; visual_ = visual; frame_ = frame; view_ = view;
  }
  std::optional<LightingVector> Read(uint32_t visual, uint32_t frame, uint32_t view) const {
    return visual == visual_ && frame == frame_ && view == view_ ? colour_ : std::nullopt;
  }
};
// Temporary visual identity is resolved at the object producer boundary only.
std::optional<LightingVector> FindNativePrimaryReceiverColour(uint32_t visual);
} // namespace bd::gpu::scene
