/**
 * @brief Late owned visual effects consumed by native sorted scene packets.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_rigid_scene.h"
#include "gpu/scene/native_shadow_receiver_bridge.h"
#include <cstring>

namespace bd::gpu::scene {
struct NativeDeferredEffects {
  uint32_t frame = 0;
  BlendState blend;
  bool alpha_to_coverage = false;
  NativePrimaryReceiver receiver;
};
// Validate every sibling before changing any. The source visual/registry,
// material staging, descriptor and graphics-device caches are not inputs.
inline bool FinalizeNativeDeferredEffects(NativeRigidSceneSubmission &submission,
    const NativeDeferredEffects &effects, uint32_t frame) {
  if (!submission.instance || !submission.model_generation || submission.frame != frame ||
      effects.frame != frame || submission.plans.empty() || submission.plans.size() > 4096 ||
      !effects.blend.alphaBlendEnable || !effects.receiver.image) return false;
  for (float value : effects.receiver.colour) if (!std::isfinite(value)) return false;
  for (float value : effects.receiver.world_to_shadow) if (!std::isfinite(value)) return false;
  const auto matrix = PackRigidMatrix(effects.receiver.world_to_shadow);
  for (const auto &plan : submission.plans)
    if (!plan.deferred || !plan.draw || plan.shadow != effects.receiver.image ||
        std::memcmp(&matrix, &plan.pass.world_to_shadow, sizeof(matrix))) return false;
  for (auto &plan : submission.plans) {
    plan.blend = effects.blend;
    plan.alpha_to_coverage = effects.alpha_to_coverage;
    const auto &colour = effects.receiver.colour;
    plan.pass.shadow_colour_strength = {colour[0], colour[1], colour[2], colour[3]};
  }
  return true;
}
} // namespace bd::gpu::scene
