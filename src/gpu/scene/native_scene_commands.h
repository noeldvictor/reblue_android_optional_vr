/**
 * @brief Native scene attachment writes and scope-owned clears.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_target_images.h"
#include <array>
#include <cmath>
#include <optional>

namespace bd::gpu::scene {
struct NativeSceneClear {
  plume::RenderColor color;
  float depth = 1.f;
  uint8_t stencil = 0;
};

// A scene scope retains the framebuffer/resolve owners. This command recipe
// holds its native sources and live layout records, never resource-header state.
class NativeSceneCommands {
public:
  static std::optional<NativeSceneCommands> Create(
      const std::array<NativeTargetImageHandle, 2> &sources,
      plume::RenderFramebuffer *framebuffer, const std::array<SampledImage, 2> &resolved,
      const NativeSceneClear &clear) {
    if (!framebuffer || !sources[0] || !sources[1] || !std::isfinite(clear.depth) ||
        clear.depth < 0.f || clear.depth > 1.f) return {};
    for (float channel : clear.color.rgba) if (!std::isfinite(channel)) return {};
    const auto &color = sources[0]->shape;
    const auto &depth = sources[1]->shape;
    if (!color.Bytes(512ull << 20) || !depth.Bytes(512ull << 20) ||
        color.format != plume::RenderFormat::R16G16B16A16_FLOAT ||
        depth.format != plume::RenderFormat::D32_FLOAT_S8_UINT ||
        color.width != depth.width || color.height != depth.height || color.layers != depth.layers ||
        color.samples != depth.samples || !sources[0]->image || !sources[1]->image ||
        sources[0]->image.get() == sources[1]->image.get() ||
        framebuffer->getWidth() != color.width || framebuffer->getHeight() != color.height) return {};
    if (color.samples == 1) {
      if (resolved[0].texture || resolved[1].texture) return {};
    } else {
      for (uint32_t i = 0; i < 2; ++i) {
        const auto &image = resolved[i];
        if (!image || image.width != color.width || image.height != color.height ||
            image.layers != color.layers || image.format != sources[i]->shape.format ||
            image.texture == sources[0]->image.get() || image.texture == sources[1]->image.get() ||
            image.layout == &sources[0]->layout || image.layout == &sources[1]->layout) return {};
      }
      if (resolved[0].texture == resolved[1].texture || resolved[0].layout == resolved[1].layout) return {};
    }
    NativeSceneCommands result;
    result.sources_ = sources;
    result.framebuffer_ = framebuffer;
    result.resolved_ = resolved;
    result.clear_ = clear;
    return result;
  }
  bool Matches(const plume::RenderTexture *color, const plume::RenderTexture *depth) const {
    return sources_[0] && sources_[1] && sources_[0]->image.get() == color && sources_[1]->image.get() == depth;
  }
  plume::RenderFramebuffer *Framebuffer() const { return framebuffer_; }
  bool ClearPending() const { return clear_.has_value(); }
  // Read only after ending this scope's active render pass. For MSAA this is
  // the ordinary attachment-resolve destination, never the multisample source.
  SampledImage ColorReadImage() const {
    return resolved_[0].texture ? resolved_[0] : sources_[0]->Sampled();
  }

  // Caller flushes outgoing draws before any transition. A resumed pass reuses
  // its contents: only UNKNOWN sources discard, and the scope clears once.
  template <typename Commands> uint32_t Bind(Commands &commands) const {
    std::array<plume::RenderTextureBarrier, 4> barriers;
    std::array<plume::RenderTexture *, 2> fresh{};
    uint32_t count = 0;
    for (uint32_t i = 0; i < 4; ++i) {
      auto *image = i < 2 ? sources_[i]->image.get() : resolved_[i - 2].texture;
      auto *layout = i < 2 ? &sources_[i]->layout : resolved_[i - 2].layout;
      if (!image) continue;
      const auto wanted = i % 2 ? plume::RenderTextureLayout::DEPTH_WRITE : plume::RenderTextureLayout::COLOR_WRITE;
      if (i < 2 && *layout == plume::RenderTextureLayout::UNKNOWN) fresh[i] = image;
      if (*layout != wanted) {
        barriers[count++] = {image, wanted};
        *layout = wanted;
      }
    }
    if (count) commands.barriers(plume::RenderBarrierStage::GRAPHICS, barriers.data(), count);
    for (auto *image : fresh) if (image) commands.discardTexture(image);
    commands.setFramebuffer(framebuffer_);
    return count;
  }
  template <typename Commands> bool ApplyClear(Commands &commands) {
    if (!clear_) return false;
    commands.clearColor(0, clear_->color);
    commands.clearDepthStencil(true, true, clear_->depth, clear_->stencil);
    clear_.reset();
    return true;
  }
private:
  std::array<NativeTargetImageHandle, 2> sources_;
  plume::RenderFramebuffer *framebuffer_ = nullptr;
  std::array<SampledImage, 2> resolved_;
  std::optional<NativeSceneClear> clear_;
};
} // namespace bd::gpu::scene
