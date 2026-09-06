/**
 * @brief Borrowed native HDR post output and physical-image preflight.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/host_post_inputs.h"
#include <plume_render_interface.h>

namespace bd::gpu {
// The owner supplies a framebuffer attached to this exact image, with matching
// layers, and keeps it and the sampling descriptor alive through the submission
// fence. Layout changes update that owner's live record. Post roots retain HDR;
// conversion to a presentation format belongs to the final frame consumer.
struct HostPostOutput {
  SampledImage image;
  plume::RenderFramebuffer *framebuffer = nullptr;

  explicit operator bool() const {
    return image && framebuffer && image.format == plume::RenderFormat::R16G16B16A16_FLOAT &&
        framebuffer->getWidth() == image.width && framebuffer->getHeight() == image.height;
  }
  bool CanRender(const HostPostInputs &inputs) const {
    return bool(*this) && inputs.CanRenderTo(image.texture, image.layers);
  }
  bool CanSampleMono(const SampledImage &source) const {
    return bool(*this) && source && source.layers == 1 && source.texture != image.texture;
  }
};
} // namespace bd::gpu
