/**
 * @brief Owned object/primitive inputs selected without a replay or source key.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"

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
  MaterialTextureValues<Image> textures;
  NativePrimitivePolicy policy;
  NativeShadowPolicy receiver_shadow = NativeShadowPolicy::Unknown;
  std::array<float, 4> material_values[3]{};
  uint32_t material_mask = 0;
};

template <class Image>
std::optional<NativeObjectPrimitive<Image>> BuildNativeObjectPrimitive(
    std::shared_ptr<const NativeInstancePose> pose, uint32_t node, uint32_t primitive,
    const NativeMaterialObjectInputs &object, const MaterialTextureValues<Image> &textures,
    const NativePrimitivePolicy &policy) {
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
  result.textures = textures; result.policy = policy;
  result.receiver_shadow = program->shadow_policies[primitive];
  result.material_mask = ComposeNativeMaterialAsset(result.material->asset, object.colour,
      object.writes_shininess, result.material_values);
  return result;
}
} // namespace bd::gpu::scene
