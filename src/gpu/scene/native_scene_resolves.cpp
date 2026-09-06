/**
 * @brief Bounded native scene resolve residency and attachment dependencies.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_scene_resolves.h"
#include "gpu/scene/fenced_asset_cache.h"
#include "gpu/device.h"
#include "gpu/bindless_allocator.h"
#include "gpu/format.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#if !defined(REBLUE_D3D12)
#include <plume_vulkan.h>
#endif

namespace bd::gpu::scene {
constexpr uint64_t kSceneResolveBudget = 512ull << 20;
struct NativeSceneResolveStore {
  FencedAssetCache<NativeSceneResolves> images{kSceneResolveBudget, 64};
  struct Entry {
    SceneResolveSources sources;
    uint64_t id = 0;
    std::weak_ptr<const NativeSceneResolves> handle;
  };
  std::vector<Entry> entries;
  uint64_t next_id = 0, requests = 0;
};

namespace {
NativeSceneResolveHandle Create(VideoState &s, const SceneResolveSources &sources,
    const std::array<NativeTargetImageHandle, 2> &source_owners) {
  auto result = std::make_shared<NativeSceneResolves>();
  result->sources = sources;
  result->source_owners = source_owners;
  auto color = plume::RenderTextureDesc::ColorTarget(sources.width, sources.height, sources.color_format);
  auto depth = plume::RenderTextureDesc::DepthTarget(sources.width, sources.height, sources.depth_format);
  color.arraySize = depth.arraySize = sources.layers;
  result->images[0] = CreateHostTexture(s.device.get(), color, "native-scene-resolved-color");
  result->images[1] = CreateHostTexture(s.device.get(), depth, "native-scene-resolved-depth");
  for (uint32_t i = 0; i < 2; ++i) {
    if (!result->images[i]) return {};
    plume::RenderTextureViewDesc view;
    view.format = i ? sources.depth_format : sources.color_format;
    view.dimension = plume::RenderTextureViewDimension::TEXTURE_2D_ARRAY;
    view.mipLevels = 1;
    view.arraySize = sources.layers;
    result->views[i] = result->images[i]->createTextureView(view);
    if (!result->views[i]) return {};
#if !defined(REBLUE_D3D12)
    // Plume can return a wrapper after native view creation failed. Never
    // publish a descriptor pointing at that null-backed sampling view.
    if (static_cast<plume::VulkanTextureView *>(result->views[i].get())->vk == VK_NULL_HANDLE)
      return {};
#endif
  }
  const plume::RenderTexture *colors[]{sources.color};
  const plume::RenderTexture *resolves[]{result->images[0].get()};
  plume::RenderFramebufferDesc fb;
  fb.colorAttachments = colors;
  fb.colorAttachmentsCount = 1;
  fb.depthAttachment = sources.depth;
  fb.colorResolveAttachments = resolves;
  fb.depthResolveAttachment = result->images[1].get();
  fb.depthResolveMode = plume::RenderResolveMode::MIN;
  fb.stencilResolveMode = plume::RenderResolveMode::NONE;
  fb.viewMask = sources.layers == 2 ? 3u : 0u;
  fb.fragmentDensityMap = sources.density_map;
  result->framebuffer = s.device->createFramebuffer(fb);
  if (!result->framebuffer) return {};
  // Allocate both slots before publishing either descriptor; allocation failure
  // cannot leave a live heap entry naming a destroyed private image.
  for (auto &slot : result->descriptors) {
    slot = AllocateSlot(s);
    if (slot == kInvalidDescriptorIndex) {
      for (const auto allocated : result->descriptors)
        if (allocated != kInvalidDescriptorIndex)
          BindlessFreeSlot(s.descriptor_slot_used, allocated, kNullTextureDescriptorCount);
      return {};
    }
  }
  for (uint32_t i = 0; i < 2; ++i)
    WriteTextureDescriptor(s, result->descriptors[i], result->images[i].get(), result->views[i].get());
  return result;
}
void Transition(VideoState &s, const NativeSceneResolves &images, bool reading) {
  std::array<plume::RenderTextureBarrier, 2> barriers;
  uint32_t count = 0;
  for (uint32_t i = 0; i < 2; ++i) {
    const auto wanted = reading ? plume::RenderTextureLayout::SHADER_READ
        : i ? plume::RenderTextureLayout::DEPTH_WRITE : plume::RenderTextureLayout::COLOR_WRITE;
    if (images.layouts[i] != wanted) {
      barriers[count++] = plume::RenderTextureBarrier(images.images[i].get(), wanted);
      images.layouts[i] = wanted;
    }
  }
  if (count) {
    s.command_list->barriers(plume::RenderBarrierStage::GRAPHICS, barriers.data(), count);
    NoteBarrierCall(count, BarrierSite::Resolve);
  }
}
} // namespace

NativeSceneResolveHandle AcquireNativeSceneResolves(const SceneResolveSources &sources,
    const std::array<NativeTargetImageHandle, 2> &source_owners) {
  if (!source_owners[0] || !source_owners[1] ||
      source_owners[0]->image.get() != sources.color || source_owners[1]->image.get() != sources.depth)
    return {};
  if (!sources.color || !sources.depth || sources.color == sources.depth ||
      !sources.color_identity || !sources.depth_identity || !sources.width || !sources.height ||
      !sources.layers || sources.layers > 2 || sources.samples < 2 || !std::has_single_bit(sources.samples) ||
      sources.color_format == plume::RenderFormat::UNKNOWN || IsDepthFormat(sources.color_format) ||
      !IsDepthFormat(sources.depth_format)) return {};
  const uint64_t stride = uint64_t(plume::RenderFormatSize(sources.color_format) +
      plume::RenderFormatSize(sources.depth_format)) * sources.layers;
  const uint64_t pixels = uint64_t(sources.width) * sources.height;
  if (!stride || pixels > kSceneResolveBudget / stride) return {};
  const uint64_t bytes = pixels * stride;
  auto &s = state();
  std::lock_guard lock(s.mutex);
  if (!s.ready || s.shutting_down.load()) return {};
  const auto &caps = s.device->getCapabilities();
  if (!caps.attachmentResolve || !(caps.depthAttachmentResolveModes &
      (1u << uint32_t(plume::RenderResolveMode::MIN))) || (sources.layers == 2 && !caps.multiview))
    return {};
  if (!s.native_scene_resolves) s.native_scene_resolves = std::make_shared<NativeSceneResolveStore>();
  auto &store = *s.native_scene_resolves;
  std::erase_if(store.entries, [](const auto &entry) { return entry.handle.expired(); });
  auto it = std::find_if(store.entries.begin(), store.entries.end(),
      [&](const auto &entry) { return entry.sources == sources; });
  const bool existing = it != store.entries.end();
  const auto id = existing ? it->id : ++store.next_id;
  auto result = store.images.Acquire(id, bytes, [&] { return Create(s, sources, source_owners); });
  if (result && !existing) store.entries.push_back({sources, id, result});
  if (!result || ++store.requests == 1 || store.requests % 600 == 0) {
    const auto stats = store.images.Stats();
    BD_INFO("[native-scene-resolves] {} created {} reused {} retired {} resident {} payload bytes; "
            "{} refused {} failed; native attachment images, no guest allocation",
        stats.created, stats.reused, stats.retired, stats.resident, stats.bytes, stats.refused, stats.failed);
  }
  return result;
}
void FinishNativeSceneResolves(VideoState &s, const NativeSceneResolves &images) {
  s.command_list->setFramebuffer(nullptr);
  // Reading resolve destinations also flushes a held zero-draw source clear.
  Transition(s, images, true);
}
void DrainNativeSceneResolvesLocked(VideoState &s, uint32_t slot) {
  if (!s.native_scene_resolves) return;
  s.native_scene_resolves->images.AfterFence(slot, [&](const NativeSceneResolves &images) {
    for (const auto descriptor : images.descriptors) {
      WriteTextureDescriptor(s, descriptor, s.null_textures[kNullTexture2DDescriptorIndex].get(),
          s.null_texture_views[kNullTexture2DDescriptorIndex].get());
      BindlessFreeSlot(s.descriptor_slot_used, descriptor, kNullTextureDescriptorCount);
    }
  });
}
void MarkUnusedNativeSceneResolvesLocked(VideoState &s, uint32_t slot) {
  if (s.native_scene_resolves) s.native_scene_resolves->images.MarkUnused(slot);
}
} // namespace bd::gpu::scene
