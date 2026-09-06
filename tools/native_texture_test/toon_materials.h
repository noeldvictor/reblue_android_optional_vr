/**
 * @brief Native Toon animation and edge-word fixtures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_toon_material.h"
#include <cassert>
#include <functional>
#include <stdexcept>
#include <vector>

namespace toon_material_test {
using namespace bd::gpu::scene;
inline uint32_t OriginalIndex(uint32_t value) {
  const int64_t high = (int64_t(std::bit_cast<int32_t>(value)) * 0x2aaaaaabLL) >> 32;
  return uint32_t(high + ((uint32_t(high) >> 31) & 1));
}
struct Port {
  std::array<uint32_t, 3> counters{};
  std::array<uint32_t, 2> bound{777, 888};
  std::vector<uint32_t> events;
  uint32_t list = 1, active = 1, offset = 0, count = 10, fallback = 999;
  std::function<void(uint32_t)> changed;
  void Event(uint32_t event) { events.push_back(event); if (changed) changed(event); }
  uint32_t Counter(uint32_t i) { return counters.at(i); }
  void SetCounter(uint32_t i, uint32_t value) { counters.at(i) = value; Event(100 + i); }
  uint32_t TextureList() { return list; }
  uint32_t ActiveList() { return active; }
  uint32_t ActiveOffset() { return offset; }
  uint32_t ImageCount(uint32_t) { return count; }
  uint32_t Image(uint32_t selected, uint32_t index) { assert(index < count); return selected * 1000 + index; }
  uint32_t FallbackImage() { return fallback; }
  void BindTexture(uint32_t slot, uint32_t image) { bound.at(slot - 6) = image; Event(slot); }
};
inline void Run() {
  for (uint32_t word : {0u, 1u, 5u, 6u, 17u, 18u, 20u, 21u, 0x7fffffffu,
                       0x80000000u, 0xffffffedu, 0xfffffffau, 0xffffffffu}) {
    assert(ToonFrameIndex(word) == OriginalIndex(word));
    for (int32_t limit : {18, 21}) {
      const uint32_t next = word + 1;
      assert(AdvanceToonCounter(word, limit) == (std::bit_cast<int32_t>(next) < limit ? next : 0));
    }
  }
  uint32_t seed = 0x746f6f6e;
  for (uint32_t i = 0; i < 100000; ++i) {
    seed = seed * 1664525 + 1013904223;
    assert(ToonFrameIndex(seed) == OriginalIndex(seed));
    volatile double wide = double(std::bit_cast<float>(seed));
    volatile float narrow = float(wide);
    assert(ToonFloatWord(seed) == std::bit_cast<uint32_t>(float(narrow)));
  }
  const std::array<uint32_t, 6> authored{0, 0x80000000u, 1, 0x7f800000u, 0xff800000u, 0x7f800123u};
  const auto edge = BuildToonEdgeParameters(authored, {0xff800456u, 0x3f800000u});
  assert(edge == (std::array<uint32_t, 8>{0, 0x80000000u, 1, 0x7f800000u,
      0xff800000u, 0x7fc00123u, 0xffc00456u, 0x3f800000u}));
  for (uint32_t a = 0; a < 18; ++a) for (uint32_t b = 0; b < 18; ++b)
    for (uint32_t c = 0; c < 21; ++c) {
      Port port; port.counters = {a, b, c}; UpdateToonMaterial(port);
      assert(port.bound == (std::array<uint32_t, 2>{1000 + a / 6, 1003 + b / 6}));
      assert(port.counters == (std::array<uint32_t, 3>{(a + 1) % 18, (b + 1) % 18, (c + 1) % 21}));
      assert(port.events == (std::vector<uint32_t>{6, 7, 100, 101, 102}));
    }
  for (uint32_t list : {0u, 1u, 2u}) for (uint32_t offset : {0u, 3u, 10u, 0xffffffffu})
    for (uint32_t index : {0u, 3u, 9u, 10u, 0xffffffffu}) {
      Port port; port.list = list; port.offset = offset;
      const uint32_t selected = index + (list == port.active ? offset : 0);
      assert(SelectToonImage(port, index) == (!list ? 0 : selected < port.count ? list * 1000 + selected : port.fallback));
    }
  Port live;
  live.changed = [&](uint32_t event) {
    if (event == 6) { live.list = 2; live.counters[1] = 12; }
    if (event == 100) live.counters[1] = 17;
  };
  UpdateToonMaterial(live);
  assert(live.bound == (std::array<uint32_t, 2>{1000, 2005}));
  assert(live.counters == (std::array<uint32_t, 3>{1, 0, 1}));
  Port failed;
  failed.changed = [](uint32_t event) { if (event == 7) throw std::runtime_error("binding failed"); };
  try { UpdateToonMaterial(failed); assert(false); } catch (const std::runtime_error &) {}
  assert(failed.events == (std::vector<uint32_t>{6, 7}));
  assert(failed.counters == (std::array<uint32_t, 3>{0, 0, 0}));
}
} // namespace toon_material_test
