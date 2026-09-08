/**
 * @brief Whole-node native opaque/cutout scene admission and retained inputs.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_rigid_shadow.h"
#include "gpu/scene/native_object_primitive.h"
#include "gpu/scene/native_texture_binding.h"
#include "gpu/scene/native_shadow.h"
#include "gpu/scene/native_scene_lights.h"
#include "gpu/native_target_images.h"
#include "gpu/scene/native_blend.h"
#include "gpu/scene/deferred_depth.h"

namespace bd::gpu::scene {
struct NativeRigidReceiver {
  NativeTargetImageHandle image;
  RenderMatrix world_to_shadow{};
  LightingVector colour{};
  NativeShadowInputs visibility;
};
struct NativeRigidCutoutInputs {
  uint32_t reference = 0, comparison = RigidCutoutGE;
  bool alpha_to_coverage = false;
  BlendState blend;
};
struct NativeRigidScenePlan {
  std::shared_ptr<const NativeGeometry> geometry;
  NativeVertexInputHandle vertex_input;
  std::array<NativeTextureGpuHandle, 3> albedo;
  NativeTargetImageHandle shadow;
  std::array<NativeMaterialSampler2D, 3> samplers{};
  NativeRigidObjectGPU object;
  NativeRigidPassGPU pass;
  PrimitiveCull cull;
  bool draw;
  std::optional<NativeSceneLightTicket> light_ticket;
  BlendState blend;
  bool alpha_to_coverage = false;
  uint32_t primitive = 0;
  bool deferred = false, depth_write = true;
  float depth = 0;
  // Pending recipes are not drawable until finalization installs a current
  // ticket and repacks all three light slots at the actual submission position.
  std::optional<NativeSceneLightRecipe> light_recipe;
  std::shared_ptr<const NativeInstancePose> skin_pose;
  std::optional<NativeBounds> skin_bounds;
};
struct NativeRigidSceneSubmission {
  uint64_t instance = 0, model_generation = 0;
  uint32_t node = 0, frame = 0;
  bool regression_node = false;
  std::vector<NativeRigidScenePlan> plans;
};
struct NativeRigidDeferredInputs { bool fixed = false; float fixed_depth = 0; };
// Transitional object producer: resolves source bindings before returning the
// retained, address-free plan. It never interprets/captures/replays the node.
std::optional<std::vector<NativeRigidScenePlan>> PrepareNativeRigidSceneForObject(
    const NativeInstancePose &pose, uint32_t node, const char *&refusal);
bool StageNativeRigidSceneDeferredForObject(NativeRigidSceneSubmission &submission);
// Transactional whole-node finalization: no sibling may retain an early light
// value, reuse a ticket, or disagree with the common authored Bind/Keep action.
inline bool FinalizeNativeRigidSceneLights(std::span<NativeRigidScenePlan> plans,
    const NativeSceneLightTicket &ticket) {
  if (plans.empty() || !ticket.update || ticket.revision == UINT64_MAX || !plans.front().light_recipe) return false;
  const auto &recipe = *plans.front().light_recipe;
  if (recipe.update != ticket.update ||
      (recipe.bind && !SameNativeSelectedLights(*recipe.bind, ticket.lights)) ||
      ticket.inherited != !recipe.bind.has_value()) return false;
  for (const auto &plan : plans) {
    if (plan.light_ticket || !plan.light_recipe || plan.light_recipe->update != recipe.update ||
        plan.light_recipe->bind.has_value() != recipe.bind.has_value() ||
        (recipe.bind && !SameNativeSelectedLights(*plan.light_recipe->bind, *recipe.bind))) return false;
  }
  NativeRigidPassGPU packed{};
  if (!SetRigidLights(packed, ticket.lights)) return false;
  for (auto &plan : plans) {
    for (uint32_t n = 0; n < 3; ++n) plan.pass.lights[n] = packed.lights[n];
    plan.light_ticket = ticket;
  }
  return true;
}
// Share rigid participation rules with casting, then classify the scene shader
// from authored recipes, before a missing pose/texture can choose legacy drawing.
inline NativeRigidCasterAdmission PrepareNativeRigidSceneAdmission(
    const NativeModelMaterialProgram &program,
    const std::optional<PrimitivePolicyInputs> &inputs, bool deferred = false, bool skin = false) {
  const bool ordinary_deferred = deferred && inputs && inputs->phase == 0 && inputs->pass_mode == 0;
  auto admission = PrepareNativeRigidCasterAdmission(program, inputs, true, ordinary_deferred, skin, {}, skin);
  if (admission.route != NativeRigidCasterRoute::Native) return admission;
  const auto unsupported = SelectedNativeRigidShadow(program)
      ? NativeRigidCasterRoute::Refused : NativeRigidCasterRoute::Legacy;
  if (inputs->phase != 0) return {unsupported};
  if (program.materials.size() != program.ranges.size()) return {};
  for (size_t n = 0; n < program.ranges.size(); ++n) {
    const auto &range = program.ranges[n];
    if (range.shader.texture_layers > 3 || range.reflection.enabled || range.features.normal_mapping_requested)
      return {unsupported};
    if (!range.shader.vertex_colour || !program.materials[n] ||
        range.features.diffuse == MaterialDiffuseMode::Unknown || !range.features.specular_requested) return {};
  }
  return admission;
}
inline std::optional<NativeRigidScenePlan> PrepareNativeRigidScene(
    const NativeModelMaterialProgram &program,
    const NativeObjectPrimitive<NativeTextureBinding> &packet,
    const NativeRigidReceiver &receiver, const char **refusal = nullptr,
    std::optional<NativeRigidCutoutInputs> cutout = {},
    std::optional<NativeRigidDeferredInputs> deferred = {},
    std::optional<NativeSceneLightRecipe> light_recipe = {}) {
  const auto refuse = [&](const char *reason) -> std::optional<NativeRigidScenePlan> {
    if (refusal) *refusal = reason;
    return {};
  };
  // Object colour always modulates the result. Ordinary specular is conditional;
  // Toon always consumes it. Unused reflection values may still be present.
  // No exact-mask restriction.
  const bool toon = packet.surface == NativeSceneSurface::Toon;
  if ((packet.surface != NativeSceneSurface::Ordinary && !toon) || toon != packet.toon.has_value())
    return refuse("native surface family or Toon inputs unavailable");
  const uint32_t required_material = 1u | (toon || (packet.features && packet.features->specular) ? 2u : 0u);
  if ((packet.material_mask & required_material) != required_material)
    return refuse("active diffuse/specular material values unavailable");
  if (!program.valid || program.ranges.empty() || program.ranges.size() > 4096 ||
      program.ranges.size() != program.geometries.size() || program.materials.size() != program.ranges.size() ||
      packet.primitive >= program.ranges.size() ||
      !packet.geometry || packet.geometry != program.geometries[packet.primitive] ||
      !packet.material || packet.material != program.materials[packet.primitive] ||
      !packet.shader.vertex_bones || *packet.shader.vertex_bones > 3 ||
      packet.shader.texture_layers > 3 || !packet.shader.vertex_colour ||
      !packet.features || packet.features->reflection || packet.features->normal_mapping ||
      (!packet.lights && !light_recipe) || (light_recipe && !light_recipe->update) ||
      !packet.fog || !packet.camera || !packet.lighting ||
      !packet.policy.routing_known || (packet.policy.deferred && (!deferred || !packet.policy.alpha_test)) ||
      !packet.textures.owns_uv ||
      packet.receiver_shadow == NativeShadowPolicy::Unknown) return refuse("native primitive owners or shader features unavailable");
  if (packet.policy.alpha_test && (!cutout || !cutout->blend.alphaBlendEnable))
    return refuse("owned cutout reference, comparison or enabled blend factors unavailable");
  const bool skinned = *packet.shader.vertex_bones != 0;
  const auto &range = program.ranges[packet.primitive];
  if (bool(range.skin) != skinned || range.shader.vertex_bones != packet.shader.vertex_bones)
    return refuse("skin binding or influence identity mismatch");
  if (skinned && (!range.skin->count || !packet.pose || FindNativeInstanceNode(*packet.pose,packet.node) != &program ||
      program.skin_geometries.size() != program.ranges.size() || !program.skin_geometries[packet.primitive]))
    return refuse("owned skin geometry or exact pose unavailable");
  const auto &geometry = skinned ? program.skin_geometries[packet.primitive] : packet.geometry;
  const uint32_t layers = packet.shader.texture_layers;
  const auto vertex_input = skinned
      ? (layers == 3 ? geometry->skin_scene_layered_vertex_input : geometry->skin_scene_vertex_input)
      : (layers == 3 ? geometry->layered_rigid_vertex_input : geometry->rigid_vertex_input);
  const auto skin_bounds = skinned ? TransformNativeSkinBounds(geometry->skin_bounds,packet.pose->transforms) : std::nullopt;
  if (skinned && (geometry->skin_influences != *packet.shader.vertex_bones || !skin_bounds))
    return refuse("skin influence layout or animated bounds unavailable");
  if (!geometry->canonical_vertices || !vertex_input || geometry->stream_mask != 1 ||
      !geometry->streams[0].buffer.ref || !geometry->index.buffer.ref || !geometry->count ||
      geometry->count % 3 || !geometry->strides[0] || geometry->strides[0] > 255)
    return refuse("native rigid vertex layout or geometry unavailable");
  std::array<NativeTextureGpuHandle, 3> albedo;
  std::array<NativeMaterialSampler2D, 3> samplers{};
  for (uint32_t n = 0; n < layers; ++n) {
    const auto &binding = packet.textures.images[n];
    const auto &image = binding.primary;
    if (!(packet.textures.image_mask & (1u << n)) || !image || !image->image || !image->view ||
        binding.slice_2d || binding.cube || image->dimension != plume::RenderTextureViewDimension::TEXTURE_2D_ARRAY ||
        !packet.samplers[n] || packet.samplers[n]->u == MaterialSampleAddress::Unknown ||
        packet.samplers[n]->v == MaterialSampleAddress::Unknown)
      return refuse("active native texture array or sampler unavailable");
    albedo[n] = image; samplers[n] = *packet.samplers[n];
  }
  const auto &shadow = receiver.image;
  if (!shadow || !shadow->Sampled() || !shadow->view || !shadow->shape.width || !shadow->shape.height || shadow->shape.layers != 1 ||
      shadow->shape.samples != 1 || shadow->shape.format != plume::RenderFormat::D32_FLOAT_S8_UINT ||
      shadow->layout != plume::RenderTextureLayout::SHADER_READ) return refuse("native primary depth image unavailable");
  const auto vector = [](const LightingVector &v) { return RigidFloat4{v[0],v[1],v[2],v[3]}; };
  const bool receives = ReceivesNativeShadow(receiver.visibility,
      packet.receiver_shadow == NativeShadowPolicy::Disabled);
  const uint32_t flags = (layers ? RigidAlbedo : 0) | (*packet.shader.vertex_colour ? RigidVertexColour : 0) |
      (packet.features->diffuse ? RigidDiffuse : 0) | (packet.features->specular ? RigidSpecular : 0) |
      (packet.features->fog ? RigidFogEnabled : 0) | (receives ? RigidReceiveShadow : 0);
  // The ordinary VS uses (asset UV + 1)/512 + live object offset.
  // Large UV values are authored wrapping coordinates, not an origin to remove.
  const auto &uv = packet.textures.uv;
  const auto &uv2 = packet.textures.secondary_uv;
  const auto offset = [](float u, float v) { return RigidFloat4{1.f/512,1.f/512,1.f/512+u,1.f/512+v}; };
  // Skin matrices already map joint-local vertices to world; no second object
  // transform or unused inverse-transpose requirement survives this boundary.
  const RenderMatrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
  auto specular = toon || packet.features->specular ? vector(packet.material_values[1]) : RigidFloat4{};
  if (toon && std::isfinite(specular.w)) specular.w = (std::max)(10.f,specular.w);
  auto object = BuildRigidObject(skinned ? identity : packet.world, vector(packet.material_values[0]),
      specular, offset(uv[0],uv[1]), flags,
      {offset(uv[2],uv[3]), offset(uv2[0],uv2[1])}, layers ? layers-1 : 0);
  if (toon && (!object || !SetRigidToonSurface(*object,*packet.toon)))
    return refuse("nonfinite native Toon material inputs");
  NativeRigidPassInputs inputs;
  // Initial acceptance is a mono scene; the backend refuses a layered target.
  // Per-eye cameras must be explicitly produced before enabling this in XR.
  inputs.world_to_clip = {packet.camera->world_to_clip, packet.camera->world_to_clip};
  inputs.world_to_shadow = receiver.world_to_shadow;
  const auto &lighting = packet.lighting->inputs;
  inputs.cameras = {vector(lighting.camera_position), vector(lighting.camera_position)};
  inputs.ambient = vector(lighting.ambient); inputs.colour_grade = vector(lighting.color_scale);
  inputs.shadow_colour_strength = vector(receiver.colour);
  // Native four-tap filter: retain authored bias; kernel radius is UV, not the
  // old width*scale metadata. Slope strength is an explicit native filter policy.
  inputs.shadow_filter = {lighting.shadow_bias, .4f*lighting.shadow_bias,
      .65f/float(shadow->shape.width), 0};
  // A pending recipe deliberately has no draw-ready light values. Even an
  // explicit Bind is packed only by the final consumer, using a current ticket.
  inputs.lights = light_recipe ? NativeSelectedLights{} : *packet.lights;
  inputs.fog = *packet.fog;
  const auto pass = BuildRigidPass(inputs);
  if (!object || !pass) return refuse("nonfinite native object/pass GPU inputs");
  if (packet.policy.alpha_test && !SetRigidCutout(*object, cutout->reference,
      packet.policy.deferred ? RigidCutoutGE : cutout->comparison))
    return refuse("unsupported native cutout comparison");
  NativeRigidScenePlan plan{geometry, vertex_input, albedo, shadow, samplers, *object, *pass,
      packet.policy.cull, packet.policy.direct};
  plan.primitive = packet.primitive;
  if (skinned) { plan.skin_pose = packet.pose; plan.skin_bounds = skin_bounds; }
  plan.light_recipe = std::move(light_recipe);
  if (packet.policy.alpha_test) {
    plan.blend = cutout->blend;
    plan.alpha_to_coverage = cutout->alpha_to_coverage;
  }
  if (packet.policy.deferred) {
    // The ordinary sorted pass establishes GE, independent of the preceding
    // direct draw's comparison. Its saved shadow participation is also its
    // depth-write byte; world bounds supply the original far-extent sort key.
    DeferredDepthRecipe recipe;
    if (deferred->fixed && packet.policy.shadow_allowed) {
      recipe.kind = DeferredDepthRecipe::Kind::Fixed;
      recipe.fixed_depth = deferred->fixed_depth;
    } else {
      if (!program.bounds) return refuse("owned deferred model bounds unavailable");
      recipe.centre = {(*program.bounds)[0],(*program.bounds)[1],(*program.bounds)[2]};
      recipe.radius = (*program.bounds)[3];
    }
    const auto depth = EvaluateDeferredDepth(recipe,packet.world,packet.camera->view);
    if (!depth) return refuse("native deferred depth unavailable");
    plan.deferred = plan.draw = true;
    plan.depth_write = packet.policy.shadow_allowed;
    plan.depth = *depth;
  }
  return plan;
}
} // namespace bd::gpu::scene
