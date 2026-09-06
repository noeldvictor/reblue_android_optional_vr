/**
 * @brief Explicit sampled scene inputs for native post-processing.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/sampled_image.h"
#include <cmath>

namespace bd::gpu {
// Borrowed, synchronous inputs; owners retain their images and live layout
// records through submission and GPU objects through the submission fence.
struct HostPostInputs {
  SampledImage scene;
  SampledImage depth;
  float exposure = 1.0f;
  bool opaque_scene_alpha = false; // native MSAA scene opacity, independent of RGB exposure

  bool CanRenderTo(plume::RenderTexture *output, uint32_t output_layers) const {
    return scene && depth && output && std::isfinite(exposure) && exposure > 0 &&
        scene.texture != depth.texture && scene.texture != output && depth.texture != output &&
        scene.layers == depth.layers && scene.layers == output_layers;
  }
};
} // namespace bd::gpu
