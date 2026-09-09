/**
 * @brief Authored gaze -> owned eye material offsets, independent of source storage.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace bd::gpu::scene {
struct NativeEyeControl {
  std::array<float,2> gaze{}, origin{}, minimum{}, maximum{};
};
inline std::optional<std::array<std::array<float,2>,2>> EvaluateNativeEyeUV(const NativeEyeControl &input) {
  for (const auto &pair : {input.gaze,input.origin,input.minimum,input.maximum})
    for (float value : pair) if (!std::isfinite(value)) return {};
  std::array<std::array<float,2>,2> result;
  for (size_t eye=0; eye<2; ++eye) for (size_t axis=0; axis<2; ++axis) {
    const float control=eye == 0 && axis == 0 ? -input.gaze[axis] : input.gaze[axis];
    // Authored controls extrapolate; do not clamp gaze or reorder the limits.
    // Preserve the rounded subtraction followed by double FMA and float store.
    const float extent=control < 0 ? float(double(input.origin[axis])-input.minimum[axis]) :
        float(double(input.maximum[axis])-input.origin[axis]);
    result[eye][axis]=float(std::fma(double(extent),double(control),double(input.origin[axis])));
    if (!std::isfinite(result[eye][axis])) return {};
  }
  return result;
}
struct NativeEyeMaterial {
  struct Entry {
    uint32_t selector=0, channel=0;
    std::array<float,2> uv{};
    bool enabled=false;
  };
  std::array<Entry,2> entries{};
  uint32_t count=0;
  bool Valid() const {
    if (!count || count > entries.size()) return false;
    for (uint32_t i=0; i<count; ++i)
      for (float value : entries[i].uv) if (!std::isfinite(value)) return false;
    return true;
  }
};
} // namespace bd::gpu::scene
