/**
 * @brief Owned three-light selection, independent of shader callbacks/storage.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_selected_lights.h"
#include <cstdint>

namespace bd::gpu::scene {
struct NativeLightCandidate {
  int32_t id = -1;
  int kind = LitDisabled;
  bool enabled = false, priority = false;
  uint32_t views = 0, excluded_objects = 0;
  LitVector position{}, direction{};
  float range = 0, intensity = 0, cone_angle = 0;
};
struct NativeLightSelectionSlot {
  int16_t id = -1;
  uint8_t weight = 0, priority = 0;
  bool operator==(const NativeLightSelectionSlot &) const = default;
};
struct NativeLightSelection {
  std::array<NativeLightSelectionSlot, 3> slots{};
  uint8_t category = 0;
  bool operator==(const NativeLightSelection &) const = default;
};
struct NativeLightSelectionInputs {
  LitVector centre{};
  float radius = 0;
  uint32_t object_class = 0, view = 0, mode = 0;
  int32_t priority_light = -1;
  bool special_scene = false;
};
struct NativeLightScoreParameters {
  float scale = 0, angle_scale = 0, ray_min = 0, ray_max = 0;
};

// Explicit float rounding follows the authored spatial/score operations. Keep
// contraction off: one quantization-boundary change can reorder selected lights.
#if defined(__clang__)
#pragma clang fp contract(off)
#endif
inline std::optional<uint8_t> ScoreNativeLight(const NativeLightCandidate &light,
    const NativeLightSelectionInputs &input, const NativeLightScoreParameters &parameters) {
  const auto finite = [](LitVector v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
  };
  if (!std::isfinite(parameters.scale)) return {};
  float score = 0;
  if (light.kind == LitDirectional) {
    if (!std::isfinite(light.intensity)) return {};
    score = light.intensity;
  } else if (light.kind == LitPoint || light.kind == LitSpot) {
    if (!finite(input.centre) || !finite(light.position) || !std::isfinite(input.radius) ||
        input.radius < 0 || !std::isfinite(light.range) || !std::isfinite(light.intensity)) return {};
    auto ray = LitVec(float(input.centre.x-light.position.x), float(input.centre.y-light.position.y),
                      float(input.centre.z-light.position.z));
    const float xx = ray.x*ray.x, yy = ray.y*ray.y, zz = ray.z*ray.z;
    const float distance = float(std::sqrt(double(float(float(xx+yy)+zz))));
    const float attenuation = float(double(float(distance-light.range))/double(input.radius));
    // The ordered comparisons intentionally send NaN (including 0/0) to 1.
    const float clamped = !(attenuation < 1.f) ? 1.f : attenuation < 0.f ? 0.f : attenuation;
    score = float(float(1.f-clamped)*light.intensity);
    if (light.kind == LitSpot) {
      if (!finite(light.direction) || !std::isfinite(light.cone_angle) ||
          !std::isfinite(parameters.ray_min) || !std::isfinite(parameters.ray_max) ||
          !std::isfinite(parameters.angle_scale)) return {};
      // The authored ray keeps a near-zero displacement unnormalized.
      if (!(distance >= parameters.ray_min && distance <= parameters.ray_max)) {
        const float inverse = float(1.0/double(distance));
        ray = LitVec(float(ray.x*inverse), float(ray.y*inverse), float(ray.z*inverse));
      }
      if (!(distance < float(input.radius+parameters.ray_max))) {
        const float x = light.direction.x*ray.x, y = light.direction.y*ray.y, z = light.direction.z*ray.z;
        const float dot = float(float(x+y)+z);
        if (!(dot > 0.f)) score = 0;
        else {
          const float angle = float(std::acos(double(dot)));
          const float arc = float(float(float(angle-light.cone_angle)*parameters.angle_scale)*distance);
          if (arc > float(input.radius-parameters.ray_max)) score = 0;
        }
      }
    }
  }
  const float quantized = score*parameters.scale;
  // Avoid undefined float->integer conversion; matches fctiwz then [0,255].
  if (!(quantized > 0.f)) return uint8_t(0);
  if (quantized >= 255.f) return uint8_t(255);
  return uint8_t(quantized);
}

// Category comes from the LIVE light owner, not the potentially older snapshot
// used for scoring. The callback returns a semantic category for the new set.
template <class Classify>
bool InsertNativeLight(NativeLightSelection &selection, const NativeLightCandidate &candidate,
    const NativeLightSelectionInputs &input, const NativeLightScoreParameters &parameters, Classify classify) {
  if (input.view >= 16 || input.object_class >= 32 ||
      (input.special_scene && input.object_class > 19)) return false;
  if (!candidate.enabled || !(candidate.views & (1u << input.view))) return true;
  const bool excluded = candidate.excluded_objects & (1u << input.object_class);
  const bool special_exemption = input.special_scene && input.view != 8 &&
      input.object_class >= 1 && input.object_class <= 8;
  if (excluded && !special_exemption) return true;
  if (!input.mode) {
    for (auto &slot : selection.slots) slot.id = -1;
    return true; // Preserve weights, priorities and category in this mode.
  }
  if (input.mode >= 3) return true;
  if (candidate.id < 0 || candidate.id > INT16_MAX) return false;
  if (input.mode == 1) {
    auto &first = selection.slots[0];
    if (candidate.kind != LitDirectional || first.priority) return true;
    if (candidate.priority) first = {int16_t(candidate.id), 0, 1};
    else {
      const auto weight = ScoreNativeLight(candidate, input, parameters);
      if (!weight) return false;
      if (*weight > first.weight) { first.id = int16_t(candidate.id); first.weight = *weight; }
    }
    return true; // Directional-only mode does not publish a new category.
  }
  const auto weight = ScoreNativeLight(candidate, input, parameters);
  if (!weight) return false;
  size_t position = 0;
  const bool sun = candidate.id == input.priority_light;
  if (sun) { if (!*weight) return true; }
  else {
    for (; position < selection.slots.size(); ++position) {
      const auto &slot = selection.slots[position];
      if (!slot.priority && (candidate.priority ? *weight != 0 : *weight > slot.weight)) break;
    }
    if (position == selection.slots.size()) return true;
  }
  auto next = selection;
  for (size_t n = next.slots.size()-1; n > position; --n) next.slots[n] = next.slots[n-1];
  next.slots[position] = {int16_t(candidate.id), *weight, uint8_t(sun || candidate.priority)};
  const auto category = classify(next);
  if (!category) return false;
  next.category = *category;
  selection = next;
  return true;
}
} // namespace bd::gpu::scene
