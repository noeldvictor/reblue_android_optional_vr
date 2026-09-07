/**
 * @brief Complete, source-free admission and inputs for an opaque rigid caster.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_mesh.h"
#include "gpu/scene/native_rigid_inputs.h"

namespace bd::gpu::scene {
// Temporary acceptance selector, never a substitute for semantic admission.
inline bool SelectedNativeRigidShadow(const NativeModelMaterialProgram &program) {
  for (const auto &geometry : program.geometries)
    if (geometry && geometry->id == 0x258694267A8DBAEEull) return true;
  return false;
}
struct NativeRigidShadowPlan {
  std::shared_ptr<const NativeGeometry> geometry;
  NativeRigidObjectGPU object{};
  NativeRigidPassGPU pass{};
  PrimitiveCull cull = PrimitiveCull::None;
  bool draw = false;
};
inline std::optional<NativeRigidShadowPlan> PrepareNativeRigidShadow(
    const NativeModelMaterialProgram &program, const RenderMatrix &world,
    const PrimitivePolicyInputs &inputs, const RenderCamera &camera) {
  // Whole-node admission. Expanding this family must preflight every sibling
  // before enqueue, including deferred and intentionally suppressed participants.
  if (!program.valid || program.ranges.size() != 1 || program.geometries.size() != 1 ||
      program.materials.size() != 1 || !program.geometries[0] || !program.materials[0] ||
      program.materials[0]->id != 0x63B8D67932573E51ull || inputs.technique != 0 || inputs.phase > 1)
    return {};
  const auto &range = program.ranges[0];
  const auto &geometry = program.geometries[0];
  if (!range.shader.vertex_bones || *range.shader.vertex_bones || range.skin ||
      !geometry->canonical_vertices || !geometry->rigid_vertex_input || geometry->stream_mask != 1 ||
      !geometry->streams[0].buffer.ref || !geometry->index.buffer.ref ||
      !geometry->count || geometry->count % 3 || !geometry->strides[0] || geometry->strides[0] > 255)
    return {};
  std::vector<NativePrimitivePolicy> policies;
  // An unowned volume/texture routing decision is not assumed to be ordinary.
  if (!ComposePrimitivePolicies(std::span(program.policy_steps), std::span(program.ranges), inputs,
      [](const PrimitivePolicyStep &) { return PrimitiveTextureClass::Unknown; }, policies) ||
      policies.size() != 1 || !policies[0].routing_known || policies[0].deferred || policies[0].alpha_test)
    return {};
  NativeRigidShadowPlan result;
  result.geometry = geometry; result.cull = policies[0].cull; result.draw = policies[0].direct;
  // The depth-only VS consumes exactly these two matrices. Zeroed unused fields
  // are padding for the shared layout, not claims of known scene light/material data.
  result.object.world = PackRigidMatrix(world);
  result.pass.world_to_shadow = PackRigidMatrix(camera.world_to_clip);
  if (!RigidFinite(result.object.world) || !RigidFinite(result.pass.world_to_shadow) ||
      world[3] != 0 || world[7] != 0 || world[11] != 0 || world[15] != 1) return {};
  return result;
}
} // namespace bd::gpu::scene
