/**
 * @brief Owned world-space occlusion observations and conservative history.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_transform.h"
#include "gpu/scene/native_bounds.h"
#include <algorithm>
#include <unordered_map>

namespace bd::gpu {
struct NativeOcclusionIdentity {
  uint64_t instance = 0, generation = 0;
  uint32_t node = 0, primitive = 0;
  bool operator==(const NativeOcclusionIdentity &) const = default;
  explicit operator bool() const { return instance && generation; }
};
struct NativeOcclusionIdentityHash {
  size_t operator()(const NativeOcclusionIdentity &id) const {
    auto value = id.instance ^ (id.generation + 0x9e3779b97f4a7c15ull + (id.instance << 6) + (id.instance >> 2));
    return std::hash<uint64_t>{}(value ^ (uint64_t(id.node) << 32) ^ id.primitive);
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
// Exactly 96 bytes: four dot-product rows and a WORLD-space box. No translated
// constants, descriptor offsets, model radius scale or source addresses.
struct NativeOcclusionPacket {
  std::array<float, 16> world_to_clip{};
  std::array<float, 4> center{}, extent{};
  bool operator==(const NativeOcclusionPacket &) const = default;
};
static_assert(sizeof(NativeOcclusionPacket) == 96);
inline std::optional<NativeOcclusionPacket> PrepareNativeOcclusion(
    const scene::RenderCamera &camera, const scene::NativeBounds &bounds) {
  if (!bounds.Valid()) return {};
  NativeOcclusionPacket result{scene::TransposeRenderMatrix(camera.world_to_clip)};
  for (unsigned axis = 0; axis < 3; ++axis) {
    result.center[axis] = float((double(bounds.min[axis])+bounds.max[axis])*0.5);
    const double half = (std::max)(double(bounds.max[axis])-result.center[axis],
        double(result.center[axis])-bounds.min[axis]);
    result.extent[axis] = std::nextafter(float((std::max)(half*1.15,1e-5)), std::numeric_limits<float>::infinity());
    if (!std::isfinite(result.extent[axis])) return {};
  }
  for (auto value : result.world_to_clip) if (!std::isfinite(value)) return {};
  // The entire inflated box must be in front of the eye and near plane.
  // Testing in homogeneous world-to-clip space works for translated/rotated
  // cameras; length(world_position) incorrectly treats the world origin as eye.
  for (uint32_t row : {2u, 3u}) {
    const auto *p = result.world_to_clip.data() + row * 4;
    double center = p[3], extent = 0, magnitude = std::abs(double(p[3]));
    for (unsigned axis = 0; axis < 3; ++axis) {
      center += double(p[axis])*result.center[axis];
      extent += std::abs(double(p[axis]))*result.extent[axis];
      magnitude += std::abs(double(p[axis]))*(std::abs(double(result.center[axis]))+result.extent[axis]);
    }
    if (!(center-extent-8*std::numeric_limits<float>::epsilon()*magnitude > 1e-5)) return {};
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
enum class NativeOcclusionDecision {
  InvalidView, InvalidBounds, Ambiguous, Capacity, NoHistory,
  ChangedDepth, ChangedCamera, ChangedBounds, Stale, Warming, Visible, Occluded, Count
};
class NativeOcclusionHistory {
public:
  void Collect(const NativeOcclusionObservation &query, bool zero) {
    if (!query.identity || !query.frame || query.frame <= last_.frame) return;
    const bool consecutive = query.frame-last_.frame == 1 && query.SameInput(last_);
    zeros_ = zero ? (consecutive ? (std::min)(zeros_+1, 2u) : 1u) : 0u;
    last_ = query;
  }
  NativeOcclusionDecision Decide(const NativeOcclusionObservation &current) const {
    using D = NativeOcclusionDecision;
    if (!last_.frame || current.identity != last_.identity) return D::NoHistory;
    if (current.scope != last_.scope) return D::ChangedDepth;
    if (current.packet.world_to_clip != last_.packet.world_to_clip) return D::ChangedCamera;
    if (current.packet.center != last_.packet.center || current.packet.extent != last_.packet.extent) return D::ChangedBounds;
    if (current.frame <= last_.frame || current.frame-last_.frame > 3) return D::Stale;
    return zeros_ >= 2 ? D::Occluded : zeros_ ? D::Warming : D::Visible;
  }
  bool Occluded(const NativeOcclusionObservation &current) const { return Decide(current) == NativeOcclusionDecision::Occluded; }
  uint32_t LastFrame() const { return last_.frame; }
private:
  NativeOcclusionObservation last_{};
  uint32_t zeros_ = 0;
};
// Only admitted native draw consumers request observations, after sibling/state
// preflight. Legacy-only nodes never occupy the query/history budget. A changed or
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
  NativeOcclusionDecision Request(NativeOcclusionIdentity identity,
      const std::optional<NativeOcclusionView> &view, const std::optional<scene::NativeBounds> &bounds) {
    using D = NativeOcclusionDecision;
    auto it = current_.find(identity);
    const bool valid_view = identity && view && frame_ && view->frame == frame_ &&
        view->scope.depth && view->scope.width && view->scope.height && view->scope.samples;
    const auto packet = valid_view && bounds ? PrepareNativeOcclusion(view->camera, *bounds) : std::nullopt;
    if (!packet) {
      if (it != current_.end()) it->second.frame = 0;
      return valid_view ? D::InvalidBounds : D::InvalidView;
    }
    NativeOcclusionObservation observation{identity, frame_, *packet, view->scope};
    if (it != current_.end()) {
      if (!it->second.SameInput(observation)) it->second.frame = 0;
      if (!it->second.frame) return D::Ambiguous;
    } else {
      if (current_.size() >= kQueries) return D::Capacity;
      current_.emplace(identity, observation);
    }
    const auto history = history_.find(identity);
    return history == history_.end() ? D::NoHistory : history->second.Decide(observation);
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
  bool HasQueries(const NativeOcclusionView &view) const {
    return std::any_of(current_.begin(), current_.end(), [&](const auto &entry) { return entry.second.Matches(view); });
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
