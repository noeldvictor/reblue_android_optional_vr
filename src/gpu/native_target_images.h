/**
 * @brief Native single-sample scene image ownership and fence-gated retirement.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_image_lease.h"
#include "gpu/scene/fenced_asset_cache.h"
#include <plume_render_interface.h>

namespace bd::gpu {
struct VideoState;
struct GuestTexture;
struct NativeTargetShape {
  uint32_t width = 0, height = 0, layers = 0;
  plume::RenderFormat format = plume::RenderFormat::UNKNOWN;
  bool operator==(const NativeTargetShape &) const = default;
  uint64_t Bytes(uint64_t budget) const {
    // This owner currently admits the scene's single-sample HDR/depth formats.
    if (!width || !height || !layers || layers > 2 ||
        (format != plume::RenderFormat::R16G16B16A16_FLOAT &&
         format != plume::RenderFormat::D32_FLOAT_S8_UINT)) return 0;
    const uint64_t pixels = uint64_t(width) * height, stride = 8 * layers;
    return pixels <= budget / stride ? pixels * stride : 0;
  }
};
struct NativeTargetImage {
  NativeTargetShape shape;
  // Descriptor invalidation precedes view destruction, then image destruction.
  std::unique_ptr<plume::RenderTexture> image;
  std::unique_ptr<plume::RenderTextureView> view;
  uint32_t descriptor = ~uint32_t{0};
  mutable plume::RenderTextureLayout layout = plume::RenderTextureLayout::UNKNOWN;
  SampledImage Sampled() const {
    return {image.get(), &layout, shape.width, shape.height, shape.layers,
        shape.format, descriptor, 1};
  }
};
using NativeTargetImageHandle = std::shared_ptr<const NativeTargetImage>;

// Serialized by the renderer lock. Identity is a native image generation, not
// a resource header or address. Adoption moves existing GPU objects, never
// duplicates them. Source pass pins still serialize writes to persistent targets;
// these leases independently keep old images alive across source recreation.
class NativeTargetImageStore {
public:
  explicit NativeTargetImageStore(uint64_t bytes = 512ull << 20, uint64_t entries = 64)
      : budget_(bytes), images_(bytes, entries) {}
  NativeTargetImageHandle Adopt(uint64_t id, const NativeTargetShape &shape,
      std::unique_ptr<plume::RenderTexture> &image,
      std::unique_ptr<plume::RenderTextureView> &view,
      uint32_t descriptor, plume::RenderTextureLayout layout) {
    const auto bytes = shape.Bytes(budget_);
    if (!id || !bytes || descriptor == ~uint32_t{0} || bool(image) != bool(view)) return {};
    std::shared_ptr<NativeTargetImage> created;
    auto result = images_.Acquire(id, bytes, [&]() -> NativeTargetImageHandle {
      if (!image || !view) return {};
      created = std::make_shared<NativeTargetImage>();
      created->shape = shape;
      created->descriptor = descriptor;
      created->layout = layout;
      return created;
    });
    if (!result || result->shape != shape || result->descriptor != descriptor) return {};
    if (created) {
      // All fallible allocation/cache insertion precedes ownership transfer.
      created->image = std::move(image);
      created->view = std::move(view);
    } else if (image || view) return {}; // never consume another generation's objects
    return result;
  }
  template <typename Retire> void AfterFence(uint32_t slot, Retire &&retire) {
    images_.AfterFence(slot, std::forward<Retire>(retire));
  }
  void MarkUnused(uint32_t slot) { images_.MarkUnused(slot); }
  scene::FencedAssetStats Stats() const { return images_.Stats(); }
private:
  uint64_t budget_;
  scene::FencedAssetCache<NativeTargetImage> images_;
};

// Temporary surface binding boundary. Returned leases, not surface headers,
// own GPU images. Native creation of the source attachments remains separate work.
NativeImageLease AdoptNativeTargetImage(GuestTexture *target);
void DrainNativeTargetImagesLocked(VideoState &s, uint32_t slot);
void MarkUnusedNativeTargetImagesLocked(VideoState &s, uint32_t slot);
} // namespace bd::gpu
