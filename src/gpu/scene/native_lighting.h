/**
 * @file    native_lighting.h
 * @brief   Address-free lighting inputs and host pass composition.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <utility>

namespace bd::gpu::scene {
using LightingVector = std::array<float, 4>;
struct LightingExtent {
  uint32_t width = 0, height = 0;
};
struct NativeLightingInputs {
  bool receivers_enabled = true;
  // Preserve byte values at the temporary ABI boundary. The normal-lit shader
  // calls PS b3 normal mapping, b6 fog and b8 specular (not shadow modes).
  uint8_t receiver_filter = 0, fog_enabled = 0, specular_enabled = 0;
  uint8_t normal_mapping = 0;
  int32_t light_count = 0;
  LightingVector ambient{}, camera_position{}, color_scale{};
  float shadow_bias = 0, shadow_threshold = 0;
  float shadow_kernel_scale = 0.25f;
  std::optional<LightingExtent> sample_extent;
  std::array<float, 3> scene_origin{};
  float scene_range = 1;
  // Light-selection view is NOT the render-view identity. The update pins the
  // coherent scene/object publication used by this pass, not a shader cache.
  uint32_t light_view = 0;
  uint64_t light_update = 0;
};
struct NativeLightingPass {
  NativeLightingInputs inputs;
  LightingVector shadow_sampling{};
  LightingVector scene_sampling{};
};

// One current pass, not a history cache. A reset/unsupported producer discards
// eligibility; another view or frame cannot borrow the preceding publication.
// Copies already returned to queued native packets retain their value lifetime.
class NativeLightingPublication {
  std::optional<NativeLightingPass> pass_;
  uint32_t frame_ = 0, view_ = 0;
public:
  void Publish(NativeLightingPass pass, uint32_t frame, uint32_t view) {
    pass_ = std::move(pass); frame_ = frame; view_ = view;
  }
  void Reset() { pass_.reset(); }
  std::optional<NativeLightingPass> Read(uint32_t frame, uint32_t view) const {
    return frame == frame_ && view == view_ ? pass_ : std::nullopt;
  }
};

inline NativeLightingPass ComposeNativeLighting(NativeLightingInputs inputs) {
  if (!inputs.receivers_enabled) {
    inputs.receiver_filter = 0;
    inputs.normal_mapping = 0;
  }
  NativeLightingPass result{inputs};
  result.shadow_sampling = {inputs.shadow_bias, inputs.shadow_threshold, 0, 0};
  if (inputs.sample_extent) {
    result.shadow_sampling[2] = float(inputs.sample_extent->width) * inputs.shadow_kernel_scale;
    result.shadow_sampling[3] = float(inputs.sample_extent->height) * inputs.shadow_kernel_scale;
  }
  result.scene_sampling = {inputs.scene_origin[0], inputs.scene_origin[1],
                           inputs.scene_origin[2], 1.0f / inputs.scene_range};
  return result;
}
} // namespace bd::gpu::scene
