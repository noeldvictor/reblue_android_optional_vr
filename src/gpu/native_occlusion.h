/**
 * @brief Owned world-space occlusion observations and conservative history.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_transform.h"
#include <algorithm>

namespace bd::gpu {
struct NativeOcclusionIdentity {
  uint64_t instance = 0, generation = 0;
  uint32_t node = 0;
  bool operator==(const NativeOcclusionIdentity &) const = default;
  explicit operator bool() const { return instance && generation; }
};
// Exactly 80 bytes, shared with native_occ_proxy_vs: four dot-product rows and a
// WORLD-space cube. No translated constants, descriptor offsets or source VAs.
struct NativeOcclusionPacket {
  std::array<float, 16> world_to_clip{};
  std::array<float, 4> sphere{};
  bool operator==(const NativeOcclusionPacket &) const = default;
};
static_assert(sizeof(NativeOcclusionPacket) == 80);
inline std::optional<NativeOcclusionPacket> PrepareNativeOcclusion(
    const scene::RenderCamera &camera, const std::array<float, 4> &sphere) {
  for (auto value : sphere) if (!std::isfinite(value)) return {};
  if (sphere[3] <= 0) return {};
  NativeOcclusionPacket result{scene::TransposeRenderMatrix(camera.world_to_clip), sphere};
  result.sphere[3] *= 1.15f;
  if (!std::isfinite(result.sphere[3])) return {};
  for (auto value : result.world_to_clip) if (!std::isfinite(value)) return {};
  // The entire inflated cube must be in front of the eye and near plane.
  // Testing in homogeneous world-to-clip space works for translated/rotated
  // cameras; length(world_position) incorrectly treats the world origin as eye.
  for (uint32_t row : {2u, 3u}) {
    const auto *p = result.world_to_clip.data() + row * 4;
    const double center = double(p[0])*sphere[0] + double(p[1])*sphere[1] + double(p[2])*sphere[2] + p[3];
    const double extent = double(result.sphere[3]) * (std::abs(p[0])+std::abs(p[1])+std::abs(p[2]));
    if (!(center-extent > 1e-5)) return {};
  }
  return result;
}
struct NativeOcclusionObservation {
  NativeOcclusionIdentity identity;
  uint32_t frame = 0;
  NativeOcclusionPacket packet;
  bool SameInput(const NativeOcclusionObservation &other) const {
    return identity == other.identity && packet == other.packet;
  }
};
class NativeOcclusionHistory {
public:
  void Collect(const NativeOcclusionObservation &query, bool zero) {
    if (!query.identity || !query.frame || query.frame <= last_.frame) return;
    const bool consecutive = query.frame-last_.frame == 1 && query.SameInput(last_);
    zeros_ = zero ? (consecutive ? (std::min)(zeros_+1, 2u) : 1u) : 0u;
    last_ = query;
  }
  bool Occluded(const NativeOcclusionObservation &current) const {
    return zeros_ >= 2 && current.frame > last_.frame && current.frame-last_.frame <= 3 &&
        current.SameInput(last_);
  }
  uint32_t LastFrame() const { return last_.frame; }
private:
  NativeOcclusionObservation last_{};
  uint32_t zeros_ = 0;
};
} // namespace bd::gpu
