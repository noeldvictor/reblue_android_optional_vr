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
  NativeTextureGpuHandle albedo;
  NativeTargetImageHandle shadow;
  NativeMaterialSampler2D sampler;
  NativeRigidObjectGPU object;
  NativeRigidPassGPU pass;
  PrimitiveCull cull;
  bool draw;
};
// Transitional object producer: resolves source bindings before returning the
// retained, address-free plan. It never interprets/captures/replays the node.
std::optional<NativeRigidScenePlan> PrepareNativeRigidSceneForObject(
    const NativeInstancePose &pose, uint32_t node, const char *&refusal);
inline std::optional<NativeRigidScenePlan> PrepareNativeRigidScene(
    const NativeModelMaterialProgram &program,
    const NativeObjectPrimitive<NativeTextureBinding> &packet,
    const NativeRigidReceiver &receiver) {
  if (!program.valid || program.ranges.size() != 1 || program.geometries.size() != 1 ||
      program.materials.size() != 1 || packet.primitive != 0 ||
      !packet.geometry || packet.geometry != program.geometries[0] ||
      !packet.material || packet.material != program.materials[0] ||
      packet.material->id != 0x63B8D67932573E51ull || program.ranges[0].skin ||
      !packet.shader.vertex_bones || *packet.shader.vertex_bones ||
      packet.shader.texture_layers != 1 || !packet.shader.vertex_colour ||
      !packet.features || packet.features->reflection || packet.features->normal_mapping ||
      !packet.lights || !packet.fog || !packet.camera || !packet.lighting ||
      !packet.policy.routing_known || packet.policy.deferred || packet.policy.alpha_test ||
      packet.material_mask != 3 || packet.textures.image_mask != 1 || !packet.textures.owns_uv ||
      packet.receiver_shadow == NativeShadowPolicy::Unknown || !packet.samplers[0]) return {};
  const auto &geometry = packet.geometry;
  if (!geometry->canonical_vertices || !geometry->rigid_vertex_input || geometry->stream_mask != 1 ||
      !geometry->streams[0].buffer.ref || !geometry->index.buffer.ref || !geometry->count ||
      geometry->count % 3 || !geometry->strides[0] || geometry->strides[0] > 255) return {};
  const auto &albedo = packet.textures.images[0].primary;
  const auto &shadow = receiver.image;
  if (!albedo || !albedo->image || !albedo->view ||
      albedo->dimension != plume::RenderTextureViewDimension::TEXTURE_2D_ARRAY ||
      !shadow || !shadow->Sampled() || !shadow->view || shadow->shape.layers != 1 ||
      shadow->shape.samples != 1 || shadow->shape.format != plume::RenderFormat::D32_FLOAT_S8_UINT ||
      shadow->layout != plume::RenderTextureLayout::SHADER_READ) return {};
  const auto &sampling = *packet.samplers[0];
  if (sampling.u == MaterialSampleAddress::Unknown || sampling.v == MaterialSampleAddress::Unknown) return {};
  const auto vector = [](const LightingVector &v) { return RigidFloat4{v[0],v[1],v[2],v[3]}; };
  const bool receives = ReceivesNativeShadow(receiver.visibility,
      packet.receiver_shadow == NativeShadowPolicy::Disabled);
  const uint32_t flags = RigidAlbedo | (*packet.shader.vertex_colour ? RigidVertexColour : 0) |
      (packet.features->diffuse ? RigidDiffuse : 0) | (packet.features->specular ? RigidSpecular : 0) |
      (packet.features->fog ? RigidFogEnabled : 0) | (receives ? RigidReceiveShadow : 0);
  // The selected ordinary VS uses (asset UV + 1)/512 + live object offset.
  // Large UV values are authored wrapping coordinates, not an origin to remove.
  const auto &uv = packet.textures.uv;
  const auto object = BuildRigidObject(packet.world, vector(packet.material_values[0]),
      vector(packet.material_values[1]), {1.f/512,1.f/512,1.f/512+uv[0],1.f/512+uv[1]}, flags);
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
  return NativeRigidScenePlan{geometry, albedo, shadow, sampling, *object, *pass,
      packet.policy.cull, packet.policy.direct};
}
} // namespace bd::gpu::scene
