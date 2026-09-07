/**
 * @brief Native depth-query pipeline shared by runtime and GPU fixtures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_target_images.h"
#include "gpu/native_occlusion.h"

namespace bd::gpu {
struct NativeOcclusionProgram {
  std::shared_ptr<plume::RenderPipelineLayout> layout;
  std::shared_ptr<plume::RenderShader> vertex, pixel;
  explicit operator bool() const { return layout && vertex && pixel; }
};
NativeOcclusionProgram CreateNativeOcclusionProgram(plume::RenderDevice &device);
std::unique_ptr<plume::RenderPipeline> CreateNativeOcclusionPipeline(
    plume::RenderDevice &device, const NativeOcclusionProgram &program, const NativeTargetShape &shape);
} // namespace bd::gpu
