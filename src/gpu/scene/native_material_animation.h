/**
 * @brief Instance-owned material animation selection and clock.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_image_catalog.h"
#include <array>
#include <memory>

namespace bd::gpu::scene {
struct NativeMaterialAnimation {
  std::shared_ptr<const NativeImageCatalog> catalog;
  std::array<uint32_t,2> ids{}, queued{};
  std::array<uint32_t,2> cues{UINT32_MAX,UINT32_MAX}; // catalog ordinals; absent is explicit
  uint32_t loop_bits=0;
  float time=0, speed=1;
  bool Valid() const {
    return catalog && std::ranges::all_of(cues,[&](uint32_t cue) {
      return cue == UINT32_MAX || cue < catalog->entries.size();
    });
  }
};
} // namespace bd::gpu::scene
