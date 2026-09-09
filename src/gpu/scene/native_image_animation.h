/**
 * @brief Immutable authored image windows and native texture leases.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_images.h"
#include <bit>
#include <limits>

namespace bd::gpu::scene {
struct NativeImageWindow {
  float start=0, duration=0, end=0;
  int32_t repeat=0;
  bool loop=false;
};
// Inclusive authored interval, indefinite zero-duration hold, then integer
// repeat. Last matching window wins; no match preserves the preceding image.
inline std::optional<bool> NativeImageWindowActive(const NativeImageWindow &window, int32_t frame) {
  if (!std::isfinite(window.start) || !std::isfinite(window.duration) || !std::isfinite(window.end)) return {};
  const float time=float(frame);
  if (window.start >= 0 && window.start <= time && (window.end >= time || window.duration == 0)) return true;
  if (!window.loop || !window.repeat) return false;
  if (double(window.start) < double(INT32_MIN) || double(window.start) > double(INT32_MAX)) return {};
  const int32_t elapsed=std::bit_cast<int32_t>(uint32_t(frame)-uint32_t(int32_t(window.start)));
  if (elapsed < 0) return false;
  const float period=float(double(float(window.repeat))*double(window.duration));
  if (!std::isfinite(period) || period < 1 || double(period) > double(INT32_MAX)) return {};
  return float(elapsed % int32_t(period)) < window.duration;
}
struct NativeImageAnimation {
  struct Key {
    NativeImageWindow window;
    MaterialImageSelection<NativeTextureBinding> image;
    bool procedural=false;
  };
  std::vector<Key> keys;
  // UINT32_MAX is the authored hold, not a replacement with an empty image.
  std::optional<uint32_t> Select(int32_t frame) const {
    uint32_t selected=UINT32_MAX;
    for (uint32_t n=0; n<keys.size(); ++n) {
      const auto active=NativeImageWindowActive(keys[n].window,frame);
      if (!active) return {};
      if (*active) selected=n;
    }
    return selected;
  }
};
} // namespace bd::gpu::scene
