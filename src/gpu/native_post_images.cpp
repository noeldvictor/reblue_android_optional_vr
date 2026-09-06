/**
 * @brief Native post image allocation, descriptors and fence-gated retirement.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/native_post_images.h"
#include "gpu/device.h"
#include "gpu/bindless_allocator.h"
#include "gpu/foveation.h"
#include "core/logging.h"
#if !defined(REBLUE_D3D12)
#include <plume_vulkan.h>
#endif

namespace bd::gpu {
namespace {
NativePostImageHandle Create(VideoState &s, const NativePostRecipe &recipe) {
  auto result = std::make_shared<NativePostImage>();
  result->recipe = recipe;
  auto desc = plume::RenderTextureDesc::ColorTarget(recipe.width, recipe.height,
      plume::RenderFormat::R16G16B16A16_FLOAT);
  desc.arraySize = recipe.layers;
  result->image = CreateHostTexture(s.device.get(), desc, "native-post-output");
  if (!result->image) return {};
  plume::RenderTextureViewDesc view;
  view.format = desc.format;
  view.dimension = plume::RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  view.mipLevels = 1;
  view.arraySize = recipe.layers;
  result->view = result->image->createTextureView(view);
  if (!result->view) return {};
#if !defined(REBLUE_D3D12)
  if (static_cast<plume::VulkanTextureView *>(result->view.get())->vk == VK_NULL_HANDLE) return {};
#endif
  const plume::RenderTexture *colors[]{result->image.get()};
  plume::RenderFramebufferDesc fb;
  fb.colorAttachments = colors;
  fb.colorAttachmentsCount = 1;
  fb.viewMask = recipe.layers == 2 ? 3u : 0u;
  fb.fragmentDensityMap = recipe.density_map;
  result->framebuffer = s.device->createFramebuffer(fb);
  if (!result->framebuffer) return {};
#if !defined(REBLUE_D3D12)
  if (static_cast<plume::VulkanFramebuffer *>(result->framebuffer.get())->vk == VK_NULL_HANDLE) return {};
#endif
  result->descriptor = AllocateSlot(s);
  if (result->descriptor == kInvalidDescriptorIndex) return {};
  // Last fallible operation precedes publication of the descriptor.
  WriteTextureDescriptor(s, result->descriptor, result->image.get(), result->view.get());
  return result;
}
} // namespace

NativePostImageHandle AcquireNativePostImage(uint32_t width, uint32_t height, uint32_t layers) {
#if defined(REBLUE_D3D12)
  return {};
#else
  auto &s = state();
  std::lock_guard lock(s.mutex);
  if (!s.ready || s.shutting_down.load() || !width || !height || !layers || layers > 2 ||
      (layers == 2 && !s.device->getCapabilities().multiview)) return {};
  FoveationEnsure(width, height, layers);
  const NativePostRecipe recipe{width, height, layers,
      FoveationWanted(width, height, layers) ? FoveationMapFor(width, height) : nullptr};
  if (!s.native_post_images) s.native_post_images = std::make_shared<NativePostImagePool>();
  auto result = s.native_post_images->Acquire(recipe, [&] { return Create(s, recipe); });
  const auto stats = s.native_post_images->Stats();
  if (!result || stats.created + stats.reused == 1 || (stats.created + stats.reused) % 600 == 0)
    BD_INFO("[native-post-images] {} created {} reused {} retired {} resident {} payload bytes; "
            "{} refused {} failed; native output ownership, no resource-header allocation",
        stats.created, stats.reused, stats.retired, stats.resident, stats.bytes, stats.refused, stats.failed);
  return result;
#endif
}
void DrainNativePostImagesLocked(VideoState &s, uint32_t slot) {
  if (!s.native_post_images) return;
  s.native_post_images->AfterFence(slot, [&](const NativePostImage &image) {
    WriteTextureDescriptor(s, image.descriptor, s.null_textures[kNullTexture2DDescriptorIndex].get(),
        s.null_texture_views[kNullTexture2DDescriptorIndex].get());
    BindlessFreeSlot(s.descriptor_slot_used, image.descriptor, kNullTextureDescriptorCount);
  });
}
void MarkUnusedNativePostImagesLocked(VideoState &s, uint32_t slot) {
  if (s.native_post_images) s.native_post_images->MarkUnused(slot);
}
} // namespace bd::gpu
