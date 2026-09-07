/**
 * @brief Authored light definitions and owned object-selected lighting.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lit_shading.h"
#include <array>
#include <cstddef>
#include <optional>

namespace bd::gpu::scene {
struct NativeLightDefinition {
  LitVector position{}, direction{}, colour{};
  float range = 0, cone_angle = 0, cone_softness = 0;
  int kind = LitDisabled;
};
using NativeSelectedLights = std::array<LitLight, 3>;
struct NativeNodeSelectedLights {
  size_t node = 0;
  NativeSelectedLights lights{};
};
inline std::optional<NativeSelectedLights> SelectNativeObjectLights(
    const std::optional<NativeSelectedLights> &object,
    const std::optional<NativeNodeSelectedLights> &override, size_t node) {
  if (override && override->node == node) return override->lights;
  return object;
}

inline std::optional<LitLight> ComposeSelectedLight(const NativeLightDefinition &input,
                                                   float cone_strength, float cone_cosine) {
  const auto finite = [](LitVector value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
  };
  if (input.kind < LitDisabled || input.kind > LitPoint || !finite(input.position) ||
      !finite(input.direction) || !finite(input.colour)) return {};
  LitLight result{};
  result.kind = input.kind;
  if (input.kind == LitDisabled) return result;
  result.position = input.position; result.direction = input.direction; result.colour = input.colour;
  if (input.kind == LitPoint || input.kind == LitSpot) {
    if (!std::isfinite(input.range) || input.range <= 0) return {};
    result.inverse_range = 1.0f / input.range;
    if (!std::isfinite(result.inverse_range)) return {};
  }
  if (input.kind == LitSpot) {
    if (!std::isfinite(cone_strength) || !std::isfinite(cone_cosine) ||
        cone_cosine < -1 || cone_cosine >= 1) return {};
    result.cone_strength = cone_strength; result.cone_cosine = cone_cosine;
  }
  // Unused directional range/cone data and disabled records are canonical zero,
  // not inherited infinities or the previous light's shader payload.
  return result;
}
} // namespace bd::gpu::scene
