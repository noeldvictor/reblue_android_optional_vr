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
};
// Transitional object producer: resolves source bindings before returning the
// retained, address-free plan. It never interprets/captures/replays the node.
std::optional<std::vector<NativeRigidScenePlan>> PrepareNativeRigidSceneForObject(
    const NativeInstancePose &pose, uint32_t node, const char *&refusal);
bool CommitNativeRigidSceneLights(std::span<const NativeRigidScenePlan> plans);
// Share rigid participation rules with casting, then classify the scene shader
// from authored recipes, before a missing pose/texture can choose legacy drawing.
inline NativeRigidCasterAdmission PrepareNativeRigidSceneAdmission(
    const NativeModelMaterialProgram &program,
    const std::optional<PrimitivePolicyInputs> &inputs) {
  auto admission = PrepareNativeRigidCasterAdmission(program, inputs, true);
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
    std::optional<NativeRigidCutoutInputs> cutout = {}) {
  const auto refuse = [&](const char *reason) -> std::optional<NativeRigidScenePlan> {
    if (refusal) *refusal = reason;
    return {};
  };
  // Object colour always modulates the result, but specular values are consumed
  // only when the owned material/pass feature enables them. Reflection values
  // may be present without being used by this shader. No exact-mask restriction.
  const uint32_t required_material = 1u | (packet.features && packet.features->specular ? 2u : 0u);
  if ((packet.material_mask & required_material) != required_material)
    return refuse("active diffuse/specular material values unavailable");
  if (!program.valid || program.ranges.empty() || program.ranges.size() > 4096 ||
      program.ranges.size() != program.geometries.size() || program.materials.size() != program.ranges.size() ||
      packet.primitive >= program.ranges.size() ||
      !packet.geometry || packet.geometry != program.geometries[packet.primitive] ||
      !packet.material || packet.material != program.materials[packet.primitive] || program.ranges[packet.primitive].skin ||
      !packet.shader.vertex_bones || *packet.shader.vertex_bones ||
      packet.shader.texture_layers > 3 || !packet.shader.vertex_colour ||
      !packet.features || packet.features->reflection || packet.features->normal_mapping ||
      !packet.lights || !packet.fog || !packet.camera || !packet.lighting ||
      !packet.policy.routing_known || packet.policy.deferred ||
      !packet.textures.owns_uv ||
      packet.receiver_shadow == NativeShadowPolicy::Unknown) return refuse("native primitive owners or shader features unavailable");
  if (packet.policy.alpha_test && (!cutout || !cutout->blend.alphaBlendEnable))
    return refuse("owned cutout reference, comparison or enabled blend factors unavailable");
  const auto &geometry = packet.geometry;
  const uint32_t layers = packet.shader.texture_layers;
  const auto vertex_input = layers == 3 ? geometry->layered_rigid_vertex_input : geometry->rigid_vertex_input;
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
  auto object = BuildRigidObject(packet.world, vector(packet.material_values[0]),
      packet.features->specular ? vector(packet.material_values[1]) : RigidFloat4{}, offset(uv[0],uv[1]), flags,
      {offset(uv[2],uv[3]), offset(uv2[0],uv2[1])}, layers ? layers-1 : 0);
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
  inputs.lights = *packet.lights; inputs.fog = *packet.fog;
  const auto pass = BuildRigidPass(inputs);
  if (!object || !pass) return refuse("nonfinite native object/pass GPU inputs");
  if (packet.policy.alpha_test && !SetRigidCutout(*object, cutout->reference, cutout->comparison))
    return refuse("unsupported native cutout comparison");
  NativeRigidScenePlan plan{geometry, vertex_input, albedo, shadow, samplers, *object, *pass,
      packet.policy.cull, packet.policy.direct};
  if (packet.policy.alpha_test) {
    plan.blend = cutout->blend;
    plan.alpha_to_coverage = cutout->alpha_to_coverage;
  }
  return plan;
}
} // namespace bd::gpu::scene
