/**
 * @brief   Native scene-colour snapshot timing and explicit image copies.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_scene_commands.h"
namespace bd::gpu::scene {
enum class SceneSnapshotPhase { Inactive, Scene, Reflection };
struct SceneSnapshotPlan {
  bool bind = false, refresh = false, publish_cache = false;
  bool operator==(const SceneSnapshotPlan &) const = default;
};
constexpr SceneSnapshotPlan PlanSceneSnapshot(SceneSnapshotPhase phase,
    bool same_subject, bool shared, bool ready) {
  if (phase == SceneSnapshotPhase::Scene)
    return {true, !((same_subject || shared) && ready), true};
  if (phase == SceneSnapshotPhase::Reflection) return {true, true, false};
  return {};
}
inline bool CanCopySceneSnapshot(const NativeSceneCommands &scene, const SampledImage &output) {
  const auto source = scene.ColorReadImage();
  return source && output && source.texture != output.texture && source.layout != output.layout &&
      source.width == output.width && source.height == output.height && source.layers == output.layers &&
      source.format == output.format;
}
// Caller flushes preceding queued draws, binds this native scope and consumes
// its pending clear first. Scope and output owners retain images through this
// synchronous recording; their stores retain GPU objects through the fence.
template <class Commands>
bool CopySceneSnapshot(Commands &commands, const NativeSceneCommands &scene, const SampledImage &output) {
  if (!CanCopySceneSnapshot(scene, output)) return false;
  const auto source = scene.ColorReadImage();
  commands.setFramebuffer(nullptr); // finish draws AND ordinary MSAA attachment resolves
  const std::array<plume::RenderTextureBarrier, 2> before{{
      {source.texture, plume::RenderTextureLayout::COPY_SOURCE},
      {output.texture, plume::RenderTextureLayout::COPY_DEST}}};
  commands.barriers(plume::RenderBarrierStage::COPY, before.data(), uint32_t(before.size()));
  *source.layout = plume::RenderTextureLayout::COPY_SOURCE;
  *output.layout = plume::RenderTextureLayout::COPY_DEST;
  commands.copyTexture(output.texture, source.texture); // all array layers, no flattened eyes
  const plume::RenderTextureBarrier after{output.texture, plume::RenderTextureLayout::SHADER_READ};
  commands.barriers(plume::RenderBarrierStage::GRAPHICS, &after, 1);
  *output.layout = plume::RenderTextureLayout::SHADER_READ;
  return true;
}
} // namespace bd::gpu::scene
