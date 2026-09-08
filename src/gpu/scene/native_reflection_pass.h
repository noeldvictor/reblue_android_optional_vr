/**
 * @brief Planar reflection completion on owned native attachments.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_scene_commands.h"

namespace bd::gpu::scene {
// The named water reflection producer owns its completed image independently
// of any outgoing resource getter. Beginning its next write invalidates reads;
// readers already holding a lease remain valid until their queue/fence retires.
class NativeReflectionPublication {
public:
  void Reset() { image_ = {}; producing_ = false; }
  void Begin(uint32_t frame) { Reset(); frame_ = frame; producing_ = true; }
  bool Complete(uint32_t frame, const NativeSceneCommands &scope, const NativeImageLease &image) {
    const auto source = scope.ColorReadImage();
    if (!producing_ || frame != frame_ || scope.ClearPending() || !image.ArrayView() ||
        !image.Fits(source.width,source.height,source.layers) ||
        image.image.format != plume::RenderFormat::R16G16B16A16_FLOAT ||
        image.image.texture != source.texture || image.image.layout != source.layout ||
        image.image.descriptor_index != source.descriptor_index ||
        *image.image.layout != plume::RenderTextureLayout::SHADER_READ) return false;
    image_ = image; producing_ = false;
    return true;
  }
  NativeImageLease Read(uint32_t frame) {
    if (frame != frame_) Reset();
    return producing_ ? NativeImageLease{} : image_;
  }
private:
  NativeImageLease image_;
  uint32_t frame_ = 0;
  bool producing_ = false;
};
// The exact producing image/view owner is the output. Flush ordered draws and
// consume the scope's clear first, including an empty reflection. No resolve,
// image copy, encoded surface format or width-selected render-target lookup.
template <class Commands>
bool FinishNativeReflection(Commands &commands, const NativeSceneCommands &scope,
    const NativeImageLease &image) {
  const auto *shape = scope.ColorShape();
  const auto source = scope.ColorReadImage();
  if (!shape || shape->samples != 1 || scope.ClearPending() || !image.ArrayView() ||
      source.texture != image.image.texture || source.layout != image.image.layout ||
      source.descriptor_index != image.image.descriptor_index ||
      image.image.format != plume::RenderFormat::R16G16B16A16_FLOAT ||
      !image.Fits(shape->width,shape->height,shape->layers)) return false;
  commands.setFramebuffer(nullptr);
  if (*source.layout != plume::RenderTextureLayout::SHADER_READ) {
    const plume::RenderTextureBarrier barrier{source.texture,plume::RenderTextureLayout::SHADER_READ};
    commands.barriers(plume::RenderBarrierStage::GRAPHICS,&barrier,1);
    *source.layout = plume::RenderTextureLayout::SHADER_READ;
  }
  return true;
}
NativeSceneCommands *ActiveNativeReflectionCommands(plume::RenderTexture *color, plume::RenderTexture *depth);
NativeImageLease FindCompletedNativeWaterReflection();
} // namespace bd::gpu::scene
