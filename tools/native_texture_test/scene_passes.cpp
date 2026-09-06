/**
 * @file    scene_passes.cpp
 * @brief   Full-size native scene policy, overflow refusal and authored exposure.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_scene_pass.h"
#include "gpu/scene/native_scene_result.h"
#include "gpu/scene/scene_precision_import.h"
#include "gpu/scene/native_pass_dispatch.h"
#include "gpu/scene/pass_dispatch_import.h"
#include "gpu/scene/native_view_schedule.h"
#include "gpu/scene/view_schedule_geometry.h"
#include "effect_activation_cases.h"
#include "gpu/native_image_layers.h"
#include <array>
#include <functional>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace bd::gpu::scene;
namespace {
void Require(bool value) {
  if (!value) throw std::runtime_error("native scene policy check failed");
}
struct DispatchAdapter {
  int32_t count = 3;
  std::array<unsigned, 3> slots{0, 1, 2};
  std::array<uint32_t, 4> results{0x101, 2, 0, 1};
  std::array<uint8_t, 4> active{0, 1, 255, 0};
  std::vector<int> events;
  std::function<void()> open;
  std::function<void(uint32_t)> begin, end;
  int32_t ParticipantCount() { events.push_back(2); return count; }
  void PublishMode() { events.push_back(0); }
  void BeginPass() { events.push_back(1); if (open) open(); }
  void EndPass() { events.push_back(3); }
  bool BeginParticipant(uint32_t i) {
    events.push_back(10 + int(i));
    const auto result = results[slots[i]];
    if (begin) begin(i);
    return ImportParticipantAccepted(result);
  }
  bool ParticipantActive(uint32_t i) {
    return ImportParticipantActive(active[slots[i]]);
  }
  void EndParticipant(uint32_t i) {
    events.push_back(30 + int(i));
    if (end) end(i);
  }
  void SetParticipantActive(uint32_t i, bool value) {
    events.push_back((value ? 20 : 40) + int(i));
    active[slots[i]] = uint8_t(value);
  }
};
void CheckDispatch() {
  // Both imported booleans require exactly one, not arbitrary nonzero.
  for (uint32_t byte = 0; byte < 256; ++byte) {
    Require(ImportParticipantActive(uint8_t(byte)) == (byte == 1));
    for (uint32_t high : {0u, 0xFFFFFF00u, 0x12345600u})
      Require(ImportParticipantAccepted(high | byte) == (byte == 1));
  }
  for (uint32_t phase = 0; phase < 16; ++phase)
    Require(ImportPassLightSpace(phase) == (phase == 1 || phase == 4 || phase == 8));
  Require(!ImportPassLightSpace(UINT32_MAX));

  DispatchAdapter basic;
  DispatchPassBegin(basic);
  Require(basic.events == std::vector<int>{0, 1, 2, 10, 20, 11, 12});
  // Refused participants retain any existing flag; begin does not clear them.
  Require(basic.active == std::array<uint8_t, 4>{1, 1, 255, 0});
  basic.events.clear();
  DispatchPassEnd(basic);
  Require(basic.events == std::vector<int>{2, 30, 40, 31, 41, 3});
  Require(basic.active == std::array<uint8_t, 4>{0, 0, 255, 0});
  basic.events.clear();
  DispatchPassEnd(basic);
  Require(basic.events == std::vector<int>{2, 3});

  for (int32_t count : {0, -1, INT32_MIN}) {
    DispatchAdapter empty;
    empty.count = count;
    DispatchPassBegin(empty);
    DispatchPassEnd(empty);
    Require(empty.events == std::vector<int>{0, 1, 2, 2, 3});
  }
  DispatchAdapter mutated;
  mutated.count = 0;
  mutated.open = [&] { mutated.count = 3; }; // begin creates the registry
  mutated.begin = [&](uint32_t i) {
    if (!i) {
      mutated.count = 1; // cannot shorten the captured loop bound
      mutated.slots = {3, 2, 1}; // but individual slots remain live
    }
  };
  DispatchPassBegin(mutated);
  Require(mutated.events == std::vector<int>{0, 1, 2, 10, 20, 11, 12});
  Require(mutated.active == std::array<uint8_t, 4>{0, 1, 255, 1});
  mutated.count = 3;
  mutated.events.clear();
  mutated.end = [&](uint32_t i) {
    if (!i) {
      mutated.count = 0;
      mutated.slots = {1, 2, 3};
    }
  };
  DispatchPassEnd(mutated);
  Require(mutated.events == std::vector<int>{2, 30, 40, 32, 42, 3});
  Require(mutated.active == std::array<uint8_t, 4>{0, 0, 255, 0});

  // Nested schedules keep independent iteration state; no global loop cursor.
  DispatchAdapter outer, inner;
  outer.begin = [&](uint32_t i) {
    if (!i) { DispatchPassBegin(inner); DispatchPassEnd(inner); }
  };
  DispatchPassBegin(outer);
  DispatchPassEnd(outer);
  Require(outer.events == inner.events);
  Require(outer.active == inner.active);

  // A callback fault must not trigger a second execution or an early pass end.
  DispatchAdapter failed;
  failed.active[0] = 1;
  failed.end = [](uint32_t) { throw 1; };
  try { DispatchPassEnd(failed); Require(false); } catch (int) {}
  Require(failed.events == std::vector<int>{2, 30} && failed.active[0] == 1);
}
struct ViewScheduleAdapter {
  uint32_t flags = 0;
  int32_t count = 3;
  std::vector<int> events;
  std::function<void(int)> changed;
  void Event(int value) { events.push_back(value); if (changed) changed(value); }
  void PrepareView() { Event(0); }
  int32_t IndexedViewCount() { return count; }
  bool IndexedViewEnabled(uint32_t index) { return index != 1; }
  void RenderIndexedView(uint32_t index) { Event(100 + int(index)); }
  void SelectPrimaryView() { Event(2); }
  bool SunShadowRequested() { return flags & 1; }
  bool CubeShadowRequested() { return flags & 2; }
  bool AuxiliaryRequested() { return flags & 4; }
  bool ShadowVolumeRequested() { return flags & 8; }
  bool ReflectionsRequested() { return flags & 16; }
  bool EnvironmentRequested() { return flags & 32; }
  bool AdditionalSceneRequested() { return flags & 64; }
  bool PostRequested() { return flags & 128; }
  void RenderSunShadow() { Event(3); }
  void RenderCubeShadow() { Event(4); }
  void RenderAuxiliary() { Event(5); }
  void RenderShadowVolume() { Event(6); }
  void RenderReflections() { Event(7); }
  void RenderEnvironment() { Event(8); }
  void RenderAdditionalScene() { Event(9); }
  void RenderMainScene() { Event(10); }
  void RenderPost() { Event(11); }
  void RestoreView() { Event(12); }
};
void CheckViewSchedule() {
  for (uint32_t flags = 0; flags < 256; ++flags) {
    ViewScheduleAdapter adapter;
    adapter.flags = flags;
    ScheduleRenderView(adapter);
    std::vector<int> expected{0, 100, 102, 2};
    for (uint32_t bit = 0; bit < 7; ++bit)
      if (flags & (1u << bit)) expected.push_back(3 + int(bit));
    expected.push_back(10);
    if (flags & 128) expected.push_back(11);
    expected.push_back(12);
    Require(adapter.events == expected);
  }
  ViewScheduleAdapter live;
  live.flags = 1;
  live.changed = [&](int event) {
    if (event == 100) live.count = 1; // indexed bound is live, unlike participant bound
    if (event == 3) live.flags = 2; // a preceding pass can request a later pass
    if (event == 10) live.flags = 128; // post decision follows main-scene execution
  };
  ScheduleRenderView(live);
  Require(live.events == std::vector<int>{0, 100, 2, 3, 4, 10, 11, 12});
  for (int32_t count : {0, -1, INT32_MIN}) {
    ViewScheduleAdapter empty;
    empty.count = count;
    ScheduleRenderView(empty);
    Require(empty.events == std::vector<int>{0, 2, 10, 12});
  }
  ViewScheduleAdapter failed;
  failed.count = 0;
  failed.changed = [](int event) { if (event == 10) throw 1; };
  try { ScheduleRenderView(failed); Require(false); } catch (int) {}
  Require(failed.events == std::vector<int>{0, 2, 10}); // no parent replay or post after failure

  const auto ray = BuildScheduledViewRay({0,0,0}, {3,4,0}, -1e-6f, 1e-6f);
  Require(ray.length == 5 && ray.direction == std::array<float,3>{0.6f,0.8f,0});
  Require(ScheduledFocusPoint(ray, {3,4,0}, 10, 20) == std::array<float,3>{12,16,0});
  Require(ScheduledFocusPoint(ray, {3,4,0}, 5, 20) == std::array<float,3>{3,4,0});
  const auto zero = BuildScheduledViewRay({1,2,3}, {1,2,3}, -1e-6f, 1e-6f);
  Require(zero.length == 0 && zero.direction == std::array<float,3>{0,0,0});
  Require(ScheduledFocusPoint(zero, {1,2,3}, 10, 20) == std::array<float,3>{1,2,3});
  const auto tiny = BuildScheduledViewRay({0,0,0}, {1e-7f,0,0}, -1e-6f, 1e-6f);
  Require(tiny.direction[0] == 1e-7f);
  const float nan = std::numeric_limits<float>::quiet_NaN();
  Require(std::isnan(BuildScheduledViewRay({0,0,0}, {nan,1,0}, -1e-6f, 1e-6f).length));
  const RenderMatrix world{2,0,0,0, 0,3,0,0, 0,0,4,0, 10,20,30,1};
  Require(TransformReflectionSphere({1,2,3,7}, world) == std::array<float,4>{12,26,42,7});
  RenderFrustum frustum;
  frustum.planes[0] = {1,0,0,0};
  Require(ReflectionSphereVisible({1,0,0,1}, frustum)); // tangent survives
  Require(!ReflectionSphereVisible({1.001f,0,0,1}, frustum));
  Require(ReflectionSphereVisible({nan,0,0,1}, frustum));
  frustum.planes[5] = {0,0,1,-10};
  Require(!ReflectionSphereVisible({0,0,12,1}, frustum)); // checks all six planes
}
struct Lease {
  int *live = nullptr;
  int value = 0;
  Lease(int &count, int id) : live(&count), value(id) { ++*live; }
  Lease(const Lease &) = delete;
  Lease &operator=(const Lease &) = delete;
  Lease(Lease &&other) noexcept
      : live(std::exchange(other.live, nullptr)), value(other.value) {}
  Lease &operator=(Lease &&other) noexcept {
    if (this != &other) {
      if (live) --*live;
      live = std::exchange(other.live, nullptr);
      value = other.value;
    }
    return *this;
  }
  ~Lease() { if (live) --*live; }
};
void CheckResults() {
  int live = 0;
  SceneResultSlot<Lease> outer;
  Require(!outer.Take(0));
  outer.Complete(7, Lease(live, 10));
  Require(live == 1);
  { // nested views have separate result slots, not a global last-image cache
    SceneResultSlot<Lease> inner;
    inner.Complete(7, Lease(live, 20));
    Require(live == 2);
    auto result = inner.Take(7);
    Require(result && result->value == 20 && live == 2);
    Require(!inner.Take(7));
  }
  Require(live == 1);
  auto result = outer.Take(7);
  Require(result && result->value == 10 && live == 1);
  Require(!outer.Take(7));
  result.reset();
  Require(live == 0);
  outer.Complete(8, Lease(live, 30));
  outer.Complete(8, Lease(live, 40));
  Require(live == 1); // replaced result was released
  Require(!outer.Take(9) && live == 0); // stale frame cannot escape its owner
  outer.Complete(UINT64_MAX, Lease(live, 50));
  Require(!outer.Take(0) && live == 0); // rollover is not a matching frame
  outer.Complete(10, Lease(live, 60));
  outer.Clear(); // new scene begin invalidates even an unconsumed result
  Require(live == 0 && !outer.Take(10));
  try {
    SceneResultSlot<Lease> interrupted;
    interrupted.Complete(11, Lease(live, 70));
    throw 1;
  } catch (int) {}
  Require(live == 0); // disabled/aborted post releases ownership on scope exit
}
}
int main() {
  CheckEffectActivation();
  CheckViewSchedule();
  CheckDispatch();
  CheckResults();
  // Actual getter adapter: only the final cached/requested precision words
  // change. No transient off state, resource header, packet or dirty-bit write.
  constexpr uint32_t device = 0x10000000;
  for (uint32_t initial : {0u, 1u, 2u, UINT32_MAX}) {
    std::array<uint32_t, 0x5000 / 4> words;
    words.fill(initial);
    uint32_t cached = initial;
    for (int repeat = 0; repeat < 2; ++repeat) {
      uint32_t writes = 0;
      PublishScenePrecisionGetters(device, [&](uint32_t address, uint32_t value) {
        Require(value == 1 && writes < 2);
        if (writes++ == 0) {
          Require(address == device + 11756);
          words[(address - device) / 4] = value;
        } else {
          Require(address == 0x82DBE2DC);
          cached = value;
        }
      });
      Require(writes == 2 && cached == 1);
      for (uint32_t i = 0; i < words.size(); ++i)
        Require(words[i] == (i == 11756 / 4 ? 1u : initial));
    }
  }
  for (uint32_t flags = 0; flags < 64; ++flags) {
    const bool cube = flags & 1, volume = flags & 2;
    const bool color = flags & 4, depth = flags & 8;
    const bool multiview = flags & 16, layered = flags & 32;
    const uint32_t expected = (!cube && !volume && (color || depth) &&
                               multiview && layered) ? 2u : 1u;
    Require(bd::gpu::AttachmentTextureLayers(cube, volume, color, depth,
                                             multiview, layered) == expected);
  }
  Require(bd::gpu::AttachmentTextureLayers(false, false, false, true, true, true) == 2);
  Require(bd::gpu::AttachmentTextureLayers(true, false, false, true, true, true) == 1);
  Require(ScaleSceneExtent({1920, 1080}, 1, 100) == SceneExtent{1920, 1080});
  Require(ScaleSceneExtent({1440, 1584}, 1, 100) == SceneExtent{1440, 1584});
  Require(ScaleSceneExtent({1440, 1440}, 1, 100) == SceneExtent{1440, 1440});
  Require(ScaleSceneExtent({1920, 1080}, 2, 100) == SceneExtent{3840, 2160});
  Require(ScaleSceneExtent({1920, 1080}, 2, 50) == SceneExtent{1920, 1080});
  Require(ScaleSceneExtent({1, 1}, 0, 1) == SceneExtent{1, 1});
  Require(ScaleSceneExtent({1920, 1080}, -1, 101) == SceneExtent{1920, 1080});
  Require(!ScaleSceneExtent({0, 1584}, 1, 100));
  Require(!ScaleSceneExtent({1440, 0}, 1, 100));
  Require(!ScaleSceneExtent({1440, 1584}, 1, 0));
  Require(!ScaleSceneExtent({1440, 1584}, 1, -1));
  Require(!ScaleSceneExtent({UINT32_MAX, 1}, 2, 100));
  Require(!ScaleSceneExtent({1, UINT32_MAX}, 2, 100));
  Require(!ScaleSceneExtent({UINT32_MAX, 1}, 1, 50));
  Require(ScaleSceneExtent({UINT32_MAX, 1}, 1, 100)->width == UINT32_MAX);
  Require(SceneClearColor(0x12345678, true) == 0xFF345678);
  Require(SceneClearColor(0x12345678, false) == 0x12345678);
  Require(SceneColorWriteMask(true) == 7 && SceneColorWriteMask(false) == 15);
  Require(SceneOutputExposure(true) == 0.25f && SceneOutputExposure(false) == 1.0f);
  return 0;
}
