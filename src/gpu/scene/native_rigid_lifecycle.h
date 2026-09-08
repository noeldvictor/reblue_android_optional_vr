/**
 * @brief Bounded generation-specific acceptance evidence; no source or GPU dependencies.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace bd::gpu::scene {
// Per retained draw, not a frame estimate. Generation, actual command recording,
// GPU visibility and resource retirement are different events.
class NativeRigidOutputReceipt {
public:
  bool Record() { if (recorded_) return false; recorded_ = true; return true; }
  bool Resolve(bool visible) {
    if (!recorded_ || resolved_) return false;
    resolved_ = true; visible_ = visible; return true;
  }
  bool Retire() {
    if (!resolved_ || retired_) return false;
    retired_ = true; return true;
  }
  bool Recorded() const { return recorded_; }
  bool Visible() const { return resolved_ && visible_; }
  bool Culled() const { return resolved_ && !visible_; }
private:
  bool recorded_ = false, resolved_ = false, visible_ = false, retired_ = false;
};
struct NativeRigidEpoch {
  struct Counts { uint64_t submitted = 0, emitted = 0, retired = 0; };
  uint64_t generation = 0, first_instance = 0;
  bool source_retired = false;
  std::array<Counts, 2> views{}; // shadow, scene
  bool Closed() const {
    return generation && source_retired && views[0].submitted == views[0].retired &&
        views[1].submitted == views[1].retired;
  }
};
class NativeRigidLifecycle {
public:
  enum class Event { Submitted, Emitted, FenceRetired };
  static constexpr size_t kCapacity = 8;
  const NativeRigidEpoch *Find(uint64_t generation) const {
    if (generation) for (const auto &epoch : epochs_) if (epoch.generation == generation) return &epoch;
    return nullptr;
  }
  bool Loaded(uint64_t generation) {
    if (!generation || Find(generation)) return false;
    for (auto &epoch : epochs_) if (!epoch.generation) {
      epoch.generation = generation; return true;
    }
    return false; // Never evict proof to make a full diagnostic fit.
  }
  bool SourceRetired(uint64_t generation) {
    auto *epoch = const_cast<NativeRigidEpoch *>(Find(generation));
    if (!epoch || epoch->source_retired) return false;
    epoch->source_retired = true;
    return true;
  }
  bool Note(Event event, uint64_t generation, uint64_t instance, uint32_t view, uint64_t count = 1) {
    auto *epoch = const_cast<NativeRigidEpoch *>(Find(generation));
    if (!epoch || !count || (view != 1 && view != 3)) return false;
    auto &values = epoch->views[view == 3];
    if (event == Event::Submitted) {
      if (!instance || count != 1 || epoch->source_retired ||
          values.submitted == (std::numeric_limits<uint64_t>::max)()) return false;
      if (!epoch->first_instance) epoch->first_instance = instance;
      ++values.submitted;
      return true;
    }
    // Commands already retained before source destruction may finish later.
    // Count actual emissions independently: fence retirement alone is not output.
    auto &total = event == Event::Emitted ? values.emitted : values.retired;
    if (count > values.submitted - total) return false;
    total += count;
    return true;
  }
private:
  std::array<NativeRigidEpoch, kCapacity> epochs_{};
};
// A continuous interactive-field window, not elapsed boot time. Losing field
// readiness resets its baseline; a different generation starts a new window.
class NativeRigidOutputWindow {
public:
  uint64_t Generation() const { return generation_; }
  const std::array<uint64_t,2> &Baseline() const { return baseline_; }
  // Reports precede their same-frame context. Three 300-frame periods leave
  // two complete post-readiness windows even at the worst reporting phase.
  bool Step(bool ready, const NativeRigidEpoch &epoch, uint64_t required = 900) {
    if (!ready || !epoch.generation || epoch.source_retired) { generation_ = 0; return false; }
    if (generation_ != epoch.generation) {
      generation_ = epoch.generation;
      for (size_t i = 0; i < 2; ++i) baseline_[i] = epoch.views[i].emitted;
      return false;
    }
    for (size_t i = 0; i < 2; ++i)
      if (epoch.views[i].emitted < baseline_[i] || epoch.views[i].emitted - baseline_[i] < required) return false;
    return required != 0;
  }
private:
  uint64_t generation_ = 0;
  std::array<uint64_t, 2> baseline_{};
};
// Same opt-in readiness contract as the reload bridge, exposed for boundary
// tests and reason reporting. These are refusal bits, not a relaxed fallback.
struct NativeRigidReloadReadiness {
  bool paused = false, walking = false;
  uint64_t stage = 0;
  enum : uint32_t { Paused = 1, NotWalking = 2, WrongStage = 4, Stale = 8 };
  uint32_t Blockers(int64_t age_ns) const {
    return (paused ? Paused : 0) | (!walking ? NotWalking : 0) |
        (stage != ((uint64_t(2) << 32) | 4101) ? WrongStage : 0) |
        (age_ns < 0 || age_ns >= 250000000 ? Stale : 0);
  }
};
} // namespace bd::gpu::scene
