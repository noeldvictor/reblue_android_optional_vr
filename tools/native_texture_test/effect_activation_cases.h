#pragma once
#include "gpu/scene/native_effect_activation.h"
#include "gpu/scene/native_registry_array.h"
#include <array>
#include <functional>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
using namespace bd::gpu::scene;
inline void EffectRequire(bool valid) { if (!valid) throw std::runtime_error("Effect activation test failed"); }
struct EffectFixture {
  std::array<uint8_t, 12> cached{};
  std::array<uint8_t, 23> active{}, allowed{};
  std::vector<int> events;
  std::function<void(uint32_t, bool)> changed;
  int32_t count = 0;
  uint32_t mode = 99;
  uint8_t post = 0;
  EffectFixture() { allowed.fill(1); }
  uint8_t Cached(RenderFeature feature) { return cached[uint32_t(feature)]; }
  void SetCached(RenderFeature feature, uint8_t value) { cached[uint32_t(feature)] = value; events.push_back(100 + int(feature)); }
  void SetMode(uint32_t value) { mode = value; }
  void SetPost(uint8_t value) { post = value; events.push_back(200); }
  uint32_t Participant(EffectParticipantGroup group, int32_t index) {
    const std::array<uint32_t, 5> bases{0, 1, 12, 14, 22};
    return bases[uint32_t(group)] + uint32_t(index);
  }
  bool ChangeAllowed(uint32_t participant) { return allowed.at(participant) != 0; }
  uint8_t Active(uint32_t participant) { return active.at(participant); }
  void SetActive(uint32_t participant, uint8_t value) { active.at(participant) = value; }
  void Membership(uint32_t participant, bool enabled) {
    EffectRequire((active.at(participant) != 0) == enabled);
    events.push_back((enabled ? 1 : -1) * (int(participant) + 1));
    if (changed) changed(participant, enabled);
  }
  int32_t IndexedCount() { return count; }
};
struct ArrayFixture {
  std::vector<uint32_t> data;
  uint32_t count = 0, maximum = 64, allocations = 0;
  uint32_t Count() { return count; }
  uint32_t Capacity() { return uint32_t(data.size()); }
  uint32_t MaxCapacity() { return maximum; }
  bool HasStorage() { return !data.empty(); }
  uint32_t Read(uint32_t i) { return data.at(i); }
  void Write(uint32_t i, uint32_t value) { data.at(i) = value; }
  void SetCount(uint32_t value) { count = value; }
  void Reserve(uint32_t capacity, bool zero_tail) { data.resize(capacity, zero_tail ? 0u : 0xdeadbeefu); ++allocations; }
  std::vector<uint32_t> Live() { return {data.begin(), data.begin() + count}; }
};
struct RegistryFixture {
  std::array<ArrayFixture, 3> groups;
  std::array<int32_t, 16> priorities{};
  std::array<uint32_t, 16> masks{};
  std::function<void(uint32_t)> priority_call;
  std::function<void(uint32_t)> mask_call;
  uint32_t mask_calls = 0, priority_calls = 0;
  uint32_t Mask(uint32_t participant) { ++mask_calls; if (mask_call) mask_call(mask_calls); return masks.at(participant); }
  int32_t Priority(uint32_t participant) { ++priority_calls; if (priority_call) priority_call(participant); return priorities.at(participant); }
  int32_t Count(uint32_t group) { return int32_t(groups.at(group).Count()); }
  uint32_t Entry(uint32_t group, int32_t index) { return groups.at(group).Read(uint32_t(index)); }
  void Insert(uint32_t group, int32_t index, uint32_t participant) { InsertRegistryEntry(groups.at(group), uint32_t(index), participant); }
  void Append(uint32_t group, uint32_t participant) { AppendRegistryEntry(groups.at(group), participant); }
  void Erase(uint32_t group, int32_t index) { EraseRegistryEntry(groups.at(group), uint32_t(index)); }
};
void CheckEffectActivation() {
  for (uint32_t value = 0; value < 256; ++value) {
    EffectFixture fixture;
    fixture.cached.fill(uint8_t(value ^ 1));
    fixture.active.fill(uint8_t(value ^ 1));
    ApplyRenderFeature(fixture, RenderFeature::All, uint8_t(value));
    for (uint32_t i = 1; i <= 7; ++i) EffectRequire(fixture.cached[i] == value);
    for (uint32_t i = 0; i < 14; ++i) EffectRequire(fixture.active[i] == value);
    EffectRequire(fixture.post == value && fixture.cached[10] == uint8_t(value ^ 1));
    std::vector<int> publications;
    for (auto event : fixture.events) if (event >= 100) publications.push_back(event);
    EffectRequire(publications == std::vector<int>{101,102,103,104,105,106,200,107});
    const auto events = fixture.events;
    ApplyRenderFeature(fixture, RenderFeature::All, uint8_t(value));
    EffectRequire(fixture.events == events); // exact-byte cached no-op
    ApplyRenderFeature(fixture, RenderFeature::ModeTwo, uint8_t(value));
    EffectRequire(fixture.mode == (value ? 2u : 0u));
    ApplyRenderFeature(fixture, RenderFeature::ModeOne, uint8_t(value));
    EffectRequire(fixture.mode == (value ? 1u : 0u));
  }
  EffectFixture unavailable;
  unavailable.allowed.fill(0);
  ApplyRenderFeature(unavailable, RenderFeature::Reflections, 2);
  EffectRequire(unavailable.cached[2] == 2 && unavailable.events == std::vector<int>{102});
  EffectFixture reflections;
  reflections.changed = [&](uint32_t participant, bool) {
    EffectRequire(reflections.cached[2] == 0); // cache publication follows all callbacks
    if (participant == 1) reflections.allowed[2] = 0;
  };
  ApplyRenderFeature(reflections, RenderFeature::Reflections, 255);
  EffectRequire(reflections.active[1] == 255 && reflections.active[2] == 0 && reflections.active[11] == 255);
  EffectFixture indexed;
  indexed.count = 4;
  indexed.active.fill(1);
  indexed.active[14] = 0;
  indexed.changed = [&](uint32_t participant, bool enabled) { if (participant == 14 && enabled) indexed.count = 1; };
  ApplyRenderFeature(indexed, RenderFeature::IndexedViews, 1);
  EffectRequire(indexed.events == std::vector<int>{15,-16,-17,-18,-19,-20,-21,-22,110});
  EffectFixture unchanged;
  unchanged.count = 2;
  unchanged.active[14] = unchanged.active[15] = 1;
  unchanged.active[16] = 1;
  ApplyRenderFeature(unchanged, RenderFeature::IndexedViews, 1);
  EffectRequire(unchanged.events == std::vector<int>{-17,110});
  EffectFixture auxiliary;
  ApplyRenderFeature(auxiliary, RenderFeature::Auxiliary, 2);
  EffectRequire(auxiliary.events == std::vector<int>{23,111});

  ArrayFixture array;
  AppendRegistryEntry(array, 10u);
  EffectRequire(array.Capacity() == 1 && array.Live() == std::vector<uint32_t>{10});
  AppendRegistryEntry(array, 20u);
  EffectRequire(array.Capacity() == 4 && array.data[2] == 0 && array.data[3] == 0);
  InsertRegistryEntry(array, 1, 15u);
  InsertRegistryEntry(array, 99, 30u); // clamp at live end
  EffectRequire(array.Live() == std::vector<uint32_t>{10,15,20,30} && array.allocations == 2);
  InsertRegistryEntry(array, 0, 5u);
  EffectRequire(array.Capacity() == 10 && array.Live() == std::vector<uint32_t>{5,10,15,20,30});
  EraseRegistryEntry(array, 1);
  EffectRequire(array.Live() == std::vector<uint32_t>{5,15,20,30} && array.data[4] == 30);
  while (array.count) EraseRegistryEntry(array, 0);
  EffectRequire(array.Capacity() == 10 && array.allocations == 3);
  try { EraseRegistryEntry(array, 0); EffectRequire(false); } catch (const std::out_of_range &) {}
  ArrayFixture limited;
  limited.maximum = 1;
  AppendRegistryEntry(limited, 1u);
  try { AppendRegistryEntry(limited, 2u); EffectRequire(false); } catch (const std::length_error &) {}
  EffectRequire(limited.Live() == std::vector<uint32_t>{1} && limited.allocations == 1);
  ArrayFixture insert_limit;
  insert_limit.maximum = 1;
  try { InsertRegistryEntry(insert_limit, 0, 1u); EffectRequire(false); } catch (const std::length_error &) {}
  EffectRequire(insert_limit.count == 0 && insert_limit.allocations == 0);

  for (uint32_t mask = 0; mask < 8; ++mask) {
    RegistryFixture registry;
    registry.masks[1] = mask;
    RegisterEffectParticipant(registry, 1u);
    EffectRequire(registry.mask_calls == 3);
    for (uint32_t group = 0; group < 3; ++group) EffectRequire(registry.groups[group].count == ((mask >> group) & 1));
    UnregisterEffectParticipant(registry, 1u);
    EffectRequire(registry.mask_calls == 6 && registry.priority_calls == 0);
    for (auto &group : registry.groups) EffectRequire(group.count == 0);
  }
  RegistryFixture ordered;
  ordered.masks.fill(7);
  ordered.priorities[1] = std::numeric_limits<int32_t>::min();
  ordered.priorities[2] = ordered.priorities[3] = 0;
  ordered.priorities[4] = std::numeric_limits<int32_t>::max();
  for (uint32_t id : {1u,2u,3u,4u,2u}) RegisterEffectParticipant(ordered, id);
  for (auto &group : ordered.groups) EffectRequire(group.Live() == std::vector<uint32_t>{4,2,3,2,1});
  UnregisterEffectParticipant(ordered, 2u);
  for (auto &group : ordered.groups) EffectRequire(group.Live() == std::vector<uint32_t>{4,3,2,1});
  RegistryFixture live;
  live.masks[1] = live.masks[2] = 1;
  live.priorities[1] = 5;
  live.priorities[2] = 6;
  RegisterEffectParticipant(live, 2u);
  live.priority_call = [&](uint32_t id) {
    if (id == 1) { AppendRegistryEntry(live.groups[0], 3u); live.groups[0].Write(0, 2); }
  };
  live.mask_call = [&](uint32_t call) { if (call == 5) live.masks[1] = 2; };
  RegisterEffectParticipant(live, 1u);
  EffectRequire(live.groups[0].Live() == std::vector<uint32_t>{2,3,1}); // snapshot scan, live append
  EffectRequire(live.groups[1].Live() == std::vector<uint32_t>{1} && live.groups[2].count == 0);
  RegistryFixture failed;
  failed.masks[1] = 7;
  failed.mask_call = [](uint32_t call) { if (call == 2) throw 17; };
  try { RegisterEffectParticipant(failed, 1u); EffectRequire(false); } catch (int) {}
  EffectRequire(failed.groups[0].Live() == std::vector<uint32_t>{1} && failed.groups[1].count == 0);
}
} // namespace
