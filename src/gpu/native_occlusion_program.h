/**
 * @brief Native depth-query pipeline shared by runtime and GPU fixtures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_target_images.h"
#include "gpu/native_occlusion.h"
#include "gpu/draw_bindings.h"

namespace bd::gpu {
struct NativeOcclusionProgram {
  std::shared_ptr<plume::RenderPipelineLayout> layout;
  std::shared_ptr<plume::RenderShader> vertex, pixel;
  explicit operator bool() const { return layout && vertex && pixel; }
};
NativeOcclusionProgram CreateNativeOcclusionProgram(plume::RenderDevice &device);
// The caller supplies its next consumer's explicit bindings. Query inputs stay
// native; restoring an interop consumer must not reuse the push-only layout.
template <typename Commands, typename Emit>
bool WithNativeOcclusionBindings(Commands &commands, const plume::RenderPipelineLayout *layout,
                                const GraphicsBindings &resume, Emit &&emit) {
  if (!layout || !resume.Valid()) return false;
  const auto restore = [&] {
    GraphicsBindingState bindings;
    return ApplyGraphicsBindings(commands, resume, bindings);
  };
  commands.setGraphicsPipelineLayout(layout);
  try { emit(); }
  catch (...) { restore(); throw; }
  return restore();
}
std::unique_ptr<plume::RenderPipeline> CreateNativeOcclusionPipeline(
    plume::RenderDevice &device, const NativeOcclusionProgram &program, const NativeTargetShape &shape);
} // namespace bd::gpu
