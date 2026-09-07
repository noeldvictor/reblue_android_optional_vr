/**
 * @brief Load-owned admission before pose fallback, culling or legacy drawing.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_rigid_shadow.h"

namespace bd::gpu::scene {
enum class NativeRigidRoute { Legacy, Scene, Shadow, Refused };
struct NativeRigidRouteDecision {
  NativeRigidRoute route;
  const char *refusal = nullptr;
};

inline bool NativeRigidFamilyKnown(const NativeModelMaterialProgram &program) {
  if (!program.valid || program.ranges.size() != program.geometries.size()) return false;
  for (const auto &geometry : program.geometries)
    if (!geometry || !geometry->id) return false;
  return true;
}

// Hard-off acceptance only. A missing owner is unclassifiable, NOT evidence
// that this is an unconverted family. Never discover selection from a pose:
// that would allow a missing/stale pose to select the legacy path itself.
inline NativeRigidRouteDecision PrepareNativeRigidRoute(
    const NativeModelRenderHandle &model, const NativeInstancePose *pose,
    uint32_t node, uint32_t view) {
  if (!model) return {NativeRigidRoute::Refused, "load-owned model unavailable"};
  const auto *program = model->FindNode(node);
  if (!program) return {NativeRigidRoute::Refused, "load-owned node unavailable or ambiguous"};
  if (!NativeRigidFamilyKnown(*program))
    return {NativeRigidRoute::Refused, "load-owned geometry identity unavailable"};
  if (!SelectedNativeRigidShadow(*program)) return {NativeRigidRoute::Legacy};
  if (!pose) return {NativeRigidRoute::Refused, "selected native pose unavailable"};
  if (!pose->instance || pose->model != model || pose->model_generation != model->Generation())
    return {NativeRigidRoute::Refused, "selected native pose belongs to another model generation"};
  if (node >= pose->transforms.size())
    return {NativeRigidRoute::Refused, "selected native transform unavailable"};
  if (!program->bounds) return {NativeRigidRoute::Refused, "selected native bounds unavailable"};
  if (view == 1) return {NativeRigidRoute::Shadow};
  if (view == 3) return {NativeRigidRoute::Scene};
  return {NativeRigidRoute::Refused, "selected render view has no native route"};
}

inline bool NativeRigidLegacyAllowed(const ModelMaterialImport *mesh) {
  return mesh && NativeRigidFamilyKnown(mesh->program) && !SelectedNativeRigidShadow(mesh->program);
}
} // namespace bd::gpu::scene
