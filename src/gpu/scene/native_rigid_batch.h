/**
 * @brief Source-free rigid batch admission, instance packing and storage placement.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_rigid_scene.h"
#include <numeric>

namespace bd::gpu::scene {
inline constexpr uint32_t kNativeRigidBatchLimit = 256;
struct NativeRigidBatchItem {
  NativeRigidInstanceGPU input{};
  std::shared_ptr<const NativeGeometry> geometry;
  std::array<NativeTextureGpuHandle, 3> albedo;
  NativeTargetImageHandle shadow;
  std::array<const plume::RenderSampler *, 3> albedo_samplers{};
  const plume::RenderSampler *shadow_sampler = nullptr;
  const plume::RenderPipeline *pipeline = nullptr;
  const plume::RenderPipelineLayout *layout = nullptr;
  plume::RenderFramebuffer *framebuffer = nullptr;
  plume::RenderViewport viewport{};
  plume::RenderRect scissor{};
  uint32_t frame = ~0u, slot = ~0u, view = ~0u;
  uint64_t model_generation = 0, instance = 0; // Host lifetime metadata, not shader ABI.
  bool regression = false; // Selected asset's reload window, not family admission.
  bool Ready(uint32_t expected_frame, uint32_t expected_slot) const {
    if (view == 1) {
      const auto &flags = input.object_data.flags;
      const bool textured = (flags.x & RigidAlbedo) != 0;
      if (flags.y != uint32_t(textured) || textured != bool(flags.x & RigidCutout) ||
          bool(albedo[0]) != textured || bool(albedo_samplers[0]) != textured ||
          albedo[1] || albedo[2] || albedo_samplers[1] || albedo_samplers[2] || shadow || shadow_sampler) return false;
    }
    if (view == 3) {
      const auto layers = input.object_data.flags.y;
      if (layers > 3 || bool(input.object_data.flags.x & RigidAlbedo) != (layers != 0)) return false;
      for (uint32_t n = 0; n < 3; ++n)
        if (!albedo_samplers[n] || (n < layers && !albedo[n]) || (n >= layers && albedo[n])) return false;
    }
    return frame == expected_frame && slot == expected_slot && model_generation && instance && geometry && pipeline && layout && framebuffer &&
        (view == 1 || (view == 3 && shadow && shadow_sampler));
  }
};
inline bool SameNativeRigidBatch(const NativeRigidBatchItem &a, const NativeRigidBatchItem &b) {
  return a.frame == b.frame && a.slot == b.slot && a.view == b.view && a.model_generation == b.model_generation &&
      a.regression == b.regression && a.geometry == b.geometry && a.pipeline == b.pipeline && a.layout == b.layout && a.framebuffer == b.framebuffer &&
      a.albedo == b.albedo && a.shadow == b.shadow && a.albedo_samplers == b.albedo_samplers && a.shadow_sampler == b.shadow_sampler &&
      a.viewport.x == b.viewport.x && a.viewport.y == b.viewport.y &&
      a.viewport.width == b.viewport.width && a.viewport.height == b.viewport.height &&
      a.viewport.minDepth == b.viewport.minDepth && a.viewport.maxDepth == b.viewport.maxDepth &&
      a.scissor.left == b.scissor.left && a.scissor.top == b.scissor.top &&
      a.scissor.right == b.scissor.right && a.scissor.bottom == b.scissor.bottom;
}
// Only a consecutive compatible prefix. A non-native/ordered draw is a barrier;
// the shared queue's existing safe reorder runs may bring compatible nodes closer.
inline uint32_t NativeRigidBatchLength(std::span<const NativeRigidBatchItem *const> items,
                                     uint32_t frame, uint32_t slot) {
  if (items.empty() || !items[0] || !items[0]->Ready(frame,slot)) return 0;
  uint32_t count = 1;
  while (count < items.size() && count < kNativeRigidBatchLimit && items[count] &&
         items[count]->Ready(frame,slot) && SameNativeRigidBatch(*items[0],*items[count])) ++count;
  return count;
}
// Preflight all entries before copying any output. CPU packets, never GPU mapped
// upload memory, remain the authoritative source during gathering.
inline bool PackNativeRigidBatch(std::span<const NativeRigidBatchItem *const> items,
                                std::span<NativeRigidInstanceGPU> output, uint32_t frame, uint32_t slot) {
  if (items.empty() || output.size() != items.size() || NativeRigidBatchLength(items,frame,slot) != items.size()) return false;
  for (size_t n=0;n<items.size();++n) output[n] = items[n]->input;
  return true;
}
// Structured-view offsets are ELEMENTS, Vulkan alignment is BYTES. The upload
// arena accepts power-of-two alignment; reserve enough headroom for both without
// padding every GPU record or making a separate buffer per draw.
struct NativeRigidStoragePlan {
  uint32_t bytes, quantum, reserve;
  uint64_t Offset(uint64_t allocation_offset) const {
    return (allocation_offset+quantum-1)/quantum*quantum;
  }
};
inline std::optional<NativeRigidStoragePlan> PlanNativeRigidStorage(uint32_t count, uint32_t alignment) {
  if (!count || count > kNativeRigidBatchLimit || !alignment || (alignment & (alignment-1)) || alignment > 65536) return {};
  const uint32_t quantum = std::lcm(uint32_t(sizeof(NativeRigidInstanceGPU)),alignment);
  if (quantum > 65536) return {};
  const auto bytes = uint32_t(count*sizeof(NativeRigidInstanceGPU));
  return NativeRigidStoragePlan{bytes,quantum,bytes+quantum-1};
}
struct NativeRigidIndexedCommand {
  uint32_t index_count, instance_count, first_index;
  int32_t base_vertex;
  uint32_t first_instance;
};
static_assert(sizeof(NativeRigidIndexedCommand) == 20 && offsetof(NativeRigidIndexedCommand, first_instance) == 16);
} // namespace bd::gpu::scene
