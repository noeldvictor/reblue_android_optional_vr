/**
 * @brief Owned object/primitive inputs selected without a replay or source key.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_selected_lights.h"
#include "gpu/scene/native_fog.h"
#include "gpu/scene/native_lighting.h"
#include "gpu/scene/native_toon_surface.h"

namespace bd::gpu::scene {
// Own every resource/value needed after the publication scope ends. Image
// slots/UV values still need material-family interpretation; this packet is not
// an assertion of shader eligibility or permission to drop unsupported siblings.
template <class Image> struct NativeObjectPrimitive {
  std::shared_ptr<const NativeInstancePose> pose;
  uint32_t node = 0, primitive = 0;
  RenderMatrix world{};
  std::shared_ptr<const NativeGeometry> geometry;
  NativeMaterialHandle material;
  NativePrimitiveShaderInputs shader;
  MaterialTextureValues<Image> textures;
  NativePrimitivePolicy policy;
  std::optional<NativeSelectedLights> lights;
  std::optional<NativeFogLayers> fog;
  std::optional<NativeLightingPass> lighting;
  std::optional<RenderCamera> camera;
  std::optional<NativeMaterialFeatures> features;
  NativeMaterialSamplers samplers;
  NativeShadowPolicy receiver_shadow = NativeShadowPolicy::Unknown;
  std::array<float, 4> material_values[3]{};
  uint32_t material_mask = 0;
  NativeSceneSurface surface = NativeSceneSurface::Ordinary;
  std::optional<NativeToonSurface> toon;
};

template <class Image>
std::optional<NativeObjectPrimitive<Image>> BuildNativeObjectPrimitive(
    std::shared_ptr<const NativeInstancePose> pose, uint32_t node, uint32_t primitive,
    const NativeMaterialObjectInputs &object, const MaterialTextureValues<Image> &textures,
    const NativePrimitivePolicy &policy, std::optional<NativeSelectedLights> lights = {},
    std::optional<NativeFogLayers> fog = {}, std::optional<NativeLightingPass> lighting = {},
    std::optional<NativeSamplerFilterPass> filters = {}, std::optional<RenderCamera> camera = {},
    std::optional<NativeToonSurface> toon = {}) {
  const auto *program = pose ? FindNativeInstanceNode(*pose, node) : nullptr;
  if (!program || !program->valid || primitive >= program->ranges.size() ||
      primitive >= program->geometries.size() || primitive >= program->materials.size() ||
      primitive >= program->shadow_policies.size() || !program->geometries[primitive] ||
      !program->materials[primitive]) return {};
  for (float value : object.colour) if (!std::isfinite(value)) return {};
  NativeObjectPrimitive<Image> result;
  result.pose = std::move(pose); result.node = node; result.primitive = primitive;
  result.world = result.pose->transforms[node];
  result.geometry = program->geometries[primitive]; result.material = program->materials[primitive];
  result.shader = program->ranges[primitive].shader;
  result.textures = textures; result.policy = policy;
  result.lights = std::move(lights);
  result.fog = std::move(fog);
  result.lighting = std::move(lighting);
  result.camera = std::move(camera);
  if (filters) result.samplers = ComposeMaterialSamplers(program->ranges[primitive].sampler_addresses, *filters);
  if (result.lighting)
    result.features = ComposeNativeMaterialFeatures(program->ranges[primitive].features,
        object, result.lighting->inputs, program->ranges[primitive].reflection.enabled);
  result.receiver_shadow = program->shadow_policies[primitive];
  result.material_mask = ComposeNativeMaterialAsset(result.material->asset, object.colour,
      object.writes_shininess, result.material_values);
  if (toon) {
    result.surface = NativeSceneSurface::Toon;
    result.toon = std::move(toon);
    result.toon->texture_colours = textures.colours;
    // Visual begin initializes the exponent to zero; when object power writes
    // are disabled it stays zero. Otherwise an omitted command can inherit a
    // preceding node's value and remains unknown. RGB ownership is independent
    // of the ordinary family's specular-enable bit, not a guessed black colour.
    const auto &material = result.material->asset.properties;
    if (material.has_specular_colour && (!object.writes_shininess || material.has_shininess)) {
      for (uint32_t c=0;c<3;++c) result.material_values[1][c] = material.specular_colour[c];
      result.material_values[1][3] = object.writes_shininess ? float(material.shininess) : 0.f;
      result.material_mask |= kNativeSpecular;
    }
  }
  return result;
}
} // namespace bd::gpu::scene
