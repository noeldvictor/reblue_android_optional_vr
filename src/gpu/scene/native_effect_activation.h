/**
 * @brief   Host effect activation and participant registry ordering.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
namespace bd::gpu::scene {
enum class RenderFeature : uint8_t {
  All, Shadow, Reflections, ShadowVolume, PairedEffects, Flag5, Flag6,
  Post, ModeTwo, ModeOne, IndexedViews, Auxiliary
};
enum class EffectParticipantGroup : uint8_t { Shadow, Reflection, Pair, Indexed, Auxiliary };

template <class Adapter, class Handle>
bool ActivateEffectParticipant(Adapter &adapter, Handle participant, uint8_t value) {
  if (!adapter.ChangeAllowed(participant) || adapter.Active(participant) == value)
    return false;
  // Publish before the registry callbacks, just as for other authored requests.
  adapter.SetActive(participant, value);
  adapter.Membership(participant, value != 0);
  return true;
}
template <class Adapter>
void ApplyRenderFeature(Adapter &adapter, RenderFeature feature, uint8_t value) {
  if (feature == RenderFeature::All) {
    for (uint8_t i = 1; i <= 7; ++i)
      ApplyRenderFeature(adapter, RenderFeature(i), value);
    return;
  }
  if (feature == RenderFeature::ModeTwo || feature == RenderFeature::ModeOne) {
    adapter.SetMode(value ? (feature == RenderFeature::ModeTwo ? 2u : 1u) : 0u);
    return;
  }
  if (adapter.Cached(feature) == value) return;
  auto activate = [&](EffectParticipantGroup group, int32_t index, uint8_t active) {
    return ActivateEffectParticipant(adapter, adapter.Participant(group, index), active);
  };
  switch (feature) {
  case RenderFeature::Shadow: activate(EffectParticipantGroup::Shadow, 0, value); break;
  case RenderFeature::Reflections:
    for (int32_t i = 0; i < 11; ++i) activate(EffectParticipantGroup::Reflection, i, value);
    break;
  case RenderFeature::PairedEffects:
    for (int32_t i = 0; i < 2; ++i) activate(EffectParticipantGroup::Pair, i, value);
    break;
  case RenderFeature::IndexedViews: {
    auto count = adapter.IndexedCount();
    for (int32_t i = 0; i < count; ++i)
      if (activate(EffectParticipantGroup::Indexed, i, value)) count = adapter.IndexedCount();
    // The count is refreshed only after a membership callback, not on an
    // unchanged slot. The inactive tail starts at that last observed count.
    for (int32_t i = count; i < 8; ++i) activate(EffectParticipantGroup::Indexed, i, 0);
    break;
  }
  case RenderFeature::Auxiliary: activate(EffectParticipantGroup::Auxiliary, 0, value); break;
  case RenderFeature::Post: adapter.SetPost(value); break;
  default: break; // ShadowVolume/Flag5/Flag6 only publish the cached byte.
  }
  adapter.SetCached(feature, value);
}

template <class Adapter, class Handle>
void RegisterEffectParticipant(Adapter &adapter, Handle participant) {
  for (uint32_t group = 0; group < 3; ++group) {
    // The callback can change after a preceding group was updated.
    if (!(adapter.Mask(participant) & (1u << group))) continue;
    const int32_t count = adapter.Count(group);
    bool inserted = false;
    for (int32_t i = 0; i < count; ++i) {
      const int32_t priority = adapter.Priority(participant);
      const auto existing = adapter.Entry(group, i); // after priority callback
      if (priority > adapter.Priority(existing)) {
        adapter.Insert(group, i, participant);
        inserted = true;
        break;
      }
    }
    if (!inserted) adapter.Append(group, participant); // use the live count here
  }
}
template <class Adapter, class Handle>
void UnregisterEffectParticipant(Adapter &adapter, Handle participant) {
  for (uint32_t group = 0; group < 3; ++group) {
    if (!(adapter.Mask(participant) & (1u << group))) continue;
    const int32_t count = adapter.Count(group);
    for (int32_t i = 0; i < count; ++i) {
      if (adapter.Entry(group, i) != participant) continue;
      adapter.Erase(group, i); // remove the first matching occurrence only
      break;
    }
  }
}
} // namespace bd::gpu::scene
