/**
 * @brief Ordered water publication and its temporary sorted-entry boundary.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_water_scene.h"
#include "gpu/scene/native_visual_inputs.h"
#include "gpu/scene/native_selected_lights.h"

namespace bd::gpu::scene {
struct NativeWaterMaterialOutput {
  NativeWaterMaterial material;
  NativeTextureGpuHandle bump, environment;
  NativeImageLease planar, snapshot;
};
// One ordered draw, not an address-indexed cache. A later/failed writer cannot
// inherit a previous draw's output. Queued packets copy values and image leases.
class NativeWaterMaterialPublication {
public:
  void Reset() { value_.reset(); }
  void Publish(NativeVisualIdentity identity, uint32_t frame, NativeWaterMaterialOutput value) {
    Reset();
    if (!identity) return;
    identity_ = identity; frame_ = frame; value_ = std::move(value);
  }
  std::optional<NativeWaterMaterialOutput> Read(NativeVisualIdentity identity, uint32_t frame) const {
    return identity && identity == identity_ && frame == frame_ ? value_ : std::nullopt;
  }
private:
  NativeVisualIdentity identity_;
  uint32_t frame_ = 0;
  std::optional<NativeWaterMaterialOutput> value_;
};
class NativeWaterMaterialScope {
public:
  NativeWaterMaterialScope(uint32_t entry, uint32_t visual, NativeVisualIdentity identity);
  ~NativeWaterMaterialScope();
  NativeWaterMaterialScope(const NativeWaterMaterialScope &) = delete;
  NativeWaterMaterialScope &operator=(const NativeWaterMaterialScope &) = delete;
  bool Draw(uint32_t stack, uint8_t *base, bool stencil_pending, int32_t &depth_write);
  // Producer-only source equality; no address enters the publication or queue.
  void Publish(uint32_t visual, std::optional<NativeWaterMaterialOutput> output);
private:
  bool Submit(bool stencil_pending);
  uint32_t entry_ = 0, visual_ = 0, frame_ = 0;
  NativeVisualIdentity identity_;
  NativeWaterMaterialPublication publication_;
  NativeSelectedLights lights_{};
};
} // namespace bd::gpu::scene
