#pragma once
#include "gpu/scene/native_effect_lifecycle.h"
#include <array>
#include <functional>
#include <stdexcept>
#include <vector>
namespace {
inline void LifecycleRequire(bool valid) { if (!valid) throw std::runtime_error("Effect lifecycle test failed"); }
struct PreparationFixture {
  int32_t count = 3;
  std::array<uint32_t, 4> slots{0, 1, 2, 3};
  std::array<int32_t, 4> results{};
  std::array<uint8_t, 4> active{};
  uint32_t input_resource = 8, resource = 9;
  std::vector<int> events;
  std::function<void(int32_t)> begin, end;
  std::function<void()> resource_begin, resource_end;
  int32_t Count() { events.push_back(1); return count; }
  int32_t Begin(int32_t i) {
    events.push_back(10 + i);
    const auto result = results.at(slots.at(i));
    if (begin) begin(i);
    return result;
  }
  void End(int32_t i) { events.push_back(20 + i); if (end) end(i); }
  uint8_t Active(int32_t i) { return active.at(slots.at(i)); }
  void SetActive(int32_t i, uint8_t value) {
    events.push_back((value ? 30 : 40) + i);
    active.at(slots.at(i)) = value;
  }
  uint32_t InputResource() { events.push_back(50); return input_resource; }
  uint32_t Resource() { events.push_back(51); return resource; }
  void SetResource(uint32_t value) { resource = value; events.push_back(value ? 52 : 53); }
  void BeginResource(uint32_t value) {
    LifecycleRequire(value == resource);
    events.push_back(54);
    if (resource_begin) resource_begin();
  }
  void EndResource(uint32_t value) {
    LifecycleRequire(value == resource);
    events.push_back(55);
    if (resource_end) resource_end();
  }
};
struct RegistryLifetimeFixture {
  std::array<uint32_t, 3> storage{10, 0, 30}, capacity{4, 0, 8}, count{2, 0, 7};
  uint32_t resource = 99;
  std::vector<int> events;
  std::function<void(uint32_t)> freeing;
  uint32_t Storage(uint32_t group) { events.push_back(10 + int(group)); return storage.at(group); }
  void Free(uint32_t data) { events.push_back(int(data)); if (freeing) freeing(data); }
  void SetStorage(uint32_t group, uint32_t data) { events.push_back(40 + int(group)); storage.at(group) = data; }
  void SetCapacity(uint32_t group, uint32_t value) { events.push_back(50 + int(group)); capacity.at(group) = value; }
  void SetCount(uint32_t group, uint32_t value) { events.push_back(60 + int(group)); count.at(group) = value; }
};
void CheckEffectLifecycle() {
  // Every three-participant combination of defined and nonstandard results.
  const std::array<int32_t, 8> statuses{0, 1, 2, 3, -1, INT32_MIN, INT32_MAX, 0x101};
  for (auto first : statuses) for (auto second : statuses) for (auto third : statuses) {
    for (bool model : {false, true}) {
      PreparationFixture fixture;
      fixture.results = {first, second, third, 0};
      fixture.active.fill(255); // rejected entries do not clear prior state
      std::vector<int> expected{1};
      std::array<uint8_t, 4> flags{255, 255, 255, 255};
      int32_t result = 0;
      for (int32_t i = 0; i < 3; ++i) {
        expected.push_back(10 + i);
        result = fixture.results[i];
        if (result == 3) break;
        if (result == 1 || result == 2) {
          expected.push_back(30 + i);
          flags[i] = 1;
          if (result == 2) break;
          result = 2;
        }
      }
      if (model && result != 3) expected.insert(expected.end(), {50, 52, 54});
      const auto actual = model ? PrepareEffectModel(fixture) : PrepareEffectParticipants(fixture);
      LifecycleRequire(actual == result && fixture.events == expected && fixture.active == flags);
      LifecycleRequire(fixture.resource == (model && result != 3 ? 8u : 9u));
    }
  }
  for (int32_t count : {0, -1, INT32_MIN}) {
    PreparationFixture empty;
    empty.count = count;
    empty.input_resource = 0;
    LifecycleRequire(PrepareEffectModel(empty) == 0);
    LifecycleRequire(empty.resource == 0 && empty.events == std::vector<int>{1, 50, 53});
    empty.events.clear();
    FinishEffectModel(empty);
    LifecycleRequire(empty.events == std::vector<int>{51, 1});
  }
  for (int32_t accepted : {1, 2}) {
    PreparationFixture live;
    live.results[0] = accepted;
    live.begin = [&](int32_t i) { if (!i) { live.slots[0] = 3; live.count = 0; live.input_resource = 7; } };
    LifecycleRequire(PrepareEffectModel(live) == (accepted == 1 ? 0 : 2));
    LifecycleRequire(live.active[0] == 0 && live.active[3] == 1 && live.resource == 7);
    LifecycleRequire(live.events == (accepted == 1 ? std::vector<int>{1,10,30,11,12,50,52,54}
                                                 : std::vector<int>{1,10,30,50,52,54}));
  }
  PreparationFixture grow;
  grow.count = 1;
  grow.begin = [&](int32_t) { grow.count = 4; };
  PrepareEffectParticipants(grow);
  LifecycleRequire(grow.events == std::vector<int>{1, 10});
  for (uint32_t byte = 0; byte < 256; ++byte) {
    PreparationFixture finish;
    finish.count = 1;
    finish.active[0] = uint8_t(byte);
    FinishEffectParticipants(finish);
    LifecycleRequire(finish.events == (byte == 1 ? std::vector<int>{1,20,40} : std::vector<int>{1}));
    LifecycleRequire(finish.active[0] == (byte == 1 ? 0 : byte));
  }
  PreparationFixture finish;
  finish.count = 0;
  finish.resource_end = [&] { finish.count = 2; finish.resource = 17; };
  finish.active = {1, 1, 0, 1};
  finish.end = [&](int32_t i) { if (!i) { finish.count = 0; finish.slots[0] = 3; } };
  FinishEffectModel(finish);
  LifecycleRequire(finish.events == std::vector<int>{51,55,53,1,20,40,21,41});
  LifecycleRequire(finish.resource == 0 && finish.active == std::array<uint8_t,4>{1,0,0,0});
  // A callback failure propagates without replay, implicit finish or clearing.
  PreparationFixture failure;
  failure.results[0] = 1;
  failure.begin = [](int32_t i) { if (i == 1) throw 17; };
  try { PrepareEffectModel(failure); LifecycleRequire(false); } catch (int) {}
  LifecycleRequire(failure.resource == 9 && failure.events == std::vector<int>{1,10,30,11});
  PreparationFixture resource_failure;
  resource_failure.resource_begin = [] { throw 17; };
  try { PrepareEffectModel(resource_failure); LifecycleRequire(false); } catch (int) {}
  LifecycleRequire(resource_failure.resource == 8 && resource_failure.events.back() == 54);
  resource_failure.events.clear();
  resource_failure.resource_end = [] { throw 17; };
  try { FinishEffectModel(resource_failure); LifecycleRequire(false); } catch (int) {}
  LifecycleRequire(resource_failure.resource == 8 && resource_failure.events == std::vector<int>{51,55});
  PreparationFixture finish_failure;
  finish_failure.active[0] = 1;
  finish_failure.end = [](int32_t) { throw 17; };
  try { FinishEffectParticipants(finish_failure); LifecycleRequire(false); } catch (int) {}
  LifecycleRequire(finish_failure.active[0] == 1 && finish_failure.events == std::vector<int>{1,20});

  RegistryLifetimeFixture lifetime;
  lifetime.freeing = [&](uint32_t data) { if (data == 30) lifetime.storage[0] = 11; };
  DestroyEffectRegistry(lifetime);
  LifecycleRequire(lifetime.events == std::vector<int>{12,30,42,52,62,11,51,61,10,11,40,50,60});
  LifecycleRequire(lifetime.storage == std::array<uint32_t,3>{} && lifetime.capacity == lifetime.storage &&
                   lifetime.count == lifetime.storage && lifetime.resource == 99);
  lifetime.events.clear();
  DestroyEffectRegistry(lifetime); // repeated teardown does not free again
  LifecycleRequire(lifetime.events == std::vector<int>{12,52,62,11,51,61,10,50,60});
  RegistryLifetimeFixture failed_free;
  failed_free.freeing = [](uint32_t) { throw 17; };
  try { DestroyEffectRegistry(failed_free); LifecycleRequire(false); } catch (int) {}
  LifecycleRequire(failed_free.storage[2] == 30 && failed_free.capacity[2] == 8 && failed_free.count[2] == 7);
}
} // namespace
