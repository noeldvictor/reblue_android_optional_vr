/**
 * @file    lighting_shader_bridge.h
 * @brief   Temporary engine staging ABI; not the native lighting contract.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lighting.h"
#include "gpu/scene/native_material_data.h"
#include <bit>

namespace bd::gpu::scene {
struct PrimitiveShaderBits { uint32_t vertex, pixel; };
inline std::optional<PrimitiveShaderBits> PackPrimitiveShaderBits(const NativePrimitiveShaderInputs &inputs) {
  if (inputs.texture_layers > 3 || !inputs.vertex_colour) return {};
  return PrimitiveShaderBits{*inputs.vertex_colour ? 16u : 0u, (1u << inputs.texture_layers) - 1};
}
inline void ApplyPrimitiveShaderBits(const PrimitiveShaderBits &bits, uint32_t &vertex, uint32_t &pixel) {
  vertex = (vertex & ~16u) | bits.vertex;
  pixel = (pixel & ~7u) | bits.pixel;
}
using LightingStagingImage = std::array<uint32_t, 103>;
inline std::array<LightingVector, 3> LightingPixelInputs(const NativeLightingPass &pass) {
  return {pass.inputs.ambient, pass.inputs.camera_position, pass.inputs.color_scale};
}
inline LightingStagingImage PackLightingStaging(const NativeLightingPass &pass) {
  LightingStagingImage result{};
  const auto &inputs = pass.inputs;
  auto vector = [&](size_t offset, const LightingVector &value) {
    for (size_t i = 0; i < 4; ++i)
      result[offset / 4 + i] = std::bit_cast<uint32_t>(value[i]);
  };
  vector(0, inputs.ambient);
  vector(16, inputs.camera_position);
  vector(80, inputs.ambient);
  vector(96, inputs.camera_position);
  vector(112, inputs.color_scale);
  vector(224, pass.shadow_sampling);
  vector(288, pass.scene_sampling);
  result[340 / 4] = inputs.normal_mapping;
  result[348 / 4] = inputs.receiver_filter;
  result[352 / 4] = inputs.fog_enabled;
  result[360 / 4] = inputs.specular_enabled;
  result[364 / 4] = inputs.light_count > 0;
  result[368 / 4] = inputs.light_count > 1;
  for (size_t offset = 372; offset <= 384; offset += 4)
    result[offset / 4] = 1;
  for (size_t offset = 396; offset <= 404; offset += 4)
    result[offset / 4] = std::bit_cast<uint32_t>(1.0f);
  // The compatibility consumer compares this modification serial. Reset plus
  // seven ambient/camera/colour/bias writes, two scene writes, optional extent
  // writes, and each nonzero feature transition. No native consumer needs it.
  result[408 / 4] = 9 + (inputs.sample_extent ? 2 : 0) +
      (inputs.receiver_filter != 0) + (inputs.fog_enabled != 0) +
      (inputs.specular_enabled != 0) + (inputs.normal_mapping != 0) +
      (inputs.light_count > 0) + (inputs.light_count > 1);
  return result;
}
} // namespace bd::gpu::scene
