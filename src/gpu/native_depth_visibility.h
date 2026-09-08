/**
 * @brief Current-depth pyramid and GPU indirect visibility; no temporal history.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_occlusion.h"
#include "gpu/native_indexed_command.h"
#include "gpu/native_target_images.h"
#include <mutex>
#include <span>

namespace bd::gpu {
struct NativeDepthPyramid {
  struct Level { uint32_t width, height, offset; };
  static constexpr uint32_t kByteLimit = 16u << 20, kCommandLimit = 4096, kCommandStride = 32;
  std::array<Level, 14> levels{};
  uint32_t count = 0, bytes = 0;
  static std::optional<NativeDepthPyramid> Plan(const NativeTargetShape &shape) {
    if (shape.layers != 1 || shape.format != plume::RenderFormat::D32_FLOAT_S8_UINT ||
        !shape.Bytes(512ull << 20) || shape.width > 8192 || shape.height > 8192) return {};
    NativeDepthPyramid result;
    uint32_t width = (shape.width+1)/2, height = (shape.height+1)/2;
    for (;;) {
      const uint64_t bytes = uint64_t(width)*height*sizeof(float);
      if (result.count == result.levels.size() || bytes > kByteLimit-result.bytes) return {};
      result.levels[result.count++] = {width,height,result.bytes/uint32_t(sizeof(float))};
      result.bytes += uint32_t(bytes);
      if (width == 1 && height == 1) return result;
      width = (width+1)/2; height = (height+1)/2;
    }
  }
};
struct NativeDepthVisibilityProgram;
using NativeDepthVisibilityProgramHandle = std::shared_ptr<const NativeDepthVisibilityProgram>;
NativeDepthVisibilityProgramHandle CreateNativeDepthVisibilityProgram(plume::RenderDevice &device);

// Shared by ALL in-flight frame slots of a renderer. Counts requested buffer
// bytes (including readback), not driver allocation granularity or owned images.
// Tighter caller limits are allowed; no factory can silently create its own budget.
class NativeDepthVisibilityBudget {
public:
  struct Usage { uint64_t bytes; uint32_t owners; };
  explicit NativeDepthVisibilityBudget(uint64_t bytes = 64ull << 20, uint32_t owners = 16)
      : byte_limit_((std::min)(bytes,uint64_t{64} << 20)), owner_limit_((std::min)(owners,16u)) {}
  Usage Used() const { std::lock_guard lock(mutex_); return {bytes_,owners_}; }
private:
  friend class NativeDepthVisibilityWork;
  bool Acquire(uint64_t bytes) {
    std::lock_guard lock(mutex_);
    if (!bytes || owners_ >= owner_limit_ || bytes > byte_limit_-bytes_) return false;
    bytes_ += bytes; ++owners_; return true;
  }
  void Release(uint64_t bytes) { std::lock_guard lock(mutex_); bytes_ -= bytes; --owners_; }
  mutable std::mutex mutex_;
  uint64_t byte_limit_, bytes_ = 0;
  uint32_t owner_limit_, owners_ = 0;
};
struct NativeDepthVisibilityReceipt {
  uint32_t requested_instances, generated_instances;
  bool draw_recorded;
  uint32_t EmittedInstances() const { return draw_recorded ? generated_instances : 0; }
};

// One command recording's resources. Caller MUST retain this owner through its
// submission fence. Descriptors never change. Ordered depth refreshes reuse its
// scratch pyramid; previously generated commands retain independent output slots.
class NativeDepthVisibilityWork {
public:
  static std::unique_ptr<NativeDepthVisibilityWork> Create(plume::RenderDevice &device,
      NativeDepthVisibilityProgramHandle program, NativeTargetImageHandle depth, const NativeOcclusionView &view,
      std::shared_ptr<NativeDepthVisibilityBudget> budget, uint32_t capacity = NativeDepthPyramid::kCommandLimit);
  // Ends outgoing rendering, samples the ORIGINAL multisample depth, then
  // restores its write layout. Caller rebinds its explicit graphics state.
  bool RecordDepth(plume::RenderCommandList &commands);
  // Same image/frame/recording, but fresh current depth and possibly new camera.
  // Rebuild after intervening clears/non-monotonic writes; never reuse stale depth.
  bool RefreshDepth(plume::RenderCommandList &commands, const NativeOcclusionView &view);
  // Appends exactly one command, preserving every field except instance_count.
  // Invalid/near-clipped bounds are conservatively emitted, not guessed hidden.
  std::optional<plume::RenderBufferReference> RecordCommand(plume::RenderCommandList &commands,
      const NativeOcclusionView &view, const std::optional<scene::NativeBounds> &bounds,
      const scene::NativeRigidIndexedCommand &draw);
  // Bind graphics state first. Records the real draw exactly once; generation
  // alone is not emission. No CPU readback participates in visibility decisions.
  bool DrawCommand(plume::RenderCommandList &commands, uint32_t index);
  bool Seal(plume::RenderCommandList &commands);
  // Caller must have completed this recording's GPU fence. No wait is added;
  // late receipts are accounting only, never a next-frame culling input.
  std::optional<std::span<const NativeDepthVisibilityReceipt>> CollectAfterFence();
  plume::RenderBuffer *Commands() const { return indirect_.get(); }
  plume::RenderBuffer *Pyramid() const { return pyramid_.get(); }
  const NativeDepthPyramid &Plan() const { return plan_; }
  uint32_t CommandCount() const { return count_; }
  uint32_t SnapshotCount() const { return snapshots_; }
private:
  // Declared first, destroyed last: GPU resources die before budget release.
  struct Reservation {
    Reservation() = default;
    Reservation(const Reservation &) = delete;
    Reservation &operator=(const Reservation &) = delete;
    std::shared_ptr<NativeDepthVisibilityBudget> budget;
    uint64_t bytes = 0;
    ~Reservation() { if (budget) budget->Release(bytes); }
  } reservation_;
  bool BuildDepth(plume::RenderCommandList &commands);
  NativeDepthVisibilityProgramHandle program_;
  NativeTargetImageHandle depth_;
  NativeOcclusionView view_;
  NativeDepthPyramid plan_;
  std::unique_ptr<plume::RenderBuffer> pyramid_, indirect_, readback_;
  std::unique_ptr<plume::RenderDescriptorSet> depth_set_, cull_set_;
  const plume::RenderCommandList *recording_ = nullptr;
  std::vector<scene::NativeRigidIndexedCommand> expected_;
  std::vector<NativeDepthVisibilityReceipt> receipts_;
  uint32_t count_ = 0, capacity_ = 0, snapshots_ = 0;
  bool sealed_ = false, collected_ = false;
};
} // namespace bd::gpu
