/**
 * @file    native_transform.h
 * @brief   Native object/pass transforms, independent of engine memory and
 * shaders.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace bd::gpu::scene {
using RenderMatrix = std::array<float, 16>; // row-major, row-vector convention
struct RenderTransformInputs {
  RenderMatrix world{}, view{}, projection{};
};
struct RenderTransforms {
  RenderTransformInputs inputs;
  RenderMatrix view_projection{};
};

inline RenderMatrix TransposeRenderMatrix(const RenderMatrix &matrix) {
  RenderMatrix result;
  for (size_t row = 0; row < 4; ++row)
    for (size_t column = 0; column < 4; ++column)
      result[row * 4 + column] = matrix[column * 4 + row];
  return result;
}

// Arithmetic layer also preserves IEEE exceptional values from transitional
// engine imports. Native assets should use the checked entry point below.
inline RenderMatrix MultiplyRenderMatrices(const RenderMatrix &left,
                                          const RenderMatrix &right) {
#if defined(__clang__)
#pragma clang fp contract(off)
#endif
  RenderMatrix result;
  // Explicit pairwise sums preserve the established CPU transform convention.
  // The native output is not a register block; packing belongs to a backend.
  for (size_t row = 0; row < 4; ++row)
    for (size_t column = 0; column < 4; ++column) {
      const auto *v = left.data() + row * 4;
      const auto *p = right.data() + column;
      const float value =
          (v[0] * p[0] + v[1] * p[4]) + (v[2] * p[8] + v[3] * p[12]);
      result[row * 4 + column] = value;
    }
  return result;
}

inline RenderTransforms
ComposeRenderTransformValues(const RenderTransformInputs &inputs) {
  return {inputs, MultiplyRenderMatrices(inputs.view, inputs.projection)};
}

inline std::optional<RenderTransforms>
ComposeRenderTransforms(const RenderTransformInputs &inputs) {
  for (const auto *matrix : {&inputs.world, &inputs.view, &inputs.projection})
    for (float value : *matrix)
      if (!std::isfinite(value))
        return {};
  auto result = ComposeRenderTransformValues(inputs);
  for (float value : result.view_projection)
    if (!std::isfinite(value))
      return {};
  return result;
}

struct RenderCamera {
  RenderMatrix view{}, projection{}, world_to_clip{};
};

// One pass owns this state. Seeing an object's world matrix cannot bootstrap a
// camera from inherited engine cache values. Both camera inputs must have been
// explicitly produced in this scope; late unknown writes invalidate ownership.
class RenderCameraState {
  RenderCamera camera_;
  std::array<bool, 2> known_{};
public:
  void Reset() { known_ = {}; }
  void Publish(const RenderTransformInputs &inputs, bool view_changed, bool projection_changed,
               bool suppressed) {
    if (suppressed) { Reset(); return; }
    const std::array<const RenderMatrix *, 2> values{&inputs.view, &inputs.projection};
    const std::array<RenderMatrix *, 2> owned{&camera_.view, &camera_.projection};
    const std::array<bool, 2> changed{view_changed, projection_changed};
    for (uint32_t n = 0; n < 2; ++n) {
      for (float value : *values[n]) if (!std::isfinite(value)) { Reset(); return; }
      if (changed[n]) { *owned[n] = *values[n]; known_[n] = true; }
      else if (known_[n] && *owned[n] != *values[n]) known_[n] = false;
    }
    if (known_[0] && known_[1]) {
      camera_.world_to_clip = MultiplyRenderMatrices(camera_.view, camera_.projection);
      for (float value : camera_.world_to_clip) if (!std::isfinite(value)) { Reset(); return; }
    }
  }
  std::optional<RenderCamera> Read() const {
    return known_[0] && known_[1] ? std::optional(camera_) : std::nullopt;
  }
};
} // namespace bd::gpu::scene
