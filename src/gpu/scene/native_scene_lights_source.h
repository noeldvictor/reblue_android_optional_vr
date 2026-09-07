/**
 * @brief Handoff-only imports; no dirty slots or shader cache/descriptors.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_scene_lights.h"
#include <bit>

namespace bd::gpu::scene {
template <class Read>
std::optional<NativeObjectLightInputs> ReadNativeObjectLightInputs(
    uint32_t visual, uint32_t node, Read read) {
  const auto word = [&](uint64_t address) -> std::optional<uint32_t> {
    return !address || (address & 3) || address > UINT32_MAX-3 ? std::nullopt : read(address);
  };
  const auto per_node = word(uint64_t(visual)+3380);
  if (!visual || node >= 4096 || !per_node) return {};
  uint64_t selection = uint64_t(visual)+3132;
  if (*per_node) {
    const auto table = word(uint64_t(visual)+3376);
    const auto entry = table && *table ? word(uint64_t(*table)+uint64_t(node)*4) : std::nullopt;
    // Null means inherited callback state, not the default object selection.
    // Refuse until that inheritance has a native producer of its own.
    if (!entry || !*entry) return {};
    selection = *entry;
  }
  const auto kind = word(selection), x = word(selection+200), y = word(selection+204);
  const auto z = word(selection+208), radius = word(selection+212);
  if (!kind || *kind >= 32 || !x || !y || !z || !radius) return {};
  NativeObjectLightInputs result{LitVec(std::bit_cast<float>(*x), std::bit_cast<float>(*y),
      std::bit_cast<float>(*z)), std::bit_cast<float>(*radius), *kind};
  if (!std::isfinite(result.centre.x) || !std::isfinite(result.centre.y) ||
      !std::isfinite(result.centre.z) || !std::isfinite(result.radius) || result.radius < 0) return {};
  return result;
}

// The caller owns synchronization, not this reader. Invoke only at the completed
// game/render handoff. Record order is authored tie order and ID is its index.
template <class Read>
std::optional<NativeSceneLightSet> ReadNativeSceneLightSet(uint32_t manager,
    bool special_scene, NativeLightScoreParameters scoring, float strength_numerator, Read read) {
  if (!manager || (manager & 3) || !std::isfinite(strength_numerator)) return {};
  bool valid = true;
  const auto word = [&](uint64_t address) {
    const auto value = !address || (address & 3) || address > UINT32_MAX-3 ? std::nullopt : read(address);
    if (!value) valid = false;
    return value.value_or(0);
  };
  const auto scalar = [&](uint64_t address) { return std::bit_cast<float>(word(address)); };
  const auto vector = [&](uint64_t address) { return LitVec(scalar(address),scalar(address+4),scalar(address+8)); };
  NativeSceneLightSet result;
  result.count = word(uint64_t(manager)+46816); result.mode = word(manager);
  // Rendering uses the non-primary thread's priority, not the publishing game thread's.
  result.priority_light = std::bit_cast<int32_t>(word(uint64_t(manager)+48032));
  result.scoring = scoring; result.special_scene = special_scene;
  if (!valid || result.count > result.kMaxLights || result.mode > 2) return {};
  for (size_t n = 0; n < result.count; ++n) {
    const uint64_t source = uint64_t(manager)+24016+n*76;
    auto &light = result.lights[n];
    auto &candidate = light.candidate;
    const auto flags = word(source);
    candidate.id = std::bit_cast<int32_t>(word(source+12));
    if (!valid || candidate.id != int32_t(n)) return {};
    candidate.kind = flags & 3; candidate.enabled = flags & 16; candidate.priority = flags & 32;
    candidate.excluded_objects = word(source+4); candidate.views = word(source+8);
    candidate.position = vector(source+20); candidate.direction = vector(source+32);
    candidate.cone_angle = scalar(source+56); candidate.intensity = scalar(source+64);
    candidate.range = scalar(source+72);
    NativeLightDefinition definition{candidate.position,candidate.direction,vector(source+44),
        candidate.range,candidate.cone_angle,scalar(source+60),candidate.kind};
    if (!valid) return {};
    const float strength = float(double(strength_numerator)/double(float(definition.cone_softness+1.f)));
    const float cone = float(std::cos(double(definition.cone_angle)));
    light.value = ComposeSelectedLight(definition, strength, cone);
    // Unused/disabled invalid numerics do not poison unrelated objects/views.
    // A selected unsupported light explicitly refuses that native packet.
  }
  return result;
}
} // namespace bd::gpu::scene
