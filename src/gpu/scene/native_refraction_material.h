/**
 * @brief Host water/refraction material preparation, independent of resource ABI.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>

namespace bd::gpu::scene {
constexpr float WaterSceneFactor(bool scene, bool force_authored, int32_t authored,
                                float default_factor = 1.f) {
  return scene && (force_authored || authored != 1) ? float(authored) : default_factor;
}
constexpr float ClampWaterHighlight(float value) { return value > 1.f ? 1.f : value; }

// Read inputs at their point of use: parameter publication can change a later
// descriptor, and image publication precedes the live snapshot-enable decision.
// Other blend terms and depth-write policy remain inherited, never reset here.
template <class Adapter> void PrepareWaterMaterial(Adapter &adapter) {
  adapter.PublishSceneFactor();
  adapter.FlushWaterParameters(0);
  adapter.FlushWaterParameters(1);
  adapter.EnableSourceAlphaBlending();
  adapter.EnableDepthTest();
  adapter.BindPlanarReflection();
  adapter.BindSceneImage();
  if (adapter.WantsSnapshot()) adapter.Snapshot();
}
template <class Adapter> void PrepareRefractionMaterial(Adapter &adapter) {
  adapter.FlushRefractionParameters();
  adapter.Snapshot();
}
} // namespace bd::gpu::scene
