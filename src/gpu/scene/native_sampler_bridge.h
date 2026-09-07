/**
 * @brief Native sampler producer access and ordinary 2D backend conversion.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_sampler.h"
#include "gpu/scene/native_texture_binding.h"
#include "gpu/sampler_key.h"
#include <plume_render_interface_types.h>
namespace bd::gpu::scene {
std::optional<NativeSamplerFilterPass> FindNativeSamplerFilters(uint32_t render_view);
inline bool MaterialSamplerImage2D(const NativeTextureBinding &binding) {
  if (!binding.primary || binding.slice_2d || binding.cube) return false;
  using D = plume::RenderTextureViewDimension;
  // NativeTextureGpu::Upload exposes ordinary images as 2D arrays. Array-layer
  // selection is not a filtered third coordinate and does not use addressW.
  return binding.primary->dimension == D::TEXTURE_2D ||
      binding.primary->dimension == D::TEXTURE_2D_ARRAY;
}
inline plume::RenderSamplerDesc MaterialSamplerDesc(const NativeMaterialSampler2D &sampler) {
  plume::RenderSamplerDesc result;
  result.minFilter = sampler.filters.min == MaterialSampleFilter::Nearest ? plume::RenderFilter::NEAREST : plume::RenderFilter::LINEAR;
  result.magFilter = sampler.filters.mag == MaterialSampleFilter::Nearest ? plume::RenderFilter::NEAREST : plume::RenderFilter::LINEAR;
  result.mipmapMode = sampler.filters.mip == MaterialSampleFilter::Nearest ? plume::RenderMipmapMode::NEAREST : plume::RenderMipmapMode::LINEAR;
  const auto address = [](MaterialSampleAddress value) {
    return value == MaterialSampleAddress::Mirror ? plume::RenderTextureAddressMode::MIRROR :
        value == MaterialSampleAddress::Clamp ? plume::RenderTextureAddressMode::CLAMP : plume::RenderTextureAddressMode::WRAP;
  };
  result.addressU = address(sampler.u); result.addressV = address(sampler.v);
  // No border-address mode and no third coordinate in this contract. Canonical
  // unused fields must never import console W/border state. Cubes/volumes are separate.
  result.addressW = plume::RenderTextureAddressMode::WRAP;
  result.borderColor = plume::RenderBorderColor::TRANSPARENT_BLACK;
  return result;
}
inline bool MaterialSamplerMatches2D(const plume::RenderSamplerDesc &owned, plume::RenderSamplerDesc imported) {
  imported.addressW = owned.addressW;
  imported.borderColor = owned.borderColor; // owned U/V can never use border addressing
  return SamplerKey(owned) == SamplerKey(imported);
}
} // namespace bd::gpu::scene
