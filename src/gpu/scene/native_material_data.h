/**
 * @file    gpu/scene/native_material_data.h
 * @brief   Asset-level material properties decoded from model commands.
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skin.h"
#include "gpu/scene/native_material_textures.h"
#include "gpu/scene/native_primitive_policy.h"
#include "gpu/scene/native_lighting.h"
#include "gpu/scene/native_material_sampler.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace bd::gpu::scene {

struct NativeMaterialObjectInputs {
  std::array<float, 4> colour{};
  bool writes_shininess = false;
  bool diffuse_enabled = true;
  bool operator==(const NativeMaterialObjectInputs &) const = default;
};

enum class MaterialDiffuseMode : uint8_t { Object, Disabled, Enabled, Unknown };
struct NativeMaterialControl {
  bool applies = false;
  bool disable_diffuse = false, disable_specular = false, disable_shadow = false;
};
// Folded in command order at load, including repeated-command elision and
// control-table null semantics. Live object/pass gates remain separate.
struct NativeMaterialFeatureRecipe {
  MaterialDiffuseMode diffuse = MaterialDiffuseMode::Object;
  std::optional<bool> specular_requested = false;
  bool normal_mapping_requested = false;
  bool operator==(const NativeMaterialFeatureRecipe &) const = default;
};
struct NativeMaterialFeatures {
  bool diffuse = false, specular = false, normal_mapping = false;
  bool reflection = false, fog = false;
  bool operator==(const NativeMaterialFeatures &) const = default;
};
inline std::optional<NativeMaterialFeatures> ComposeNativeMaterialFeatures(
    const NativeMaterialFeatureRecipe &recipe, const NativeMaterialObjectInputs &object,
    const NativeLightingInputs &pass, bool reflection) {
  if (recipe.diffuse == MaterialDiffuseMode::Unknown || !recipe.specular_requested) return {};
  return NativeMaterialFeatures{
      recipe.diffuse == MaterialDiffuseMode::Object ? object.diffuse_enabled : recipe.diffuse == MaterialDiffuseMode::Enabled,
      *recipe.specular_requested && object.writes_shininess && pass.specular_enabled != 0,
      recipe.normal_mapping_requested && pass.normal_mapping != 0, reflection, pass.fog_enabled != 0};
}

// Named asset properties, not a captured shader register file. Unknown fields
// stay unknown: an omitted command inherits state and is not a white default.
struct NativeMaterialProperties {
  std::array<float, 3> diffuse_multiplier{};
  std::array<float, 3> specular_colour{};
  std::array<float, 4> reflection_colour{};
  bool modulate_diffuse = false;
  uint8_t shininess = 0;
  bool has_diffuse_multiplier = false;
  bool has_specular_colour = false;
  bool has_reflection_colour = false;
  bool has_shininess = false;
  bool operator==(const NativeMaterialProperties &) const = default;
};

// Selection is separate from enable: disabling reflection leaves the last
// image bound. Table indices are model import recipes, not persistent asset IDs.
enum class ReflectionTextureSource : uint8_t { PassDefault, Table, Unknown };
struct NativeReflectionRecipe {
  ReflectionTextureSource source = ReflectionTextureSource::PassDefault;
  uint8_t table_index = 0;
  bool enabled = false;
  bool operator==(const NativeReflectionRecipe &) const = default;
};

// Ordinary scene-family inputs, not captured shader bools. Texture layers are
// initialized to one by the scene interpreter, then selected by material commands.
// Declaration-dependent values stay unknown until the load adapter resolves them;
// a COLOR attribute or an omitted bone command does not establish either value.
struct NativePrimitiveShaderInputs {
  uint8_t texture_layers = 1;
  std::optional<bool> vertex_colour;
  std::optional<uint8_t> vertex_bones;
  bool operator==(const NativePrimitiveShaderInputs &) const = default;
};

struct NativeMaterialRange {
  NativeMaterialProperties material;
  NativeReflectionRecipe reflection;
  NativePrimitiveShaderInputs shader;
  NativeMaterialFeatureRecipe features;
  std::array<NativeSamplerAddress, 5> sampler_addresses = MaterialSamplerEntry();
  // Unknown until a bone-index command; an explicit empty binding is unskinned.
  std::optional<NativeSkinBinding> skin;
  uint32_t index_count = 0;
  uint32_t first_index = 0;
  uint16_t index_record = 0xffff;
  uint16_t vertex_record = 0xffff;
  uint16_t stream = 0;
  // Import-only index into the model's control table, not a shader bool value.
  uint16_t control_record = 0xffff;
  uint32_t texture_assignment_end = 0;
  uint32_t policy_step_end = 0;
  PrimitiveWinding winding = PrimitiveWinding::Pass;
};

// Operand framing is shared by the bounded guest reader and offline decoder.
// -1 is an unsupported opcode. 0x00ff ends the stream, only at an opcode boundary.
int MeshCommandOperands(uint16_t command);

// Input words are host endian. The native material program here covers scene
// phase 0; the adapter must not apply it to phase 1's shader/colour overrides.
// Failure is transactional, including truncated operands and missing terminator.
bool DecodeMeshMaterials(std::span<const uint16_t> commands,
                         std::vector<NativeMaterialRange> &out,
                         std::vector<MaterialImageAssignment> *textures = nullptr,
                         std::vector<PrimitivePolicyStep> *policies = nullptr,
                         const std::function<std::optional<NativeMaterialControl>(uint16_t)> &control = {});

// Compose only fully known values; no staging globals or sibling draw state.
// Specular power is written by the game only when the visual permits it.
uint32_t ComposeNativeMaterial(const NativeMaterialProperties &material,
                               const std::array<float, 4> &object_colour,
                               bool writes_shininess,
                               std::array<float, 4> &diffuse,
                               std::array<float, 4> &specular,
                               std::array<float, 4> &reflection);
constexpr uint32_t kNativeDiffuse = 1;
constexpr uint32_t kNativeSpecular = 2;
constexpr uint32_t kNativeReflection = 4;

} // namespace bd::gpu::scene
