/**
 * @brief Native scene image creation, sampling descriptors and fenced residency.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/native_target_images.h"
#include "gpu/device.h"
#include "gpu/bindless_allocator.h"
#include "core/logging.h"
#if !defined(REBLUE_D3D12)
#include <plume_vulkan.h>
#endif

namespace bd::gpu {
namespace {
NativeTargetImageHandle Create(VideoState &s, const NativeTargetShape &shape) {
  auto result = std::make_shared<NativeTargetImage>();
  result->shape = shape;
  const bool depth = shape.format == plume::RenderFormat::D32_FLOAT_S8_UINT;
  auto desc = depth ? plume::RenderTextureDesc::DepthTarget(shape.width, shape.height, shape.format)
      : plume::RenderTextureDesc::ColorTarget(shape.width, shape.height, shape.format);
  desc.arraySize = shape.layers;
  desc.multisampling.sampleCount = static_cast<plume::RenderSampleCounts>(shape.samples);
  result->image = CreateHostTexture(s.device.get(), desc,
      depth ? "native-scene-source-depth" : "native-scene-source-color");
  if (!result->image) return {};
  plume::RenderTextureViewDesc view;
  view.format = shape.format;
  view.dimension = plume::RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  view.mipLevels = 1;
  view.arraySize = shape.layers;
  result->view = result->image->createTextureView(view);
  if (!result->view) return {};
#if !defined(REBLUE_D3D12)
  if (static_cast<plume::VulkanTextureView *>(result->view.get())->vk == VK_NULL_HANDLE) return {};
#endif
  result->descriptor = AllocateSlot(s);
  if (result->descriptor == kInvalidDescriptorIndex) return {};
  WriteTextureDescriptor(s, result->descriptor, result->image.get(), result->view.get());
  return result;
}
} // namespace

NativeTargetImageHandle AcquireNativeTargetImage(uint64_t identity, const NativeTargetShape &shape) {
  if (!identity || !shape.Bytes(512ull << 20)) return {};
  auto &s = state();
  std::lock_guard lock(s.mutex);
  if (!s.ready || s.shutting_down.load() ||
      (shape.layers == 2 && !s.device->getCapabilities().multiview) ||
      !(s.device->getSampleCountsSupported(shape.format) & shape.samples)) return {};
  if (!s.native_target_images) s.native_target_images = std::make_shared<NativeTargetImageStore>();
  auto result = s.native_target_images->Acquire(identity, shape, [&] { return Create(s, shape); });
  const auto stats = s.native_target_images->Stats();
  if (!result || stats.created + stats.reused == 1 || (stats.created + stats.reused) % 600 == 0)
    BD_INFO("[native-target-images] {} created {} reused {} retired {} resident {} payload bytes; "
            "{} refused {} failed; native source allocation and ownership, no surface pool",
        stats.created, stats.reused, stats.retired, stats.resident, stats.bytes, stats.refused, stats.failed);
  return result;
}
void DrainNativeTargetImagesLocked(VideoState &s, uint32_t slot) {
  if (!s.native_target_images) return;
  s.native_target_images->AfterFence(slot, [&](const NativeTargetImage &image) {
    WriteTextureDescriptor(s, image.descriptor, s.null_textures[kNullTexture2DDescriptorIndex].get(),
        s.null_texture_views[kNullTexture2DDescriptorIndex].get());
    BindlessFreeSlot(s.descriptor_slot_used, image.descriptor, kNullTextureDescriptorCount);
  });
}
void MarkUnusedNativeTargetImagesLocked(VideoState &s, uint32_t slot) {
  if (s.native_target_images) s.native_target_images->MarkUnused(slot);
}
} // namespace bd::gpu
