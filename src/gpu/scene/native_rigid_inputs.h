/**
 * @brief Explicit C++/GPU layout for the native opaque rigid shader family.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#ifdef __cplusplus
#include "gpu/scene/native_transform.h"
#include "gpu/scene/native_lit_shading.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>
namespace bd::gpu::scene {
using RigidUInt = uint32_t;
struct alignas(16) RigidFloat4 { float x, y, z, w; };
struct alignas(16) RigidUint4 { uint32_t x, y, z, w; };
#else
#define RigidFloat4 float4
#define RigidUint4 uint4
#define RigidUInt uint
#endif

// Row-vector matrices expressed as four explicit 16-byte rows, avoiding implicit
// compiler matrix packing. No bool, pointer, descriptor index or source address.
struct RigidMatrix { RigidFloat4 rows[4]; };
struct RigidLightGPU {
  RigidFloat4 position_range; // xyz position, w inverse range
  RigidFloat4 direction_cone; // xyz direction, w cone cosine
  RigidFloat4 colour_strength; // rgb, w cone strength
  RigidUint4 kind; // x: LitDisabled/Directional/Spot/Point; yzw reserved zero
};
struct RigidFogGPU {
  RigidFloat4 origin_start, direction_end, colour_opacity;
  RigidUint4 mode; // disabled, radial, blend, reserved zero
};
static const RigidUInt RigidAlbedo = 1, RigidVertexColour = 2, RigidDiffuse = 4,
                  RigidSpecular = 8, RigidReceiveShadow = 16, RigidFogEnabled = 32;
struct NativeRigidObjectGPU {
  RigidMatrix world;
  RigidFloat4 normal_rows[3]; // inverse transpose, row-vector convention
  RigidFloat4 diffuse, specular; // diffuse RGBA; specular RGB/shininess
  RigidFloat4 uv_scale_offset; // named UV scale.xy + offset.zw
  RigidUint4 flags; // x flags above; yzw reserved zero
};
struct NativeRigidPassGPU {
  RigidMatrix world_to_clip[2], world_to_shadow;
  RigidFloat4 cameras[2], ambient, colour_grade, shadow_colour_strength;
  RigidFloat4 shadow_filter; // depth bias, slope bias, UV kernel radius, reserved
  RigidLightGPU lights[3];
  RigidFogGPU fog[2];
};
// An instance owns its lighting/fog too: different selected lights must not
// accidentally become a batch-wide value. No translated constant-record ABI.
struct NativeRigidInstanceGPU {
  NativeRigidObjectGPU object_data;
  NativeRigidPassGPU pass_data;
};

#ifdef __cplusplus
static_assert(sizeof(RigidLightGPU) == 64 && sizeof(RigidFogGPU) == 64);
static_assert(sizeof(NativeRigidObjectGPU) == 176 && alignof(NativeRigidObjectGPU) == 16);
static_assert(offsetof(NativeRigidObjectGPU, normal_rows) == 64);
static_assert(offsetof(NativeRigidObjectGPU, diffuse) == 112);
static_assert(offsetof(NativeRigidObjectGPU, flags) == 160);
static_assert(sizeof(NativeRigidPassGPU) == 608 && alignof(NativeRigidPassGPU) == 16);
static_assert(offsetof(NativeRigidPassGPU, cameras) == 192);
static_assert(offsetof(NativeRigidPassGPU, lights) == 288);
static_assert(offsetof(NativeRigidPassGPU, fog) == 480);
static_assert(sizeof(NativeRigidInstanceGPU) == 784 && alignof(NativeRigidInstanceGPU) == 16);
static_assert(offsetof(NativeRigidInstanceGPU, pass_data) == 176);
static_assert(std::is_trivially_copyable_v<NativeRigidInstanceGPU>);
static_assert(std::is_trivially_copyable_v<NativeRigidObjectGPU> &&
              std::is_trivially_copyable_v<NativeRigidPassGPU>);

inline RigidMatrix PackRigidMatrix(const RenderMatrix &matrix) {
  RigidMatrix result{};
  for (uint32_t row = 0; row < 4; ++row)
    result.rows[row] = {matrix[row*4], matrix[row*4+1], matrix[row*4+2], matrix[row*4+3]};
  return result;
}
inline bool RigidFinite(RigidFloat4 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}
inline bool RigidFinite(const RigidMatrix &matrix) {
  for (const auto &row : matrix.rows) if (!RigidFinite(row)) return false;
  return true;
}

// Source-free object producer. Normal matrices handle nonuniform scale; singular
// or non-affine worlds are not silently accepted as rigid objects.
inline std::optional<NativeRigidObjectGPU> BuildRigidObject(const RenderMatrix &world,
    RigidFloat4 diffuse, RigidFloat4 specular, RigidFloat4 uv, uint32_t flags) {
  NativeRigidObjectGPU result{};
  result.world = PackRigidMatrix(world);
  if (!RigidFinite(result.world) || !RigidFinite(diffuse) || !RigidFinite(specular) ||
      !RigidFinite(uv) || specular.w < 0 || (flags & ~63u) ||
      world[3] != 0 || world[7] != 0 || world[11] != 0 || world[15] != 1) return {};
  const float a = world[0], b = world[1], c = world[2];
  const float d = world[4], e = world[5], f = world[6];
  const float g = world[8], h = world[9], i = world[10];
  const float determinant = a*(e*i-f*h) - b*(d*i-f*g) + c*(d*h-e*g);
  if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12f) return {};
  const float inverse = 1 / determinant;
  result.normal_rows[0] = {(e*i-f*h)*inverse, (f*g-d*i)*inverse, (d*h-e*g)*inverse, 0};
  result.normal_rows[1] = {(c*h-b*i)*inverse, (a*i-c*g)*inverse, (b*g-a*h)*inverse, 0};
  result.normal_rows[2] = {(b*f-c*e)*inverse, (c*d-a*f)*inverse, (a*e-b*d)*inverse, 0};
  for (const auto &row : result.normal_rows) if (!RigidFinite(row)) return {};
  result.diffuse = diffuse; result.specular = specular;
  result.uv_scale_offset = uv; result.flags.x = flags;
  return result;
}

struct NativeRigidPassInputs {
  std::array<RenderMatrix, 2> world_to_clip{};
  RenderMatrix world_to_shadow{};
  std::array<RigidFloat4, 2> cameras{};
  RigidFloat4 ambient{}, colour_grade{}, shadow_colour_strength{}, shadow_filter{};
  std::array<LitLight, 3> lights{};
  std::array<LitFog, 2> fog{};
};
// Scalar light/fog semantics are explicitly packed, never memcpy'd as a GPU ABI.
inline std::optional<NativeRigidPassGPU> BuildRigidPass(const NativeRigidPassInputs &input) {
  NativeRigidPassGPU result{};
  for (uint32_t eye = 0; eye < 2; ++eye) {
    result.world_to_clip[eye] = PackRigidMatrix(input.world_to_clip[eye]);
    result.cameras[eye] = input.cameras[eye];
    if (!RigidFinite(result.world_to_clip[eye]) || !RigidFinite(result.cameras[eye])) return {};
  }
  result.world_to_shadow = PackRigidMatrix(input.world_to_shadow);
  result.ambient = input.ambient; result.colour_grade = input.colour_grade;
  result.shadow_colour_strength = input.shadow_colour_strength; result.shadow_filter = input.shadow_filter;
  if (!RigidFinite(result.world_to_shadow) || !RigidFinite(result.ambient) ||
      !RigidFinite(result.colour_grade) || !RigidFinite(result.shadow_colour_strength) ||
      !RigidFinite(result.shadow_filter) || result.shadow_filter.z < 0 || result.shadow_filter.w != 0) return {};
  for (uint32_t n = 0; n < 3; ++n) {
    const auto &light = input.lights[n];
    auto &packed = result.lights[n];
    packed.position_range = {light.position.x, light.position.y, light.position.z, light.inverse_range};
    packed.direction_cone = {light.direction.x, light.direction.y, light.direction.z, light.cone_cosine};
    packed.colour_strength = {light.colour.x, light.colour.y, light.colour.z, light.cone_strength};
    packed.kind.x = uint32_t(light.kind);
    if (!RigidFinite(packed.position_range) || !RigidFinite(packed.direction_cone) ||
        !RigidFinite(packed.colour_strength) || light.kind < LitDisabled || light.kind > LitPoint ||
        light.inverse_range < 0 || (light.kind == LitSpot && (light.cone_cosine >= 1 || light.cone_cosine < -1))) return {};
  }
  for (uint32_t n = 0; n < 2; ++n) {
    const auto &fog = input.fog[n]; auto &packed = result.fog[n];
    packed.origin_start = {fog.origin.x, fog.origin.y, fog.origin.z, fog.start};
    packed.direction_end = {fog.direction.x, fog.direction.y, fog.direction.z, fog.end};
    packed.colour_opacity = {fog.colour.x, fog.colour.y, fog.colour.z, fog.opacity};
    packed.mode = {uint32_t(fog.disabled), uint32_t(fog.radial), uint32_t(fog.blend), 0};
    if (!RigidFinite(packed.origin_start) || !RigidFinite(packed.direction_end) ||
        !RigidFinite(packed.colour_opacity) || (!fog.disabled && fog.end == fog.start) ||
        fog.blend < LitFogBlend || fog.blend > LitFogSubtract) return {};
  }
  return result;
}
} // namespace bd::gpu::scene
#endif
