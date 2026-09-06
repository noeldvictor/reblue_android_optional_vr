/**
 * @brief Device-owned native scene framebuffer creation and fence retirement.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_scene_framebuffer.h"
#include "gpu/device.h"
#include "core/logging.h"

namespace bd::gpu::scene {
NativeSceneFramebufferHandle AcquireNativeSceneFramebuffer(
    const std::array<NativeTargetImageHandle, 2> &sources, const plume::RenderTexture *density_map) {
  auto &s = state();
  std::lock_guard lock(s.mutex);
  if (!s.ready || s.shutting_down.load() || !sources[0] ||
      (sources[0]->shape.layers == 2 && !s.device->getCapabilities().multiview) ||
      (density_map && !s.device->getCapabilities().fragmentDensityMap)) return {};
  if (!s.native_scene_framebuffers)
    s.native_scene_framebuffers = std::make_shared<NativeSceneFramebufferStore>();
  auto result = s.native_scene_framebuffers->Acquire(sources, density_map,
      [&](const plume::RenderFramebufferDesc &desc) { return s.device->createFramebuffer(desc); });
  const auto stats = s.native_scene_framebuffers->Stats();
  if (!result || stats.created + stats.reused == 1 || (stats.created + stats.reused) % 600 == 0)
    BD_INFO("[native-scene-framebuffers] {} created {} reused {} retired {} resident; "
            "{} refused {} failed; native attachment owners, no resource-header cache",
        stats.created, stats.reused, stats.retired, stats.resident, stats.refused, stats.failed);
  return result;
}
void DrainNativeSceneFramebuffersLocked(VideoState &s, uint32_t slot) {
  if (s.native_scene_framebuffers) s.native_scene_framebuffers->AfterFence(slot);
}
void MarkUnusedNativeSceneFramebuffersLocked(VideoState &s, uint32_t slot) {
  if (s.native_scene_framebuffers) s.native_scene_framebuffers->MarkUnused(slot);
}
} // namespace bd::gpu::scene
