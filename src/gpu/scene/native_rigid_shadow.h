/**
 * @brief Complete, source-free admission and inputs for rigid shadow casters.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_mesh.h"
#include "gpu/scene/native_rigid_inputs.h"
#include "gpu/scene/native_texture_binding.h"

namespace bd::gpu::scene {
struct NativeInstancePose;
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
  NativeTextureGpuHandle albedo;
  std::optional<NativeMaterialSampler2D> sampler;
};
struct NativeRigidShadowCutout {
  NativeTextureBinding image;
  std::array<float, 4> uv{};
  std::optional<NativeMaterialSampler2D> sampler;
  bool textured = false, owns_uv = false;
};
enum class NativeRigidCasterRoute { Legacy, Native, Refused };
struct NativeRigidCasterAdmission {
  NativeRigidCasterRoute route = NativeRigidCasterRoute::Refused;
  std::vector<NativePrimitivePolicy> policies;
};
// Classify the authored family before consulting a pose or GPU allocation.
// Depth-only opaque casting does not consume material colour, texture layers,
// samplers or lights. Skin/deformation, alpha and volume/effect routing do.
// Cutout admission is shared by the scene and texture-owning shadow producers.
// The opaque-only builder below still refuses missing cutout packets.
// The original selected asset stays fail-closed even if its contract regresses.
inline NativeRigidCasterAdmission PrepareNativeRigidCasterAdmission(
    const NativeModelMaterialProgram &program,
    const std::optional<PrimitivePolicyInputs> &inputs, bool scene_cutouts = false,
    bool shadow_deferred = false) {
  if (!program.valid || program.ranges.empty() || program.ranges.size() > 4096 ||
      program.ranges.size() != program.geometries.size()) return {};
  const auto unsupported = SelectedNativeRigidShadow(program)
      ? NativeRigidCasterRoute::Refused : NativeRigidCasterRoute::Legacy;
  for (const auto &range : program.ranges)
    if (!range.shader.vertex_bones || *range.shader.vertex_bones || range.skin)
      return {unsupported};
  if (!inputs) return {};
  if (inputs->technique != 0 || inputs->phase > 1) return {unsupported};
  // This family deliberately excludes texture-dependent effect participation.
  // Do not classify a missing image as an ordinary volume-free material.
  if (inputs->texture_effects)
    for (const auto &step : program.policy_steps)
      if (step.operation == PrimitivePolicyOperation::Texture) return {unsupported};
  std::vector<NativePrimitivePolicy> policies;
  if (!ComposePrimitivePolicies(std::span(program.policy_steps), std::span(program.ranges), *inputs,
      [](const PrimitivePolicyStep &) { return PrimitiveTextureClass::Unknown; }, policies)) return {};
  for (const auto &policy : policies)
    if (!policy.routing_known || (policy.deferred && !shadow_deferred) ||
        (policy.alpha_test && !scene_cutouts)) return {unsupported};
  return {NativeRigidCasterRoute::Native, std::move(policies)};
}
inline NativeRigidCasterAdmission PrepareNativeRigidShadowAdmission(
    const NativeModelMaterialProgram &program, const std::optional<PrimitivePolicyInputs> &inputs) {
  // Ordinary phase1 list entries have depth writes, no colour/stencil effects
  // and no depth sorting (light-space skips it). Min-depth plus discarded holes
  // can join the same native queue as solid casters without a guest list entry.
  const bool phase1 = inputs && inputs->phase == 1 && inputs->pass_mode == 0;
  auto admission = PrepareNativeRigidCasterAdmission(program, inputs, true, phase1);
  if (admission.route != NativeRigidCasterRoute::Native) return admission;
  for (size_t n = 0; n < admission.policies.size(); ++n) if (admission.policies[n].alpha_test) {
    // Other phases/forced pass modes have different participation contracts.
    if (!phase1) return {SelectedNativeRigidShadow(program)
        ? NativeRigidCasterRoute::Refused : NativeRigidCasterRoute::Legacy};
  }
  return admission;
}
std::optional<std::vector<NativeRigidShadowPlan>> PrepareNativeRigidShadowForObject(
    const NativeInstancePose &pose, uint32_t node, const RenderCamera &camera, const char *&refusal);
inline std::optional<std::vector<NativeRigidShadowPlan>> PrepareNativeRigidShadow(
    const NativeModelMaterialProgram &program, const RenderMatrix &world,
    const PrimitivePolicyInputs &inputs, const RenderCamera &camera,
    std::span<const NativeRigidShadowCutout> cutouts = {}) {
  const auto admission = PrepareNativeRigidShadowAdmission(program, inputs);
  if (admission.route != NativeRigidCasterRoute::Native) return {};
  if (!cutouts.empty() && cutouts.size() != program.ranges.size()) return {};
  NativeRigidObjectGPU object{};
  NativeRigidPassGPU pass{};
  // The depth-only VS consumes exactly these two matrices. Zeroed unused fields
  // are padding for the shared layout, not claims of known scene light/material data.
  object.world = PackRigidMatrix(world);
  pass.world_to_shadow = PackRigidMatrix(camera.world_to_clip);
  if (!RigidFinite(object.world) || !RigidFinite(pass.world_to_shadow) ||
      world[3] != 0 || world[7] != 0 || world[11] != 0 || world[15] != 1) return {};
  std::vector<NativeRigidShadowPlan> result;
  result.reserve(program.ranges.size());
  // Preflight every sibling. No partial plan can escape on a late failure.
  for (size_t n = 0; n < program.ranges.size(); ++n) {
    const auto &geometry = program.geometries[n];
    if (!geometry || !geometry->id || !geometry->canonical_vertices || !geometry->rigid_vertex_input ||
        geometry->stream_mask != 1 || !geometry->streams[0].buffer.ref || !geometry->index.buffer.ref ||
        !geometry->count || geometry->count % 3 || !geometry->strides[0] || geometry->strides[0] > 255) return {};
    const auto &policy = admission.policies[n];
    NativeRigidShadowPlan plan{geometry, object, pass, policy.cull, policy.direct || policy.deferred};
    if (policy.alpha_test) {
      if (cutouts.empty()) return {};
      const auto &cutout = cutouts[n];
      if (cutout.textured) {
        if (!policy.deferred) return {}; // direct callbacks select shadownull VS
        const auto &image = cutout.image.primary;
        if (!cutout.owns_uv || !image || !image->image || !image->view || cutout.image.slice_2d || cutout.image.cube ||
            image->dimension != plume::RenderTextureViewDimension::TEXTURE_2D_ARRAY || !cutout.sampler ||
            cutout.sampler->u != MaterialSampleAddress::Wrap || cutout.sampler->v != MaterialSampleAddress::Wrap) return {};
        plan.object.uv_scale_offset = {1.f/512, 1.f/512, 1.f/512+cutout.uv[0], 1.f/512+cutout.uv[1]};
        if (!RigidFinite(plan.object.uv_scale_offset)) return {};
        plan.object.flags = {RigidCutout | RigidAlbedo, 1, RigidCutoutGE, std::bit_cast<uint32_t>(.6f)};
        plan.albedo = image; plan.sampler = cutout.sampler;
      }
    }
    result.push_back(std::move(plan));
  }
  return result;
}
} // namespace bd::gpu::scene
