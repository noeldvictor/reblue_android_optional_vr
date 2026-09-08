/**
 * @brief Water-bottom depth and its producing projection, independent of source slots.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_scene_commands.h"

namespace bd::gpu::scene {
struct NativeWaterBottom {
  NativeImageLease image;
  RenderMatrix world_to_bottom{};
};

// Keep the projection of the pass that actually wrote this image. The legacy
// VS40 publication is a later view-to-water transform, not this world matrix.
inline std::optional<NativeWaterBottom> ReadNativeWaterBottom(
    const NativeSceneCommands &scope, uint32_t frame, uint32_t view) {
  const auto camera = scope.Camera(frame, view);
  const auto image = NativeImageLease::From(scope.DepthOwner());
  if (scope.ColorShape() || scope.ClearPending() || !camera || !image.ArrayView() ||
      image.image.format != plume::RenderFormat::D32_FLOAT_S8_UINT ||
      *image.image.layout != plume::RenderTextureLayout::SHADER_READ) return {};
  return NativeWaterBottom{image, camera->world_to_clip};
}

// Caller flushes ordered draws and consumes the scope's clear before completion.
// No copy, console resolve, colour conversion or guessed source selection.
template <class Commands>
bool FinishNativeWaterBottom(Commands &commands, const NativeSceneCommands &scope) {
  const auto depth = scope.DepthOwner();
  if (scope.ColorShape() || scope.ClearPending() || !depth || !depth->Sampled()) return false;
  commands.setFramebuffer(nullptr);
  if (depth->layout != plume::RenderTextureLayout::SHADER_READ) {
    const plume::RenderTextureBarrier barrier{depth->image.get(), plume::RenderTextureLayout::SHADER_READ};
    commands.barriers(plume::RenderBarrierStage::GRAPHICS, &barrier, 1);
    depth->layout = plume::RenderTextureLayout::SHADER_READ;
  }
  return true;
}

NativeSceneCommands *ActiveNativeWaterBottomCommands(plume::RenderTexture *color, plume::RenderTexture *depth);
std::optional<NativeWaterBottom> FindCompletedNativeWaterBottom();
} // namespace bd::gpu::scene
