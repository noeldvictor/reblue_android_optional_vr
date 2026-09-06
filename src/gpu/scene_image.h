/**
 * @brief   Explicit sampled-image receipt, separate from compatibility getters.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
namespace bd::gpu {
struct GuestTexture;
struct SceneImage {
  GuestTexture *image = nullptr;
  float exposure = 1.0f;
};
} // namespace bd::gpu
