/**
 * @brief Native single-sample scene framebuffers, independent of binding headers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_target_images.h"
#include <algorithm>
#include <array>
#include <vector>

namespace bd::gpu::scene {
struct NativeSceneFramebuffer {
  // Retain attachments until AFTER the framebuffer dies, including retirement.
  std::array<NativeTargetImageHandle, 2> sources;
  NativeImageLease color;
  const plume::RenderTexture *density_map = nullptr; // device-lifetime foveation store
  std::unique_ptr<plume::RenderFramebuffer> framebuffer;

  bool Matches(const plume::RenderTexture *color, const plume::RenderTexture *depth) const {
    return sources[1] && this->color.image.texture == color &&
        sources[1]->image.get() == depth;
  }
};
using NativeSceneFramebufferHandle = std::shared_ptr<const NativeSceneFramebuffer>;

// The source image store accounts for image bytes. This store bounds framebuffer
// count/host metadata, not driver-private allocation sizes. No source is copied.
// All operations are serialized by the video lock; pending entries remain counted.
class NativeSceneFramebufferStore {
public:
  explicit NativeSceneFramebufferStore(uint64_t entries = 64)
      : frames_(sizeof(NativeSceneFramebuffer) * entries, entries) {}

  template <typename Create>
  NativeSceneFramebufferHandle Acquire(const std::array<NativeTargetImageHandle, 2> &sources,
      const plume::RenderTexture *density_map, Create &&create) {
    if (!Compatible(sources) || (!sources[0] && density_map)) return {};
    return AcquireOwned(sources,NativeImageLease::From(sources[0]),density_map,std::forward<Create>(create));
  }
  template <typename Create>
  NativeSceneFramebufferHandle AcquireLeasedColor(const NativeImageLease &color, const NativeTargetImageHandle &depth,
      Create &&create) {
    if (!color.ArrayView() || !depth || !depth->Sampled() || !depth->shape.Bytes(512ull<<20) ||
        depth->shape.format != plume::RenderFormat::D32_FLOAT_S8_UINT || depth->shape.samples != 1 ||
        color.image.format != plume::RenderFormat::R16G16B16A16_FLOAT ||
        !color.Fits(depth->shape.width,depth->shape.height,depth->shape.layers) ||
        color.image.texture == depth->image.get() || color.image.layout == &depth->layout) return {};
    return AcquireOwned({NativeTargetImageHandle{},depth},color,nullptr,std::forward<Create>(create));
  }
private:
  template <typename Create>
  NativeSceneFramebufferHandle AcquireOwned(const std::array<NativeTargetImageHandle,2> &sources,
      const NativeImageLease &color, const plume::RenderTexture *density_map, Create &&create) {
    // The cache keeps the old owner alive until its fence; expired weak keys
    // cannot match recycled image addresses or retain source images themselves.
    std::erase_if(keys_, [](const auto &key) { return key.owner.expired(); });
    const auto it = std::find_if(keys_.begin(), keys_.end(), [&](const auto &key) {
      const auto owner = key.owner.lock();
      return owner && owner->sources == sources && owner->color == color && owner->density_map == density_map;
    });
    const bool existing = it != keys_.end();
    const auto id = existing ? it->id : ++next_id_;
    auto result = frames_.Acquire(id, sizeof(NativeSceneFramebuffer), [&]() -> NativeSceneFramebufferHandle {
      auto owner = std::make_shared<NativeSceneFramebuffer>();
      owner->sources = sources;
      owner->color = color;
      owner->density_map = density_map;
      const plume::RenderTexture *colors[]{color.image.texture};
      plume::RenderFramebufferDesc desc;
      desc.colorAttachments = color ? colors : nullptr;
      desc.colorAttachmentsCount = color ? 1u : 0u;
      desc.depthAttachment = sources[1]->image.get();
      desc.viewMask = sources[1]->shape.layers == 2 ? 3u : 0u;
      desc.fragmentDensityMap = density_map;
      owner->framebuffer = std::forward<Create>(create)(desc);
      return owner->framebuffer ? owner : NativeSceneFramebufferHandle{};
    });
    if (result && !existing) keys_.push_back({id, result});
    return result;
  }
public:
  void AfterFence(uint32_t slot) {
    frames_.AfterFence(slot, [](const NativeSceneFramebuffer &) {});
  }
  void MarkUnused(uint32_t slot) { frames_.MarkUnused(slot); }
  FencedAssetStats Stats() const { return frames_.Stats(); }

private:
  static bool Compatible(const std::array<NativeTargetImageHandle, 2> &sources) {
    if (!sources[1] || !sources[1]->Sampled() || !sources[1]->shape.Bytes(512ull << 20)) return false;
    const auto &depth = sources[1]->shape;
    if (depth.format != plume::RenderFormat::D32_FLOAT_S8_UINT) return false;
    // A single-sample depth-only pass uses the same native owner/cache.
    if (!sources[0]) return true;
    if (!sources[0]->Sampled() || !sources[0]->shape.Bytes(512ull << 20) ||
        sources[0]->image.get() == sources[1]->image.get()) return false;
    const auto &color = sources[0]->shape;
    return color.format == plume::RenderFormat::R16G16B16A16_FLOAT &&
        depth.format == plume::RenderFormat::D32_FLOAT_S8_UINT &&
        color.width == depth.width && color.height == depth.height && color.layers == depth.layers;
  }
  struct Key { uint64_t id; std::weak_ptr<const NativeSceneFramebuffer> owner; };
  FencedAssetCache<NativeSceneFramebuffer> frames_;
  std::vector<Key> keys_;
  uint64_t next_id_ = 0;
};

NativeSceneFramebufferHandle AcquireNativeSceneFramebuffer(
    const std::array<NativeTargetImageHandle, 2> &sources, const plume::RenderTexture *density_map);
NativeSceneFramebufferHandle AcquireNativeLeasedColorFramebuffer(
    const NativeImageLease &color, const NativeTargetImageHandle &depth);
void DrainNativeSceneFramebuffersLocked(VideoState &s, uint32_t slot);
void MarkUnusedNativeSceneFramebuffersLocked(VideoState &s, uint32_t slot);
} // namespace bd::gpu::scene
