/**
 * @file    native_view_schedule.h
 * @brief   Host ordering of complete view work, independent of the engine ABI.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
namespace bd::gpu::scene {
// Conditions are sampled at their execution boundary, not at frame start:
// preparation and earlier passes may update later work. Indexed view counts
// also remain live between iterations. The adapter imports authored requests;
// it does not choose a different pass order or bypass the main/post boundary.
template <class Adapter> void ScheduleRenderView(Adapter &adapter) {
  adapter.PrepareView();
  for (int32_t i = 0; i < adapter.IndexedViewCount(); ++i)
    if (adapter.IndexedViewEnabled(uint32_t(i)))
      adapter.RenderIndexedView(uint32_t(i));
  adapter.SelectPrimaryView();
  if (adapter.SunShadowRequested()) adapter.RenderSunShadow();
  if (adapter.CubeShadowRequested()) adapter.RenderCubeShadow();
  if (adapter.AuxiliaryRequested()) adapter.RenderAuxiliary();
  if (adapter.ShadowVolumeRequested()) adapter.RenderShadowVolume();
  if (adapter.ReflectionsRequested()) adapter.RenderReflections();
  if (adapter.EnvironmentRequested()) adapter.RenderEnvironment();
  if (adapter.AdditionalSceneRequested()) adapter.RenderAdditionalScene();
  adapter.RenderMainScene();
  if (adapter.PostRequested()) adapter.RenderPost();
  adapter.RestoreView();
}
} // namespace bd::gpu::scene
