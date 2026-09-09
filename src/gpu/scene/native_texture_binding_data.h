/**
 * @brief Sampled-image leases usable without renderer headers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <memory>

namespace bd::gpu::scene {
struct NativeTextureGpu;
struct NativeTextureBinding {
  std::shared_ptr<const NativeTextureGpu> primary, slice_2d, cube;
  bool operator==(const NativeTextureBinding &) const = default;
};
} // namespace bd::gpu::scene
