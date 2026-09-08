/**
 * @brief Bounded owned packets awaiting the existing sorted scene consumer.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_rigid_scene.h"
#include "gpu/scene/deferred_work.h"
#include "gpu/scene/native_water_deferred.h"
#include <utility>

namespace bd::gpu::scene {
class NativeDeferredQueue {
public:
  struct Entry {
    NativeRigidSceneSubmission submission;
    DeferredInsertion order;
    std::optional<NativeWaterDeferred> water;
    uint32_t Frame() const { return water ? water->frame : submission.frame; }
    NativeVisualIdentity Identity() const {
      return water ? water->Identity() : NativeVisualIdentity{submission.instance,submission.model_generation};
    }
  };
  static constexpr size_t kLimit = 5140, kBytes = 8u << 20;
  static constexpr size_t kRecordBytes = sizeof(NativeRigidScenePlan) > 512 ? sizeof(NativeRigidScenePlan) : 512;
private:
  std::vector<Entry> entries_;
  bool draining_ = false;
public:
  std::span<const Entry> Entries() const { return entries_; }
  std::vector<NativeVisualIdentity> OrdinaryIdentities() const {
    std::vector<NativeVisualIdentity> identities;
    for (const auto &entry : entries_) if (!entry.water) identities.push_back(entry.Identity());
    std::sort(identities.begin(),identities.end());
    identities.erase(std::unique(identities.begin(),identities.end()),identities.end());
    return identities;
  }
  bool Stage(NativeRigidSceneSubmission &submission, uint32_t preceding, uint32_t frame) {
    if (draining_ || !submission.instance || !submission.model_generation ||
        submission.frame != frame || submission.plans.empty() || submission.plans.size() > 4096 ||
        (!entries_.empty() && (entries_.front().Frame() != frame ||
                              preceding < entries_.back().order.preceding))) return false;
    size_t count = 0;
    for (const auto &plan : submission.plans) if (plan.deferred) {
      if (!plan.draw || !std::isfinite(plan.depth) || plan.light_ticket ||
          !plan.light_recipe || !plan.light_recipe->update) return false;
      ++count;
    }
    if (!count || preceding > kLimit || entries_.size() > kLimit - preceding ||
        count > kLimit - preceding - entries_.size() ||
        kLimit * sizeof(Entry) + (entries_.size() + count) *
            kRecordBytes > kBytes) return false;
    // Allocate all siblings before publishing the first. The retained entry
    // vector has a fixed capacity; each delayed primitive owns exactly one plan.
    std::vector<Entry> pending;
    std::vector<NativeRigidScenePlan> direct;
    pending.reserve(count); direct.reserve(submission.plans.size() - count);
    for (const auto &plan : submission.plans) {
      if (!plan.deferred) { direct.push_back(plan); continue; }
      NativeRigidSceneSubmission single{submission.instance, submission.model_generation,
          submission.node, frame, submission.regression_node, {plan}};
      pending.push_back({std::move(single), {plan.depth, preceding}});
    }
    if (entries_.capacity() < kLimit) entries_.reserve(kLimit);
    for (auto &entry : pending) entries_.push_back(std::move(entry));
    submission.plans = std::move(direct);
    return true;
  }
  bool StageWater(std::span<const NativeWaterDeferred> water, uint32_t preceding, uint32_t frame) {
    // The same ordering, frame, count and byte limits apply across families.
    // Charge the bounded producer adapter conservatively in addition to entries.
    if (draining_ || water.empty() || preceding > kLimit || entries_.size() > kLimit - preceding ||
        water.size() > kLimit - preceding - entries_.size() ||
        (!entries_.empty() && (entries_.front().Frame() != frame || preceding < entries_.back().order.preceding)) ||
        kLimit * sizeof(Entry) + (entries_.size() + water.size()) *
            kRecordBytes > kBytes) return false;
    for (const auto &plan : water) if (!plan.Valid(frame)) return false;
    if (entries_.capacity() < kLimit) entries_.reserve(kLimit);
    for (const auto &plan : water) entries_.push_back({{}, {plan.depth,preceding}, plan});
    return true;
  }
  bool BeginDrain(uint32_t frame) {
    if (draining_) return false;
    for (const auto &entry : entries_) if (entry.Frame() != frame) return false;
    draining_ = true;
    return true;
  }
  std::optional<NativeWaterDeferred> TakeWater(uint32_t index) {
    if (!draining_ || index >= entries_.size() || !entries_[index].water) return {};
    return std::exchange(entries_[index].water,std::nullopt);
  }
  std::optional<NativeRigidSceneSubmission> Take(uint32_t index) {
    if (!draining_ || index >= entries_.size() || entries_[index].submission.plans.empty()) return {};
    auto result = std::move(entries_[index].submission);
    entries_[index].submission.plans.clear();
    return result;
  }
  bool EndDrain() {
    if (!draining_) return false;
    for (const auto &entry : entries_) if (entry.water || !entry.submission.plans.empty()) return false;
    entries_.clear(); draining_ = false;
    return true;
  }
};
} // namespace bd::gpu::scene
