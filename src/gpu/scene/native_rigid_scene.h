/**
 * @brief Whole-node native opaque scene admission and retained draw inputs.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_rigid_shadow.h"
#include "gpu/scene/native_object_primitive.h"
#include "gpu/scene/native_texture_binding.h"
#include "gpu/scene/native_shadow.h"
#include "gpu/native_target_images.h"

namespace bd::gpu::scene {
struct NativeRigidReceiver {
  NativeTargetImageHandle image;
  RenderMatrix world_to_shadow{};
  LightingVector colour{};
  NativeShadowInputs visibility;
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
};
// Transitional object producer: resolves source bindings before returning the
// retained, address-free plan. It never interprets/captures/replays the node.
std::optional<std::vector<NativeRigidScenePlan>> PrepareNativeRigidSceneForObject(
    const NativeInstancePose &pose, uint32_t node, const char *&refusal);
// Share opaque participation rules with casting, then classify the scene shader
// from authored recipes, before a missing pose/texture can choose legacy drawing.
inline NativeRigidCasterAdmission PrepareNativeRigidSceneAdmission(
    const NativeModelMaterialProgram &program,
    const std::optional<PrimitivePolicyInputs> &inputs) {
  auto admission = PrepareNativeRigidCasterAdmission(program, inputs);
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
    const NativeRigidReceiver &receiver) {
  if (!program.valid || program.ranges.empty() || program.ranges.size() > 4096 ||
      program.ranges.size() != program.geometries.size() || program.materials.size() != program.ranges.size() ||
      packet.primitive >= program.ranges.size() ||
      !packet.geometry || packet.geometry != program.geometries[packet.primitive] ||
      !packet.material || packet.material != program.materials[packet.primitive] || program.ranges[packet.primitive].skin ||
      !packet.shader.vertex_bones || *packet.shader.vertex_bones ||
      packet.shader.texture_layers > 3 || !packet.shader.vertex_colour ||
      !packet.features || packet.features->reflection || packet.features->normal_mapping ||
      !packet.lights || !packet.fog || !packet.camera || !packet.lighting ||
      !packet.policy.routing_known || packet.policy.deferred || packet.policy.alpha_test ||
      packet.material_mask != 3 || !packet.textures.owns_uv ||
      packet.receiver_shadow == NativeShadowPolicy::Unknown) return {};
  const auto &geometry = packet.geometry;
  const uint32_t layers = packet.shader.texture_layers;
  const auto vertex_input = layers == 3 ? geometry->layered_rigid_vertex_input : geometry->rigid_vertex_input;
  if (!geometry->canonical_vertices || !vertex_input || geometry->stream_mask != 1 ||
      !geometry->streams[0].buffer.ref || !geometry->index.buffer.ref || !geometry->count ||
      geometry->count % 3 || !geometry->strides[0] || geometry->strides[0] > 255) return {};
  std::array<NativeTextureGpuHandle, 3> albedo;
  std::array<NativeMaterialSampler2D, 3> samplers{};
  for (uint32_t n = 0; n < layers; ++n) {
    const auto &binding = packet.textures.images[n];
    const auto &image = binding.primary;
    if (!(packet.textures.image_mask & (1u << n)) || !image || !image->image || !image->view ||
        binding.slice_2d || binding.cube || image->dimension != plume::RenderTextureViewDimension::TEXTURE_2D_ARRAY ||
        !packet.samplers[n] || packet.samplers[n]->u == MaterialSampleAddress::Unknown ||
        packet.samplers[n]->v == MaterialSampleAddress::Unknown) return {};
    albedo[n] = image; samplers[n] = *packet.samplers[n];
  }
  const auto &shadow = receiver.image;
  if (!shadow || !shadow->Sampled() || !shadow->view || !shadow->shape.width || !shadow->shape.height || shadow->shape.layers != 1 ||
      shadow->shape.samples != 1 || shadow->shape.format != plume::RenderFormat::D32_FLOAT_S8_UINT ||
      shadow->layout != plume::RenderTextureLayout::SHADER_READ) return {};
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
  const auto object = BuildRigidObject(packet.world, vector(packet.material_values[0]),
      vector(packet.material_values[1]), offset(uv[0],uv[1]), flags,
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
  if (!object || !pass) return {};
  return NativeRigidScenePlan{geometry, vertex_input, albedo, shadow, samplers, *object, *pass,
      packet.policy.cull, packet.policy.direct};
}
} // namespace bd::gpu::scene
