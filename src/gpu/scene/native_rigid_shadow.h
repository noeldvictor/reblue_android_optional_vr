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
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_skin_mesh.h"

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
  std::optional<NativeBounds> skin_bounds;
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
  const char *reason = "invalid program or missing policy";
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
    bool shadow_deferred = false, bool skin = false,
    std::span<const NativePrimitivePolicy> owned_policies = {}, bool skin_scene = false, bool toon_scene = false) {
  if (!program.valid || program.ranges.empty() || program.ranges.size() > 4096 ||
      program.ranges.size() != program.geometries.size()) return {};
  const auto unsupported = SelectedNativeRigidShadow(program)
      ? NativeRigidCasterRoute::Refused : NativeRigidCasterRoute::Legacy;
  bool skinned = false;
  for (const auto &range : program.ranges) {
    if (!range.shader.vertex_bones) return {unsupported,{},"missing influence count"};
    if (*range.shader.vertex_bones) {
      if (!skin || *range.shader.vertex_bones > 3 || !range.skin || !range.skin->count)
        return {unsupported,{},"skin disabled or unsupported binding/count"};
      skinned = true;
    } else if (range.skin) return {unsupported,{},"rigid sibling retains a skin binding"};
  }
  if (!inputs) return {};
  const bool toon_shadow = skin && inputs->technique == 1 && inputs->phase == 1 && inputs->pass_mode == 0;
  const bool toon_surface = toon_scene && skin_scene && inputs->technique == 1 && inputs->phase == 0 && inputs->pass_mode == 0;
  if ((inputs->technique != 0 && !toon_shadow && !toon_surface) || inputs->phase > 1)
    return {unsupported,{},"unconverted technique/phase"};
  if (skinned && ((inputs->phase != 1 && !(skin_scene && inputs->phase == 0)) || inputs->pass_mode != 0))
    return {unsupported,{},"unconverted skin pass mode"};
  // Texture classification comes from the exact object scope's owned base
  // table and early overrides. A missing image must never mean ordinary.
  bool needs_owned = toon_shadow || toon_surface;
  if (inputs->texture_effects)
    for (const auto &step : program.policy_steps)
      needs_owned |= step.operation == PrimitivePolicyOperation::Texture;
  if (needs_owned && owned_policies.empty()) return {unsupported,{},"owned primitive participation unavailable"};
  std::vector<NativePrimitivePolicy> policies;
  // Recompose non-texture semantics and compare with the classified producer's
  // complete result. Ordinary below is only a reference: unknown/volume routes
  // cannot pass the owned-policy equality check, nor can altered alpha/cull/order.
  if (!ComposePrimitivePolicies(std::span(program.policy_steps), std::span(program.ranges), *inputs,
      [](const PrimitivePolicyStep &) { return PrimitiveTextureClass::Ordinary; }, policies)) return {};
  if (!owned_policies.empty()) {
    if (owned_policies.size() != policies.size()) return {unsupported,{},"owned primitive policy count mismatch"};
    for (size_t n = 0; n < policies.size(); ++n) {
      if (!owned_policies[n].routing_known) return {unsupported,{},"texture-dependent participation"};
      if (owned_policies[n] != policies[n]) return {NativeRigidCasterRoute::Refused,{},"owned primitive policy mismatch"};
    }
  }
  for (const auto &policy : policies) {
    if (!policy.routing_known) return {unsupported,{},"unknown primitive routing"};
    if (policy.deferred && !shadow_deferred) return {unsupported,{},"unconverted deferred participation"};
    if (policy.alpha_test && !scene_cutouts) return {unsupported,{},"unconverted cutout sibling"};
  }
  return {NativeRigidCasterRoute::Native, std::move(policies),"owned caster"};
}
inline NativeRigidCasterAdmission PrepareNativeRigidShadowAdmission(
    const NativeModelMaterialProgram &program, const std::optional<PrimitivePolicyInputs> &inputs, bool skin = false,
    std::span<const NativePrimitivePolicy> owned_policies = {}) {
  // Ordinary phase1 list entries have depth writes, no colour/stencil effects
  // and no depth sorting (light-space skips it). Min-depth plus discarded holes
  // can join the same native queue as solid casters without a guest list entry.
  const bool phase1 = inputs && inputs->phase == 1 && inputs->pass_mode == 0;
  auto admission = PrepareNativeRigidCasterAdmission(program, inputs, true, phase1, skin, owned_policies);
  if (admission.route != NativeRigidCasterRoute::Native) return admission;
  for (size_t n = 0; n < admission.policies.size(); ++n) if (admission.policies[n].alpha_test) {
    // Other phases/forced pass modes have different participation contracts.
    if (!phase1) return {SelectedNativeRigidShadow(program)
        ? NativeRigidCasterRoute::Refused : NativeRigidCasterRoute::Legacy};
  }
  return admission;
}
std::optional<std::vector<NativeRigidShadowPlan>> PrepareNativeRigidShadowForObject(
    const NativeInstancePose &pose, uint32_t node, const RenderCamera &camera, const char *&refusal, bool skin = false);
// Borrowed only within the exact current object/pose/pass scope. Preparation
// uses its immutable model/image owners, never source keys or shader registers.
std::span<const NativePrimitivePolicy> FindNativeShadowPoliciesForObject(
    const NativeInstancePose &pose, uint32_t node, const PrimitivePolicyInputs &inputs);
inline std::optional<NativeBounds> NativeSkinCasterBounds(const NativeModelMaterialProgram &program,
    const NativeInstancePose &pose, uint32_t node) {
  if (node >= pose.transforms.size() ||
      program.geometries.size() != program.ranges.size()) return {};
  std::optional<NativeBounds> result;
  for (size_t n = 0; n < program.ranges.size(); ++n) {
    if (!program.ranges[n].shader.vertex_bones) return {};
    const bool skin = *program.ranges[n].shader.vertex_bones != 0;
    if (skin && program.skin_geometries.size() != program.ranges.size()) return {};
    const auto &geometry = skin ? program.skin_geometries[n] : program.geometries[n];
    if (!geometry) return {};
    const auto bounds = skin ? TransformNativeSkinBounds(geometry->skin_bounds,pose.transforms) :
        geometry->bounds ? TransformNativeBounds(*geometry->bounds,pose.transforms[node]) : std::nullopt;
    if (!bounds) return {};
    if (!result) result = bounds;
    else for (uint32_t axis = 0; axis < 3; ++axis) {
      result->min[axis] = (std::min)(result->min[axis],bounds->min[axis]);
      result->max[axis] = (std::max)(result->max[axis],bounds->max[axis]);
    }
  }
  return result;
}
inline std::optional<std::vector<NativeRigidShadowPlan>> PrepareNativeRigidShadow(
    const NativeModelMaterialProgram &program, const RenderMatrix &world,
    const PrimitivePolicyInputs &inputs, const RenderCamera &camera,
    std::span<const NativeRigidShadowCutout> cutouts = {}, const NativeInstancePose *skin_pose = nullptr,
    std::span<const NativePrimitivePolicy> owned_policies = {}) {
  const auto admission = PrepareNativeRigidShadowAdmission(program, inputs,skin_pose != nullptr,owned_policies);
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
    const bool skinned = *program.ranges[n].shader.vertex_bones != 0;
    if (skinned && (!skin_pose || program.skin_geometries.size() != program.ranges.size())) return {};
    const auto &geometry = skinned ? program.skin_geometries[n] : program.geometries[n];
    if (!geometry || !geometry->id || !geometry->canonical_vertices ||
        !(skinned ? geometry->skin_shadow_vertex_input : geometry->rigid_vertex_input) ||
        geometry->stream_mask != 1 || !geometry->streams[0].buffer.ref || !geometry->index.buffer.ref ||
        !geometry->count || geometry->count % 3 || !geometry->strides[0] || geometry->strides[0] > 255) return {};
    const auto &policy = admission.policies[n];
    NativeRigidShadowPlan plan{geometry, object, pass, policy.cull, policy.direct || policy.deferred};
    if (skinned) {
      if (geometry->skin_influences != *program.ranges[n].shader.vertex_bones) return {};
      plan.skin_bounds = TransformNativeSkinBounds(geometry->skin_bounds,skin_pose->transforms);
      if (!plan.skin_bounds) return {};
    }
    if (policy.alpha_test) {
      if (cutouts.empty()) return {};
      const auto &cutout = cutouts[n];
      if (cutout.textured) {
        if (skinned && !geometry->skin_shadow_cutout_vertex_input) return {};
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
