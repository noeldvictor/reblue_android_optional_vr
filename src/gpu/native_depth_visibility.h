/**
 * @brief Current-depth pyramid and GPU indirect visibility; no temporal history.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_occlusion.h"
#include "gpu/native_indexed_command.h"
#include "gpu/native_target_images.h"

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

// One recording's resources. The caller MUST retain this owner through its
// submission fence; never update its descriptors or reuse it for another pass.
// Runtime integration must also bound aggregate in-flight owners, not just each.
class NativeDepthVisibilityWork {
public:
  static std::unique_ptr<NativeDepthVisibilityWork> Create(plume::RenderDevice &device,
      NativeDepthVisibilityProgramHandle program, NativeTargetImageHandle depth, const NativeOcclusionView &view,
      uint32_t capacity = NativeDepthPyramid::kCommandLimit);
  // Ends outgoing rendering, samples the ORIGINAL multisample depth, then
  // restores its write layout. Caller rebinds its explicit graphics state.
  bool RecordDepth(plume::RenderCommandList &commands);
  // Appends exactly one command, preserving every field except instance_count.
  // Invalid/near-clipped bounds are conservatively emitted, not guessed hidden.
  std::optional<plume::RenderBufferReference> RecordCommand(plume::RenderCommandList &commands,
      const NativeOcclusionView &view, const std::optional<scene::NativeBounds> &bounds,
      const scene::NativeRigidIndexedCommand &draw);
  plume::RenderBuffer *Commands() const { return indirect_.get(); }
  plume::RenderBuffer *Pyramid() const { return pyramid_.get(); }
  const NativeDepthPyramid &Plan() const { return plan_; }
  uint32_t CommandCount() const { return count_; }
private:
  NativeDepthVisibilityProgramHandle program_;
  NativeTargetImageHandle depth_;
  NativeOcclusionView view_;
  NativeDepthPyramid plan_;
  std::unique_ptr<plume::RenderBuffer> pyramid_, indirect_;
  std::unique_ptr<plume::RenderDescriptorSet> depth_set_, cull_set_;
  const plume::RenderCommandList *recording_ = nullptr;
  uint32_t count_ = 0, capacity_ = 0;
};
} // namespace bd::gpu
