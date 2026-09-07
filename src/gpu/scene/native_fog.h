/**
 * @brief Owned ordered fog layers for native object/pass submission.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lit_shading.h"
#include <array>
#include <optional>

namespace bd::gpu::scene {
using NativeFogLayers = std::array<LitFog, 2>;
struct NativeFogDefinition {
  LitVector direction{}, origin{}, colour{};
  float opacity = 0, start = 0, end = 0;
  bool disabled = true, radial = false;
  int blend = LitFogBlend;
};
inline std::optional<LitFog> ComposeNativeFog(const NativeFogDefinition &input) {
  const auto finite = [](LitVector value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
  };
  if (!finite(input.direction) || !finite(input.origin) || !finite(input.colour) ||
      !std::isfinite(input.opacity) || !std::isfinite(input.start) || !std::isfinite(input.end) ||
      input.blend < LitFogBlend || input.blend > LitFogSubtract ||
      (!input.disabled && input.end == input.start)) return {};
  LitFog result{};
  result.disabled = input.disabled;
  if (!input.disabled) {
    result.direction = input.direction; result.origin = input.origin; result.colour = input.colour;
    result.opacity = input.opacity; result.start = input.start; result.end = input.end;
    result.radial = input.radial; result.blend = input.blend;
  }
  return result;
}
} // namespace bd::gpu::scene
