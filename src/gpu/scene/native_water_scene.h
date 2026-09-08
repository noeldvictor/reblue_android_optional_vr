/**
 * @brief Owned water scene packets consumed by the shared native draw queue.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_water_inputs.h"
#include "gpu/scene/native_texture_gpu.h"
#include "gpu/scene/native_mesh.h"
#include "gpu/scene/native_blend.h"
#include "gpu/native_image_lease.h"
#include <vector>

namespace bd::gpu::scene {
// Image and view must share the same retained owner. These are explicit native
// roles, not snapshots of translated texture slots or bindless register indices.
struct NativeWaterImages {
  NativeTextureGpuHandle bump, environment;
  NativeImageLease planar, snapshot, bottom, shadow;
  bool operator==(const NativeWaterImages &) const = default;
  bool Ready(const RigidUint4 &layers) const {
    using D = plume::RenderTextureViewDimension;
    if (!bump || !environment || !bump->image || !bump->view ||
        !environment->image || !environment->view ||
        bump->dimension != D::TEXTURE_2D_ARRAY || environment->dimension != D::TEXTURE_CUBE || layers.w) return false;
    const auto target = [](const NativeImageLease &lease, uint32_t count, bool depth) {
      return lease.ArrayView() && (count == 1 || count == 2) && lease.image.layers == count &&
          lease.image.format == (depth ? plume::RenderFormat::D32_FLOAT_S8_UINT : plume::RenderFormat::R16G16B16A16_FLOAT) &&
          *lease.image.layout == plume::RenderTextureLayout::SHADER_READ;
    };
    return target(planar,layers.x,false) && target(snapshot,layers.y,false) &&
        target(bottom,layers.z,false) && target(shadow,1,true);
  }
  const plume::RenderTexture *Image(uint32_t role) const {
    switch (role) {
    case 0: return bump->image.get(); case 1: return planar.image.texture;
    case 2: return snapshot.image.texture; case 3: return bottom.image.texture;
    case 4: return environment->image.get(); case 5: return shadow.image.texture;
    default: return nullptr;
    }
  }
  const plume::RenderTextureView *View(uint32_t role) const {
    switch (role) {
    case 0: return bump->view.get(); case 1: return planar.ArrayView();
    case 2: return snapshot.ArrayView(); case 3: return bottom.ArrayView();
    case 4: return environment->view.get(); case 5: return shadow.ArrayView();
    default: return nullptr;
    }
  }
};
struct NativeWaterBatchData {
  NativeWaterInstanceGPU input{};
  NativeWaterImages images;
  std::array<const plume::RenderSampler *,6> samplers{};
  bool Ready() const {
    return images.Ready(input.image_layers) &&
        std::all_of(samplers.begin(),samplers.end(),[](const auto *sampler) { return sampler != nullptr; });
  }
};
// Same binding operation in the real emitter and GPU fixture. Complete preflight
// precedes any descriptor mutation. No optional image is filled from an active
// attachment: producers must explicitly supply legal unused native resources.
inline bool BindNativeWaterImages(const NativeWaterBatchData &water,
    plume::RenderDescriptorSet &images, plume::RenderDescriptorSet &samplers) {
  if (!water.Ready()) return false;
  for (uint32_t role = 0; role < 6; ++role) {
    images.setTexture(role,water.images.Image(role),plume::RenderTextureLayout::SHADER_READ,water.images.View(role));
    samplers.setSampler(role,water.samplers[role]);
  }
  return true;
}
// Deformation is world-Y, AFTER the affine transform. The native VS adds
// amplitude * COLOR0.r * (sin(a) + .5*sin(b)); include both waves, signed/HDR
// weights and outward FP32 error, not a guessed constant radius.
inline std::optional<NativeBounds> NativeWaterWorldBounds(const NativeGeometry &geometry,
    const NativeWaterInstanceGPU &input) {
  if (!geometry.bounds || !geometry.wave_weight || !std::isfinite(*geometry.wave_weight) ||
      *geometry.wave_weight < 0 || !std::isfinite(input.object_data.material.waves.x)) return {};
  auto bounds = TransformNativeBounds(*geometry.bounds,std::bit_cast<RenderMatrix>(input.object_data.world));
  if (!bounds) return {};
  const double displacement = 1.5 * std::abs(double(input.object_data.material.waves.x)) * *geometry.wave_weight;
  const double magnitude = (std::max)(std::abs(double(bounds->min[1])),std::abs(double(bounds->max[1]))) + displacement;
  const double error = 16*std::numeric_limits<float>::epsilon()*magnitude +
      32*double((std::numeric_limits<float>::min)());
  bounds->min[1] = std::nextafter(float(double(bounds->min[1])-displacement-error),-std::numeric_limits<float>::infinity());
  bounds->max[1] = std::nextafter(float(double(bounds->max[1])+displacement+error),std::numeric_limits<float>::infinity());
  return bounds->Valid() ? bounds : std::nullopt;
}
struct NativeWaterScenePlan {
  std::shared_ptr<const NativeGeometry> geometry;
  NativeWaterInstanceGPU input{};
  NativeWaterImages images;
  std::array<plume::RenderSamplerDesc,6> samplers{};
  BlendState blend;
  plume::RenderCullMode cull = plume::RenderCullMode::NONE;
  bool depth_write = true;
};
struct NativeWaterSceneSubmission {
  uint64_t instance = 0, model_generation = 0;
  uint32_t frame = 0;
  std::vector<NativeWaterScenePlan> plans;
};
} // namespace bd::gpu::scene
