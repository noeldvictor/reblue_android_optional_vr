/**
 * @brief Source-free rigid batch admission, instance packing and storage placement.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_rigid_scene.h"
#include "gpu/scene/native_water_scene.h"
#include "gpu/native_indexed_command.h"
#include "gpu/scene/native_rigid_lifecycle.h"
#include "gpu/scene/native_skin_batch.h"
#include <numeric>

namespace bd::gpu::scene {
inline constexpr uint32_t kNativeRigidBatchLimit = 256;
struct NativeRigidBatchItem {
  NativeRigidInstanceGPU input{};
  // Water reuses this queue/store/fence path with its own larger GPU record.
  // Ordinary draws keep the existing ABI and allocate no water payload.
  std::shared_ptr<const NativeWaterBatchData> water;
  std::shared_ptr<const NativeInstancePose> skin_pose;
  std::shared_ptr<const NativeGeometry> geometry;
  std::array<NativeTextureGpuHandle, 3> albedo;
  NativeTargetImageHandle shadow;
  NativeTargetImageHandle scene_depth;
  std::optional<NativeBounds> world_bounds;
  mutable NativeRigidOutputReceipt output;
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
  const NativeRigidPassGPU &Pass() const { return water ? water->input.pass_data : input.pass_data; }
  bool Cutout() const { return !water && (input.object_data.flags.x & RigidCutout); }
  bool Ready(uint32_t expected_frame, uint32_t expected_slot) const {
    if (skin_pose && (water || (view != 1 && view != 3) || !geometry ||
        (view == 1 && (!geometry->skin_shadow_vertex_input || (Cutout() && !geometry->skin_shadow_cutout_vertex_input))) ||
        (view == 3 && !(input.object_data.flags.y == 3 ? geometry->skin_scene_layered_vertex_input : geometry->skin_scene_vertex_input)) ||
        !geometry->skin_influences || geometry->skin_bounds.empty() || !world_bounds || !world_bounds->Valid() ||
        skin_pose->instance != instance || skin_pose->model_generation != model_generation ||
        !skin_pose->model || skin_pose->model->Generation() != model_generation ||
        skin_pose->transforms.empty() || skin_pose->transforms.size() > NativeInstanceRegistry::kMaxTransforms ||
        geometry->skin_bounds.back().joint >= skin_pose->transforms.size())) return false;
    if (geometry && geometry->skin_influences && !skin_pose) return false;
    if (water) {
      return view == 3 && frame == expected_frame && slot == expected_slot && model_generation && instance &&
          !regression && geometry && geometry->water_vertex_input && geometry->canonical_vertices &&
          world_bounds && world_bounds->Valid() && pipeline && layout && framebuffer && scene_depth &&
          !shadow && !shadow_sampler && water->Ready() &&
          std::none_of(albedo.begin(),albedo.end(),[](const auto &image) { return bool(image); }) &&
          std::none_of(albedo_samplers.begin(),albedo_samplers.end(),[](const auto *sampler) { return sampler != nullptr; });
    }
    if (view == 1) {
      const auto &flags = input.object_data.flags;
      const bool textured = (flags.x & RigidAlbedo) != 0;
      if (flags.y != uint32_t(textured) || textured != bool(flags.x & RigidCutout) ||
          bool(albedo[0]) != textured || bool(albedo_samplers[0]) != textured ||
          albedo[1] || albedo[2] || albedo_samplers[1] || albedo_samplers[2] || shadow || shadow_sampler || scene_depth) return false;
    }
    if (view == 3) {
      const auto layers = input.object_data.flags.y;
      if (layers > 3 || bool(input.object_data.flags.x & RigidAlbedo) != (layers != 0)) return false;
      for (uint32_t n = 0; n < 3; ++n)
        if (!albedo_samplers[n] || (n < layers && !albedo[n]) || (n >= layers && albedo[n])) return false;
    }
    return frame == expected_frame && slot == expected_slot && model_generation && instance && geometry && pipeline && layout && framebuffer &&
        (view == 1 || (view == 3 && shadow && shadow_sampler && scene_depth));
  }
};
inline bool SameNativeRigidBatch(const NativeRigidBatchItem &a, const NativeRigidBatchItem &b) {
  return bool(a.skin_pose) == bool(b.skin_pose) && bool(a.water) == bool(b.water) && (!a.water ||
      (a.water->images == b.water->images && a.water->samplers == b.water->samplers)) &&
      a.frame == b.frame && a.slot == b.slot && a.view == b.view && a.model_generation == b.model_generation &&
      a.regression == b.regression && a.geometry == b.geometry && a.pipeline == b.pipeline && a.layout == b.layout && a.framebuffer == b.framebuffer &&
      a.albedo == b.albedo && a.shadow == b.shadow && a.albedo_samplers == b.albedo_samplers && a.shadow_sampler == b.shadow_sampler &&
      a.scene_depth == b.scene_depth && std::bit_cast<RenderMatrix>(a.Pass().world_to_clip[0]) ==
          std::bit_cast<RenderMatrix>(b.Pass().world_to_clip[0]) &&
      std::bit_cast<RenderMatrix>(a.Pass().world_to_clip[1]) == std::bit_cast<RenderMatrix>(b.Pass().world_to_clip[1]) &&
      a.viewport.x == b.viewport.x && a.viewport.y == b.viewport.y &&
      a.viewport.width == b.viewport.width && a.viewport.height == b.viewport.height &&
      a.viewport.minDepth == b.viewport.minDepth && a.viewport.maxDepth == b.viewport.maxDepth &&
      a.scissor.left == b.scissor.left && a.scissor.top == b.scissor.top &&
      a.scissor.right == b.scissor.right && a.scissor.bottom == b.scissor.bottom;
}
// Batch visibility is all-or-nothing. Any uncertain sibling keeps the entire
// instanced command visible; never cull based only on the first instance.
inline std::optional<NativeBounds> NativeRigidBatchBounds(std::span<const NativeRigidBatchItem *const> items) {
  std::optional<NativeBounds> bounds;
  for (const auto *item : items) {
    if (!item || !item->world_bounds || !item->world_bounds->Valid()) return {};
    if (!bounds) bounds = item->world_bounds;
    else for (uint32_t axis=0;axis<3;++axis) {
      bounds->min[axis] = (std::min)(bounds->min[axis],item->world_bounds->min[axis]);
      bounds->max[axis] = (std::max)(bounds->max[axis],item->world_bounds->max[axis]);
    }
  }
  return bounds;
}
// Only a consecutive compatible prefix. A non-native/ordered draw is a barrier;
// the shared queue's existing safe reorder runs may bring compatible nodes closer.
inline uint32_t NativeRigidBatchLength(std::span<const NativeRigidBatchItem *const> items,
                                     uint32_t frame, uint32_t slot) {
  if (items.empty() || !items[0] || !items[0]->Ready(frame,slot)) return 0;
  uint32_t count = 1;
  std::array<const NativeInstancePose *,kNativeRigidBatchLimit> poses{};
  uint32_t pose_count = 0, matrices = 0;
  if (items[0]->skin_pose) {
    poses[pose_count++] = items[0]->skin_pose.get();
    matrices = uint32_t(items[0]->skin_pose->transforms.size());
    if (matrices > kNativeSkinPaletteMatrices) return 0;
  }
  while (count < items.size() && count < kNativeRigidBatchLimit && items[count] &&
         items[count]->Ready(frame,slot) && SameNativeRigidBatch(*items[0],*items[count])) {
    const auto &pose = items[count]->skin_pose;
    if (pose && std::find(poses.begin(),poses.begin()+pose_count,pose.get()) == poses.begin()+pose_count) {
      if (pose->transforms.size() > kNativeSkinPaletteMatrices-matrices) break;
      matrices += uint32_t(pose->transforms.size()); poses[pose_count++] = pose.get();
    }
    ++count;
  }
  return count;
}
// Preflight all entries before copying any output. CPU packets, never GPU mapped
// upload memory, remain the authoritative source during gathering.
inline bool PackNativeRigidBatch(std::span<const NativeRigidBatchItem *const> items,
                                std::span<NativeRigidInstanceGPU> output, uint32_t frame, uint32_t slot) {
  if (items.empty() || output.size() != items.size() || NativeRigidBatchLength(items,frame,slot) != items.size()) return false;
  if (items[0]->water) return false;
  for (size_t n=0;n<items.size();++n) output[n] = items[n]->input;
  return true;
}
inline bool PackNativeWaterBatch(std::span<const NativeRigidBatchItem *const> items,
                                std::span<NativeWaterInstanceGPU> output, uint32_t frame, uint32_t slot) {
  if (items.empty() || output.size() != items.size() || NativeRigidBatchLength(items,frame,slot) != items.size() ||
      !items[0]->water) return false;
  for (size_t n=0;n<items.size();++n) output[n] = items[n]->water->input;
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
inline std::optional<NativeRigidStoragePlan> PlanNativeRigidStorage(uint32_t count, uint32_t alignment,
    uint32_t stride = sizeof(NativeRigidInstanceGPU)) {
  if (!count || count > kNativeRigidBatchLimit || !alignment || (alignment & (alignment-1)) || alignment > 65536) return {};
  if (stride != sizeof(NativeRigidInstanceGPU) && stride != sizeof(NativeWaterInstanceGPU)) return {};
  const uint32_t quantum = std::lcm(stride,alignment);
  if (quantum > 65536) return {};
  const auto bytes = count*stride;
  return NativeRigidStoragePlan{bytes,quantum,bytes+quantum-1};
}
} // namespace bd::gpu::scene
