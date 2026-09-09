/**
 * @brief Owned animation controller plans; no source addresses or scratch layout.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_animation_blend.h"
#include <bit>

namespace bd::gpu::scene {
inline constexpr size_t kNativeAnimationSlots = 6;
struct NativeAnimationSelectionUpdate {
  bool restart=false;
  float weight=0;
};
// Selection identity is distinct from clip identity: two authored entries may
// alias one asset yet request a restart. Same selection ignores new parameters
// unless forced. Negative requested weight preserves the previous weight.
inline std::optional<NativeAnimationSelectionUpdate> PlanNativeAnimationSelection(
    uint64_t previous, uint64_t selected, bool force, double weight, float previous_weight) {
  if (!force && previous == selected) return NativeAnimationSelectionUpdate{};
  const float next_weight=weight < 0 ? previous_weight : float(weight);
  if (!std::isfinite(next_weight)) return {};
  return NativeAnimationSelectionUpdate{true,next_weight};
}
inline std::array<float,4> AdvanceNativeAnimationOffsets(
    const std::array<float,4> &offsets, const std::array<float,4> &rates) {
  std::array<float,4> result;
  for (size_t n=0; n<4; ++n) {
    float sum=offsets[n]+rates[n];
    // Scalar adds precede the source's flush-to-zero vector fractional wrap.
    // Preserve its toward-zero (not floor) rule, including negative offsets.
    if ((std::bit_cast<uint32_t>(sum)&0x7f800000u) == 0) sum=std::copysign(0.0f,sum);
    result[n]=-(std::trunc(sum)-sum);
  }
  return result;
}
struct NativeAnimationSlot {
  bool present=false, loop=false;
  uint32_t loops=0;
  float weight=0, target_weight=0, weight_rate=0;
  float time_ticks=0, time_rate=0, duration_ticks=0, contribution=0;
  NativeJointName included{{},0};
};
struct NativeAnimationControllerInput {
  std::array<NativeAnimationSlot,kNativeAnimationSlots> slots;
  std::array<NativeJointName,kNativeAnimationSlots> overlays;
  uint32_t active=0, overlay_count=0;
  float delta_ticks=0;
  bool sequential=false, replace_slots=false;
};
// Layer identities are local to a completed plan, never byte offsets. Output
// starts at the previous channels; other layers are created by whole sampling.
inline constexpr uint8_t kNativeAnimationCombined=6, kNativeAnimationOutput=7;
struct NativeAnimationStep {
  enum class Kind { Sample, Copy, Mix } kind=Kind::Sample;
  uint8_t destination=kNativeAnimationOutput, left=0, right=0, slot=0;
  float seconds=0, weight=1;
  bool reset=false, subtree=false;
  NativeJointName included{{},0}, root{{},0};
};
struct NativeAnimationControllerPlan {
  std::array<NativeAnimationSlot,kNativeAnimationSlots> slots;
  std::array<NativeAnimationStep,24> steps;
  size_t count=0;
  std::array<bool,kNativeAnimationSlots> advanced{};
};
inline bool AdvanceNativeAnimationSlot(NativeAnimationSlot &slot, float delta) {
  if (!slot.present) return true;
  for (float value : {delta,slot.weight,slot.target_weight,slot.weight_rate,
         slot.time_ticks,slot.time_rate,slot.duration_ticks})
    if (!std::isfinite(value)) return false;
  if (slot.duration_ticks < 0) return false;
  slot.weight=float(std::fma(double(slot.weight_rate),double(delta),double(slot.weight)));
  if (slot.weight > slot.target_weight) slot.weight=slot.target_weight;
  slot.time_ticks=float(std::fma(double(slot.time_rate),double(delta),double(slot.time_ticks)));
  if (!std::isfinite(slot.weight) || !std::isfinite(slot.time_ticks)) return false;
  // Authored single subtraction, NOT fmod or an accumulated multi-loop count.
  if (slot.time_ticks > slot.duration_ticks) {
    if (slot.loop) { ++slot.loops; slot.time_ticks-=slot.duration_ticks; }
    else slot.time_ticks=slot.duration_ticks;
  }
  return true;
}
inline std::optional<NativeAnimationControllerPlan> PlanNativeAnimationController(
    const NativeAnimationControllerInput &input) {
  if (input.active > kNativeAnimationSlots || input.overlay_count > kNativeAnimationSlots ||
      !std::isfinite(input.delta_ticks)) return {};
  NativeAnimationControllerPlan plan; plan.slots=input.slots;
  const bool sequential=input.active == 1 || input.sequential;
  auto sample=[&](uint32_t slot, uint8_t destination, bool reset, bool subtree) {
    const auto &state=plan.slots[slot];
    auto &step=plan.steps[plan.count++]; step.slot=uint8_t(slot); step.destination=destination;
    step.reset=reset; step.subtree=subtree;
    const float ticks=subtree ? state.time_ticks : std::clamp(state.time_ticks,0.0f,state.duration_ticks);
    step.seconds=float(double(ticks)/30.0);
    step.weight=reset ? 1 : subtree ? state.weight : std::min(state.weight,1.0f);
    step.included=subtree ? NativeJointName{{},0} : state.included;
    if (subtree) step.root=input.overlays[slot];
  };
  for (uint32_t n=0; n<input.active; ++n) {
    auto &slot=plan.slots[n];
    if (!slot.present) continue;
    if (!slot.included.Valid() || !AdvanceNativeAnimationSlot(slot,input.delta_ticks)) return {};
    plan.advanced[n]=true;
    if (sequential) {
      if (input.replace_slots || slot.weight > 0)
        sample(n,kNativeAnimationOutput,input.replace_slots,false);
    } else {
      if (!std::isfinite(slot.contribution)) return {};
      if (slot.contribution > 0) sample(n,uint8_t(n),true,false);
    }
  }
  if (!sequential) {
    bool first=true;
    for (uint32_t n=0; n<input.active; ++n) {
      const auto &slot=plan.slots[n];
      if (!std::isfinite(slot.contribution)) return {};
      if (slot.contribution <= 0) continue;
      // The source would consume uninitialized/stale global scratch here.
      // This is not a valid native layer; refuse the complete plan before writes.
      if (!slot.present) return {};
      auto &step=plan.steps[plan.count++]; step.destination=kNativeAnimationCombined;
      step.right=uint8_t(n);
      if (first) { step.kind=NativeAnimationStep::Kind::Copy; step.left=uint8_t(n); first=false; }
      else {
        step.kind=NativeAnimationStep::Kind::Mix;
        step.left=n == 1 ? 0 : kNativeAnimationCombined;
        // Authored adjacent-slot denominator, not the accumulated contribution.
        const float denominator=plan.slots[n-1].contribution+slot.contribution;
        step.weight=slot.contribution/denominator;
        if (!std::isfinite(step.weight)) return {};
      }
    }
    if (first || !std::isfinite(plan.slots[0].weight)) return {};
    auto &step=plan.steps[plan.count++]; step.destination=kNativeAnimationOutput;
    step.weight=plan.slots[0].weight;
    if (std::abs(step.weight-1) < kNativeAnimationWeightEpsilon) {
      step.kind=NativeAnimationStep::Kind::Copy; step.left=kNativeAnimationCombined;
    } else {
      step.kind=NativeAnimationStep::Kind::Mix;
      step.left=kNativeAnimationOutput; step.right=kNativeAnimationCombined;
    }
  } else {
    for (uint32_t n=1; n<input.overlay_count; ++n) {
      auto &slot=plan.slots[n];
      if (!slot.present) continue;
      if (!input.overlays[n].Valid() || !AdvanceNativeAnimationSlot(slot,input.delta_ticks)) return {};
      // A slot also in the active range advances twice, in this exact order.
      plan.advanced[n]=true; sample(n,kNativeAnimationOutput,false,true);
    }
  }
  return plan;
}
// Authored post-controller layers advance once per update, independently of the
// main delta. Their selection gates are boundary inputs, not console field IDs.
inline std::optional<NativeAnimationControllerPlan> PlanNativeAnimationLateLayers(
    const std::array<NativeAnimationSlot,kNativeAnimationSlots> &slots,
    const std::array<bool,kNativeAnimationSlots> &enabled, bool replace,
    NativeJointName included) {
  if (!included.Valid()) return {};
  NativeAnimationControllerPlan plan; plan.slots=slots;
  for (uint8_t n : {4,5,3}) {
    auto &slot=plan.slots[n];
    if (!enabled[n] || !slot.present) continue;
    if (!AdvanceNativeAnimationSlot(slot,1)) return {};
    plan.advanced[n]=true;
    if (!replace && slot.weight <= 0) continue;
    auto &step=plan.steps[plan.count++]; step.slot=n;
    step.reset=replace; step.included=included;
    step.seconds=float(double(std::clamp(slot.time_ticks,0.0f,slot.duration_ticks))/30.0);
    step.weight=replace ? 1 : std::min(slot.weight,1.0f);
  }
  return plan;
}
} // namespace bd::gpu::scene
