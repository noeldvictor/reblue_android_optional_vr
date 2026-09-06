/**
 * @brief Borrowed native sampled image, independent of guest resource headers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <plume_render_interface_types.h>

namespace bd::gpu {
// The owner retains the image, descriptor and layout record until its readers
// have submitted, and retains GPU objects until their submission fence. The
// layout pointer updates the owner's actual state; this is not a copied cache.
struct SampledImage {
  plume::RenderTexture *texture = nullptr;
  plume::RenderTextureLayout *layout = nullptr;
  uint32_t width = 0, height = 0, layers = 0;
  plume::RenderFormat format = plume::RenderFormat::UNKNOWN;
  uint32_t descriptor_index = ~uint32_t{0};
  uint32_t samples = 0;
  explicit operator bool() const {
    return texture && layout && width && height && layers && layers <= 2 &&
        format != plume::RenderFormat::UNKNOWN && descriptor_index != ~uint32_t{0} && samples == 1;
  }
};
} // namespace bd::gpu
