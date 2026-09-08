/**
 * @brief Owned water material and explicit native GPU interface, without source identities.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#ifdef __cplusplus
#include "gpu/scene/native_rigid_inputs.h"
namespace bd::gpu::scene {
#else
#include "src/gpu/scene/native_rigid_inputs.h"
#endif

static const RigidUInt WaterRefraction = 1, WaterShore = 2, WaterDiffuse = 4,
    WaterFog = 8, WaterShadow = 16, WaterCel = 32;
static const RigidUInt WaterReflectionNone = 0, WaterReflectionPlanar = 1,
    WaterReflectionEnvironment = 2;
struct NativeWaterMaterialGPU {
  RigidFloat4 tint;
  RigidFloat4 scroll; // velocity.xy, UV scale, animation phase
  RigidFloat4 waves; // displacement amplitude, speed, direction radians, frequency
  RigidFloat4 surface; // reflection distortion, refraction distortion, distance fade, normal blend
  RigidFloat4 highlight; // exponent, intensity, shoreline brightness, shoreline gain
  RigidFloat4 shore; // depth-to-opacity scale; yzw reserved
  RigidUint4 modes; // reflection choice, feature flags; zw reserved
};
struct NativeWaterObjectGPU {
  RigidMatrix world;
  RigidFloat4 normal_rows[3];
  NativeWaterMaterialGPU material;
};
struct NativeWaterInstanceGPU {
  NativeWaterObjectGPU object_data;
  NativeRigidPassGPU pass_data; // the existing semantic lights/fog and per-eye cameras
  RigidMatrix world_to_bottom[2];
  RigidUint4 image_layers; // planar, snapshot, bottom: explicitly mono or per-eye; w reserved
};

#ifdef __cplusplus
// These fields have meaning independently of shader registers. Temporary import
// descriptors never escape into a native material, program or queued instance.
struct NativeWaterMaterial {
  RigidFloat4 tint{};
  float scroll_u = 0, scroll_v = 0, uv_scale = 0, phase = 0;
  float wave_amplitude = 0, wave_speed = 0, wave_direction = 0, wave_frequency = 0;
  float reflection_distortion = 0, refraction_distortion = 0, distance_fade = 0, normal_blend = 0;
  float shininess = 0, highlight = 0, shore_brightness = 0, shore_gain = 0, depth_opacity = 0;
  uint32_t reflection = WaterReflectionNone;
  bool refraction = false, shore = false;
};
static_assert(sizeof(NativeWaterMaterialGPU) == 112);
static_assert(sizeof(NativeWaterObjectGPU) == 224);
static_assert(offsetof(NativeWaterInstanceGPU, pass_data) == 224);
static_assert(offsetof(NativeWaterInstanceGPU, world_to_bottom) == 832);
static_assert(offsetof(NativeWaterInstanceGPU, image_layers) == 960);
static_assert(sizeof(NativeWaterInstanceGPU) == 976 && alignof(NativeWaterInstanceGPU) == 16);
static_assert(std::is_trivially_copyable_v<NativeWaterInstanceGPU>);

inline std::optional<NativeWaterInstanceGPU> BuildNativeWaterInstance(
    const RenderMatrix &world, const NativeWaterMaterial &material,
    const NativeRigidPassInputs &pass, const std::array<RenderMatrix, 2> &world_to_bottom,
    std::array<uint32_t, 3> image_layers, uint32_t features) {
  // Reuse affine validation/normal transforms and the real lighting/fog packer.
  const auto object = BuildRigidObject(world, {}, {}, {}, 0);
  const auto lighting = BuildRigidPass(pass);
  if (!object || !lighting || material.reflection > WaterReflectionEnvironment ||
      material.shininess < 0 || (features & ~(WaterDiffuse | WaterFog | WaterShadow | WaterCel))) return {};
  for (auto layers : image_layers) if (layers != 1 && layers != 2) return {};
  NativeWaterInstanceGPU result{};
  result.object_data.world = object->world;
  std::copy(std::begin(object->normal_rows), std::end(object->normal_rows), result.object_data.normal_rows);
  auto &m = result.object_data.material;
  m.tint = material.tint;
  m.scroll = {material.scroll_u, material.scroll_v, material.uv_scale, material.phase};
  m.waves = {material.wave_amplitude, material.wave_speed, material.wave_direction, material.wave_frequency};
  m.surface = {material.reflection_distortion, material.refraction_distortion, material.distance_fade, material.normal_blend};
  m.highlight = {material.shininess, material.highlight, material.shore_brightness, material.shore_gain};
  m.shore = {material.depth_opacity, 0, 0, 0};
  if (!RigidFinite(m.tint) || !RigidFinite(m.scroll) || !RigidFinite(m.waves) ||
      !RigidFinite(m.surface) || !RigidFinite(m.highlight) || !RigidFinite(m.shore)) return {};
  m.modes = {material.reflection, features | (material.refraction ? WaterRefraction : 0) |
      (material.shore ? WaterShore : 0), 0, 0};
  result.pass_data = *lighting;
  for (uint32_t eye = 0; eye < 2; ++eye) {
    result.world_to_bottom[eye] = PackRigidMatrix(world_to_bottom[eye]);
    if (!RigidFinite(result.world_to_bottom[eye])) return {};
  }
  result.image_layers = {image_layers[0], image_layers[1], image_layers[2], 0};
  return result;
}
} // namespace bd::gpu::scene
#endif
