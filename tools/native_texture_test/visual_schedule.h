/**
 * @brief Independent linked-order oracle and callback-sensitive schedule tests.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_visual_schedule.h"
#include "gpu/scene/visual_schedule_import.h"
#include <bit>
#include <cassert>
#include <random>
#include <vector>

namespace visual_schedule_test {
using namespace bd::gpu::scene;
// Independent original-order oracle: float-rounded scalar operations, linked
// push-front entries, then descending buckets. Does not call VisualSortLayer.
inline uint32_t ReferenceLayer(VisualSortInput input, VisualSortRange range, bool model) {
  if (input.flags & 0x10000) return 1023;
  if (input.flags & 0x20000) return 0;
  volatile float difference = double(range.upper) - double(range.lower);
  const double extent = difference == range.zero ? range.one : difference;
  volatile float numerator = double(input.depth) - double(range.lower);
  volatile float divided = double(numerator) / extent;
  double t = divided;
  if (model) {
    if (t < range.zero) t = range.zero;
    if (t > range.one) t = range.one;
  } else {
    if (t > range.zero) { if (!(t < range.one)) t = range.one; }
    else t = range.zero;
  }
  volatile float inverse = double(range.one) - t;
  volatile float product = double(inverse) * double(range.scale);
  const double truncated = std::trunc(double(product));
  uint32_t converted = 0x80000000;
  if (truncated >= INT32_MAX) converted = INT32_MAX;
  else if (truncated >= INT32_MIN) converted = uint32_t(int32_t(truncated));
  const uint32_t offset = (1u - converted) << 2;
  return offset / 4;
}
inline void Order() {
  NativeVisualOrder order;
  std::mt19937 random(0x82424af8);
  std::vector<VisualSortInput> inputs;
  uint64_t compared = 0;
  for (bool model : {false, true}) {
    const uint32_t limit = model ? kSortedModelLimit : kSortedPrimitiveLimit;
    for (uint32_t trial = 0; trial < 160; ++trial) {
      const uint32_t count = trial == 0 ? limit : random() % (limit + 1);
      inputs.resize(count);
      VisualSortRange range{-35, trial % 3 ? 80.f : -35.f, 0, 1, -1021};
      for (auto &input : inputs) {
        input.flags = random();
        input.depth = trial & 1 ? std::bit_cast<float>(uint32_t(random())) : float(int(random() % 500) - 200);
      }
      auto read = [&](uint32_t i) -> std::optional<VisualSortInput> { return inputs.at(i); };
      assert(order.Build(count, range, model, read));
      std::array<int32_t, 1024> heads;
      heads.fill(-1);
      std::vector<int32_t> next(count);
      for (uint32_t i = 0; i < count; ++i) {
        const auto layer = ReferenceLayer(inputs[i], range, model);
        assert(layer < heads.size());
        next[i] = heads[layer]; heads[layer] = int32_t(i);
      }
      // Changing producer inputs cannot change an already-owned host order.
      for (auto &input : inputs) input = {0, 0};
      size_t cursor = 0;
      for (int layer = 1023; layer >= 0; --layer) {
        for (int32_t i = heads[layer]; i != -1; i = next[i]) {
          assert(order.Keys()[cursor++] == (uint64_t(layer) << 32 | uint32_t(i)));
          ++compared;
        }
      }
      assert(cursor == count);
      assert(!order.Build(limit + 1, range, model, read) && order.Keys().empty());
      assert(!order.Build(1, range, model, [](uint32_t) -> std::optional<VisualSortInput> { return {}; }));
    }
  }
  assert(compared > 400000);
  const VisualSortRange range{0, 1, 0, 1, -1021};
  assert(VisualSortLayer({std::bit_cast<float>(0x7f812345u), 0}, range, true) == 1);
  assert(VisualSortLayer({std::bit_cast<float>(0x7f812345u), 0}, range, false) == 1022);
  assert(VisualSortLayer({0, 0x30000}, range, true) == 1023);
  for (float upper : {INFINITY, -INFINITY, std::bit_cast<float>(0x7fc00000u), 0.f})
    for (float depth : {INFINITY, -INFINITY, std::bit_cast<float>(0x7fc12345u), -0.f, 1.f})
      for (bool model : {false, true}) {
        const VisualSortRange unusual{0, upper, 0, 1, -1021};
        assert(VisualSortLayer({depth, 0}, unusual, model) == ReferenceLayer({depth, 0}, unusual, model));
      }
  assert(!order.Build(1, {0, 1, 0, 1, 2}, false,
      [](uint32_t) { return std::optional(VisualSortInput{0, 0}); }));
  assert(order.Keys().empty());
  for (uint32_t flags = 0; flags < 65536; ++flags) {
    const auto blend = ImportVisualBlend(flags | 0xc0000000u);
    const uint32_t mask = flags & 0xff00;
    uint32_t source = 6, destination = 7, mode = 0;
    if (mask == 256) { destination = 1; mode = 1; }
    if (mask == 512) { source = 0; destination = 4; mode = 2; }
    if (mask == 1024) { source = 0; mode = 4; }
    if (mask == 2048) { source = 9; destination = 1; mode = 3; }
    if (mask == 4096) { source = 0; destination = 5; mode = 5; }
    assert(blend.source == source && blend.destination == destination && blend.mode == mode);
  }
  assert(VisualScalarWord(0x7f812345) == 0x7fc12345);
  assert(VisualScalarWord(0xff800001) == 0xffc00001);
  assert(VisualScalarWord(0x80000000) == 0x80000000);
}

struct Port {
  std::vector<uint32_t> events, flags;
  uint32_t deferred = 510;
  bool model_scope = false, inject = false, mutate = false;
  void ModelScope(bool enable) { model_scope = enable; events.push_back(enable ? 100 : 101); }
  void Model(uint32_t index) {
    assert(model_scope); events.push_back(200 + index);
    if (inject) flags = {0, 0x10, 0x40000, 0x80000, 0x40000};
  }
  void BeginPrimitives() { assert(!model_scope); events.push_back(300); }
  uint32_t PrimitiveCount() { return uint32_t(flags.size()); }
  void PreparePrimitives() { events.push_back(301); }
  void SortPrimitives(NativeVisualOrder &order) {
    events.push_back(302);
    assert(order.Build(PrimitiveCount(), {0, 1, 0, 1, -1021}, false,
        [&](uint32_t index) { return std::optional(VisualSortInput{0, flags[index]}); }));
  }
  void PrepareSharedMaterial() { events.push_back(303); if (mutate) flags.back() = 0x10; }
  uint32_t DeferredCount() { return deferred; }
  uint32_t PrimitiveFlags(uint32_t index) { return flags.at(index); }
  void Defer(uint32_t index, uint32_t count) {
    assert(count == deferred && count < 512); ++deferred; events.push_back(400 + index);
  }
  void DeferredOverflow() { events.push_back(499); }
  void SelectMode(uint32_t mode) { events.push_back(500 + mode); }
  void Primitive(uint32_t index) {
    events.push_back(600 + index);
    if (mutate) deferred = 0; // callbacks may consume/reset the shared output
  }
  void ResetColour() { events.push_back(700); }
  void EndPrimitives() { events.push_back(701); }
};
inline void Schedule() {
  NativeVisualOrder order;
  Port port;
  ExecuteVisualSchedule(port, order, 0, 0);
  assert(port.events.empty());
  auto models = [&] {
    assert(order.Build(2, {0, 1, 0, 1, -1021}, true,
        [](uint32_t) { return std::optional(VisualSortInput{0, 0}); }));
  };
  models();
  ExecuteVisualSchedule(port, order, 2, 0);
  assert((port.events == std::vector<uint32_t>{100, 201, 200, 101, 300, 700, 701}));
  port = {}; port.inject = true; models();
  ExecuteVisualSchedule(port, order, 2, 0);
  assert((port.events == std::vector<uint32_t>{100, 201, 200, 101, 300, 301, 302, 303,
      404, 403, 499, 504, 601, 503, 600, 700, 701}));
  assert(port.deferred == 512);
  port = {}; port.flags = {0x40000, 0, 0x40000}; port.mutate = true; port.deferred = 512;
  ExecuteVisualSchedule(port, order, 0, 3);
  assert((port.events == std::vector<uint32_t>{300, 301, 302, 303, 504, 602, 503, 601, 400, 700, 701}));
  assert(port.deferred == 1);
  port = {}; port.flags = {0x40000, 0xc0000}; port.deferred = 0;
  ExecuteVisualSchedule(port, order, 0, 2);
  assert((port.events == std::vector<uint32_t>{300, 301, 302, 303, 401, 400, 700, 701}));
}
struct ModelPort {
  std::array<uint32_t, 26> entry{};
  std::array<uint32_t, 1241> visual{};
  std::vector<uint32_t> events;
  bool depth = false, mutate = false, alias = false;
  uint32_t Entry(uint32_t offset) { return entry.at(offset / 4); }
  void Store(uint32_t offset, uint32_t value) {
    visual.at(offset / 4) = value; events.push_back(offset);
    if (alias && offset == 4932) entry[24] = 0x7f800001;
  }
  uint32_t Table() { return visual[0]; }
  uint32_t Method(uint32_t table, uint32_t slot) { events.push_back(10000 + slot); return table + slot; }
  void Callback(uint32_t function) {
    events.push_back(function);
    if (mutate && function == 2020) { visual[0] = 4000; entry[23] = 0x1198; }
  }
  void CopyMatrix() { events.push_back(8000); }
  void InitBones() { events.push_back(8001); if (mutate) entry[19] = 0x7f812345; }
  void State(uint32_t field, uint32_t value) {
    events.push_back(9000 + field * 2 + value);
    if (mutate && field == 48) { entry[23] = 0x218; depth = true; }
  }
  bool DepthPolicy() { return depth; }
  uint32_t Blend(uint32_t flags) { events.push_back(11000 + flags); return ImportVisualBlend(flags).mode; }
};
inline void Models() {
  for (bool special : {false, true}) {
    ModelPort port;
    port.visual.fill(0xdeadbeef); port.visual[0] = 2000;
    port.entry[23] = special ? 0x100000 : 0;
    port.entry[24] = 0x7f812345; port.entry[25] = 0x80000000;
    for (uint32_t i = 0; i < 4; ++i) port.entry[19 + i] = 0x3f800000 + i;
    PrepareSortedVisualModel(port);
    assert(port.visual[3000 / 4] == (special ? 0u : 6u));
    for (uint32_t i = 0; i < (special ? 4u : 8u); ++i)
      assert(port.visual[(special ? 3444 : 4932) / 4 + i] == (i & 1 ? 0x80000000u : 0x7fc12345u));
    for (uint32_t i = 0; i < 4; ++i) assert(port.visual[3004 / 4 + i] == port.entry[19 + i]);
    assert(port.visual[1864 / 4] == 0 && port.visual[3040 / 4] == 0);
    assert(port.events.back() == 2004);
    assert(std::count(port.events.begin(), port.events.end(), 2020) == (special ? 0 : 1));
    assert(std::count(port.events.begin(), port.events.end(), 9081) == 0);
    assert(port.visual[(special ? 4932 : 3440) / 4] == 0xdeadbeef);
  }
  ModelPort port;
  port.visual[0] = 2000; port.mutate = true; port.alias = true;
  port.entry[24] = 0x3f800000;
  PrepareSortedVisualModel(port);
  assert(port.visual[4932 / 4] == 0x3f800000 && port.visual[4940 / 4] == 0x7fc00001);
  assert(port.visual[3004 / 4] == 0x7fc12345);
  assert(port.visual[3040 / 4] == 1 && port.visual[1864 / 4] == 2);
  assert(port.events.back() == 4004);
  assert(std::count(port.events.begin(), port.events.end(), 9096) == 1); // depth write false
  assert(std::count(port.events.begin(), port.events.end(), 9081) == 1); // live depth test true
  const auto matrix = std::find(port.events.begin(), port.events.end(), 8000);
  assert(matrix != port.events.end() && *(matrix - 1) == 2020 && *(matrix + 1) == 8001);
}
inline void Run() { Order(); Schedule(); Models(); }
} // namespace visual_schedule_test
