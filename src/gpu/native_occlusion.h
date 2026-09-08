/**
 * @brief Owned world-space occlusion observations and conservative history.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_transform.h"
#include <algorithm>
#include <unordered_map>

namespace bd::gpu {
struct NativeOcclusionIdentity {
  uint64_t instance = 0, generation = 0;
  uint32_t node = 0;
  bool operator==(const NativeOcclusionIdentity &) const = default;
  explicit operator bool() const { return instance && generation; }
};
struct NativeOcclusionIdentityHash {
  size_t operator()(const NativeOcclusionIdentity &id) const {
    auto value = id.instance ^ (id.generation + 0x9e3779b97f4a7c15ull + (id.instance << 6) + (id.instance >> 2));
    return std::hash<uint64_t>{}(value ^ id.node);
  }
};
struct NativeOcclusionScope {
  uint64_t depth = 0;
  uint32_t width = 0, height = 0, samples = 0;
  bool operator==(const NativeOcclusionScope &) const = default;
};
struct NativeOcclusionView {
  scene::RenderCamera camera;
  NativeOcclusionScope scope;
  uint32_t frame = 0;
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
  NativeOcclusionScope scope;
  bool SameInput(const NativeOcclusionObservation &other) const {
    return identity == other.identity && scope == other.scope && packet == other.packet;
  }
  bool Matches(const NativeOcclusionView &view) const {
    return frame && frame == view.frame && scope == view.scope &&
        packet.world_to_clip == scene::TransposeRenderMatrix(view.camera.world_to_clip);
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
// Current observations authorize both queries and culling. A missing, changed or
// ambiguous observation cannot inherit an earlier node's permission to disappear.
// Only values are retained: no source addresses, GPU pointers or mapped uploads.
class NativeOcclusionTracker {
public:
  static constexpr uint32_t kQueries = 2048, kHistory = 8192;
  void Begin(uint32_t frame) {
    current_.clear();
    frame_ = frame;
    for (auto it = history_.begin(); it != history_.end();) {
      if (frame < it->second.LastFrame() || frame-it->second.LastFrame() > 8) it = history_.erase(it);
      else ++it;
    }
  }
  void Note(NativeOcclusionIdentity identity, const std::optional<NativeOcclusionView> &view,
            const std::array<float, 4> &sphere) {
    if (!identity) return;
    const auto packet = view && view->frame == frame_ && view->scope.depth && view->scope.width && view->scope.height
        ? PrepareNativeOcclusion(view->camera, sphere) : std::nullopt;
    auto it = current_.find(identity);
    if (!packet) {
      if (it != current_.end()) it->second.frame = 0;
      return;
    }
    NativeOcclusionObservation observation{identity, frame_, *packet, view->scope};
    if (it != current_.end()) {
      if (!it->second.SameInput(observation)) it->second.frame = 0;
      return;
    }
    if (current_.size() < kQueries) current_.emplace(identity, observation);
  }
  void Collect(const NativeOcclusionObservation &query, bool zero) {
    if (!query.identity || !query.frame) return;
    auto it = history_.find(query.identity);
    if (it == history_.end()) {
      if (history_.size() >= kHistory) return;
      it = history_.try_emplace(query.identity).first;
    }
    it->second.Collect(query, zero);
  }
  bool Occluded(NativeOcclusionIdentity identity, const NativeOcclusionView &view) const {
    const auto current = current_.find(identity);
    const auto history = history_.find(identity);
    return current != current_.end() && current->second.Matches(view) &&
        history != history_.end() && history->second.Occluded(current->second);
  }
  template <typename Emit> void Queries(const NativeOcclusionView &view, Emit &&emit) const {
    for (const auto &[id, query] : current_) if (query.Matches(view)) emit(query);
  }
  void EndPass() { current_.clear(); }
  size_t CurrentCount() const { return current_.size(); }
  size_t HistoryCount() const { return history_.size(); }
private:
  uint32_t frame_ = 0;
  std::unordered_map<NativeOcclusionIdentity, NativeOcclusionObservation, NativeOcclusionIdentityHash> current_;
  std::unordered_map<NativeOcclusionIdentity, NativeOcclusionHistory, NativeOcclusionIdentityHash> history_;
};
} // namespace bd::gpu
