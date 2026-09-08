/**
 * @brief Bounded native pose palettes shared by instanced GPU skin consumers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_rigid_inputs.h"

namespace bd::gpu::scene {
inline constexpr uint32_t kNativeSkinPaletteMatrices = 16384; // 1MiB per batch
struct NativeSkinPalettePlan {
  std::vector<std::shared_ptr<const NativeInstancePose>> poses;
  std::vector<RigidUint4> ranges; // base/count per GPU instance; zw reserved zero
  uint32_t matrices = 0;
};
inline std::optional<NativeSkinPalettePlan> PlanNativeSkinPalette(
    std::span<const std::shared_ptr<const NativeInstancePose>> instances) {
  if (instances.empty() || instances.size() > 256) return {};
  NativeSkinPalettePlan result;
  for (const auto &pose : instances) {
    if (!pose || !pose->instance || !pose->model || pose->model_generation != pose->model->Generation() ||
        pose->transforms.empty() || pose->transforms.size() > NativeInstanceRegistry::kMaxTransforms) return {};
    uint32_t first = 0;
    const auto found = std::find(result.poses.begin(),result.poses.end(),pose);
    for (auto it = result.poses.begin(); it != found; ++it) first += uint32_t((*it)->transforms.size());
    if (found == result.poses.end()) {
      if (pose->transforms.size() > kNativeSkinPaletteMatrices-result.matrices) return {};
      for (const auto &matrix : pose->transforms) {
        for (float value : matrix) if (!std::isfinite(value)) return {};
        if (matrix[3] != 0 || matrix[7] != 0 || matrix[11] != 0 || matrix[15] != 1) return {};
      }
      result.matrices += uint32_t(pose->transforms.size());
      result.poses.push_back(pose);
    }
    result.ranges.push_back({first,uint32_t(pose->transforms.size()),0,0});
  }
  return result;
}
} // namespace bd::gpu::scene
