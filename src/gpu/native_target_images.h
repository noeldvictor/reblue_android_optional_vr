/**
 * @brief Native scene attachment recipes, ownership and fence-gated retirement.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_image_lease.h"
#include "gpu/scene/fenced_asset_cache.h"
#include <plume_render_interface.h>
#include <bit>

namespace bd::gpu {
struct VideoState;
struct NativeTargetShape {
  uint32_t width = 0, height = 0, layers = 0;
  plume::RenderFormat format = plume::RenderFormat::UNKNOWN;
  uint32_t samples = 1;
  bool operator==(const NativeTargetShape &) const = default;
  uint64_t Bytes(uint64_t budget) const {
    // Explicit scene HDR/depth formats; sample count is a native image property.
    if (!width || !height || !layers || layers > 2 ||
        !std::has_single_bit(samples) || samples > 8 ||
        (format != plume::RenderFormat::R16G16B16A16_FLOAT &&
         format != plume::RenderFormat::D32_FLOAT_S8_UINT)) return 0;
    const uint64_t pixels = uint64_t(width) * height, stride = 8 * layers * samples;
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
        shape.format, descriptor, shape.samples};
  }
};
using NativeTargetImageHandle = std::shared_ptr<const NativeTargetImage>;

// Serialized by the renderer lock. Identity is a native image generation, not
// a resource header or address. Source pass pins serialize persistent writes;
// these leases independently keep old images alive across source recreation.
class NativeTargetImageStore {
public:
  explicit NativeTargetImageStore(uint64_t bytes = 512ull << 20, uint64_t entries = 64)
      : budget_(bytes), images_(bytes, entries) {}
  template <typename Create>
  NativeTargetImageHandle Acquire(uint64_t id, const NativeTargetShape &shape, Create &&create) {
    const auto bytes = shape.Bytes(budget_);
    if (!id || !bytes) return {};
    auto result = images_.Acquire(id, bytes, [&]() -> NativeTargetImageHandle {
      return std::forward<Create>(create)();
    });
    return result && result->shape == shape ? result : NativeTargetImageHandle{};
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

// Native image creation; no guest format, header, address, tile or surface pool.
NativeTargetImageHandle AcquireNativeTargetImage(uint64_t identity, const NativeTargetShape &shape);
void DrainNativeTargetImagesLocked(VideoState &s, uint32_t slot);
void MarkUnusedNativeTargetImagesLocked(VideoState &s, uint32_t slot);
} // namespace bd::gpu
