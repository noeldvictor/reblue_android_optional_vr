/**
 * @brief Native scene MSAA resolve images and fence-gated ownership.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/host_post_inputs.h"
#include "gpu/native_target_images.h"
#include <plume_render_interface.h>
#include <array>
#include <memory>

namespace bd::gpu {
struct VideoState;
namespace scene {
struct SceneResolveSources {
  plume::RenderTexture *color = nullptr, *depth = nullptr;
  uint64_t color_identity = 0, depth_identity = 0;
  uint32_t width = 0, height = 0, layers = 0, samples = 0;
  plume::RenderFormat color_format = plume::RenderFormat::UNKNOWN;
  plume::RenderFormat depth_format = plume::RenderFormat::UNKNOWN;
  plume::RenderTexture *density_map = nullptr;
  bool operator==(const SceneResolveSources &) const = default;
};

struct NativeSceneResolves {
  SceneResolveSources sources;
  // A retained resolved getter must also retain the sources referenced by its
  // framebuffer. These owners die last, after the framebuffer and its views.
  std::array<NativeTargetImageHandle, 2> source_owners;
  // Views and framebuffers die before their native owned images.
  std::array<std::unique_ptr<plume::RenderTexture>, 2> images;
  std::array<std::unique_ptr<plume::RenderTextureView>, 2> views;
  std::unique_ptr<plume::RenderFramebuffer> framebuffer;
  std::array<uint32_t, 2> descriptors{~uint32_t{0}, ~uint32_t{0}};
  mutable std::array<plume::RenderTextureLayout, 2> layouts{
      plume::RenderTextureLayout::UNKNOWN, plume::RenderTextureLayout::UNKNOWN};

  HostPostInputs Sampled(float exposure) const {
    const auto image = [&](uint32_t i) {
      return SampledImage{images[i].get(), &layouts[i], sources.width, sources.height,
          sources.layers, i ? sources.depth_format : sources.color_format, descriptors[i], 1};
    };
    return {image(0), image(1), exposure, true};
  }
};
using NativeSceneResolveHandle = std::shared_ptr<const NativeSceneResolves>;

// Native source identity/format in, independently owned native images out.
// No guest resource allocation, EDRAM identity, copy or draw is performed.
NativeSceneResolveHandle AcquireNativeSceneResolves(const SceneResolveSources &sources,
    const std::array<NativeTargetImageHandle, 2> &source_owners);
// Called with the video lock held, after queued work is flushed. Write-side
// transitions belong to NativeSceneCommands alongside its source attachments.
void FinishNativeSceneResolves(VideoState &s, const NativeSceneResolves &images);
void DrainNativeSceneResolvesLocked(VideoState &s, uint32_t slot);
void MarkUnusedNativeSceneResolvesLocked(VideoState &s, uint32_t slot);
} // namespace scene
} // namespace bd::gpu
