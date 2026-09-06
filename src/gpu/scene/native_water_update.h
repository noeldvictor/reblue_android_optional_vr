/**
 * @brief Native water animation and scene-sampling demand policy.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>

namespace bd::gpu::scene {
// Preserve authored rounding and a single strict-greater wrap, not modulo.
inline float AddWaterPhase(float phase, float step) { return float(double(phase) + double(step)); }
inline float WrapWaterPhase(float phase, float limit) { return float(double(phase) - double(limit)); }
inline float ScaleWaterParameter(float value, float scale) { return float(double(value) * double(scale)); }
struct SamplingDemandChange {
  bool changed = false;
  int32_t add = -1, remove = -1;
};
constexpr SamplingDemandChange ChangeSamplingDemand(uint32_t previous, uint32_t next) {
  const auto index = [](uint32_t mode) { return mode == 1 ? 0 : mode == 3 ? 1 : -1; };
  return previous == next ? SamplingDemandChange{} : SamplingDemandChange{true, index(next), index(previous)};
}
} // namespace bd::gpu::scene
