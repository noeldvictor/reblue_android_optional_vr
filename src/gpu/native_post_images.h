/**
 * @brief Native HDR post images, exclusive write leases and fenced residency.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/host_post_output.h"
#include "gpu/scene/fenced_asset_cache.h"
#include <algorithm>
#include <memory>
#include <vector>

namespace bd::gpu {
struct VideoState;
struct NativePostRecipe {
  uint32_t width = 0, height = 0, layers = 0;
  plume::RenderTexture *density_map = nullptr;
  bool operator==(const NativePostRecipe &) const = default;
};
struct NativePostImage {
  NativePostRecipe recipe;
  // Framebuffers/views die before the image; the pool retires its descriptor first.
  std::unique_ptr<plume::RenderTexture> image;
  std::unique_ptr<plume::RenderTextureView> view;
  std::unique_ptr<plume::RenderFramebuffer> framebuffer;
  uint32_t descriptor = ~uint32_t{0};
  mutable plume::RenderTextureLayout layout = plume::RenderTextureLayout::UNKNOWN;
  HostPostOutput Output() const {
    return {{image.get(), &layout, recipe.width, recipe.height, recipe.layers,
        plume::RenderFormat::R16G16B16A16_FLOAT, descriptor, 1}, framebuffer.get()};
  }
};
using NativePostImageHandle = std::shared_ptr<const NativePostImage>;

// Serialized by the renderer lock. An outstanding reader or writer prevents
// another write lease of the same physical image. Pool-only images can be reused
// with ordered GPU barriers; destruction still waits for a proven fence.
class NativePostImagePool {
public:
  explicit NativePostImagePool(uint64_t bytes = 256ull << 20, uint64_t entries = 64)
      : budget_(bytes), images_(bytes, entries) {}
  template <typename Create>
  NativePostImageHandle Acquire(const NativePostRecipe &recipe, Create &&create) {
    if (!recipe.width || !recipe.height || !recipe.layers || recipe.layers > 2) return {};
    const uint64_t pixels = uint64_t(recipe.width) * recipe.height;
    const uint64_t stride = 8 * recipe.layers; // single-sample FP16 RGBA
    if (pixels > budget_ / stride) return {};
    const uint64_t bytes = pixels * stride;
    std::erase_if(entries_, [](const auto &entry) { return entry.handle.expired(); });
    const auto it = std::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
      return entry.recipe == recipe && entry.handle.use_count() == 1;
    });
    const bool existing = it != entries_.end();
    const auto id = existing ? it->id : ++next_id_;
    auto result = images_.Acquire(id, bytes, std::forward<Create>(create));
    if (result) {
      if (!existing) entries_.push_back({recipe, id, result});
      // Force ordering before the next full overwrite, including when the last
      // user also left it in a write layout. The backend owns the actual old
      // layout. This does NOT discard or reset the GPU image.
      result->layout = plume::RenderTextureLayout::UNKNOWN;
    }
    return result;
  }
  template <typename Retire> void AfterFence(uint32_t slot, Retire &&retire) {
    images_.AfterFence(slot, std::forward<Retire>(retire));
  }
  void MarkUnused(uint32_t slot) { images_.MarkUnused(slot); }
  scene::FencedAssetStats Stats() const { return images_.Stats(); }
private:
  struct Entry {
    NativePostRecipe recipe;
    uint64_t id;
    std::weak_ptr<const NativePostImage> handle;
  };
  uint64_t budget_, next_id_ = 0;
  scene::FencedAssetCache<NativePostImage> images_;
  std::vector<Entry> entries_;
};

NativePostImageHandle AcquireNativePostImage(uint32_t width, uint32_t height, uint32_t layers);
void DrainNativePostImagesLocked(VideoState &s, uint32_t slot);
void MarkUnusedNativePostImagesLocked(VideoState &s, uint32_t slot);
} // namespace bd::gpu
