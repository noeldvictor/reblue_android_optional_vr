/**
 * @brief Live deferred-visual scheduling and failure-boundary CPU fixtures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_deferred_visuals.h"
#include <array>
#include <cassert>
#include <functional>
#include <vector>

namespace deferred_visuals_test {
struct Port {
  std::vector<uint32_t> events;
  std::array<uint32_t, 512> flags{};
  uint32_t count = 0;
  bool snapshot = true;
  std::function<void(uint32_t)> changed;
  void Event(uint32_t event) { events.push_back(event); if (changed) changed(event); }
  bool Snapshot() { Event(1); return snapshot; }
  void Begin() { Event(2); }
  uint32_t Count() { return count; }
  uint32_t Flags(uint32_t index) { Event(1000 + index); return flags.at(index); }
  void SelectMode(uint32_t mode) { Event(mode); }
  void Primitive(uint32_t index, bool translated) { Event(2000 + index * 2 + translated); }
  void End() { Event(3); }
  void Clear() { count = 0; Event(4); }
};
inline void Run() {
  using bd::gpu::scene::ExecuteDeferredVisuals;
  Port empty;
  assert(ExecuteDeferredVisuals(empty, 0) && empty.events.empty());
  assert(!ExecuteDeferredVisuals(empty, 513) && empty.events.empty());
  Port refused; refused.snapshot = false; refused.count = 2;
  assert(!ExecuteDeferredVisuals(refused, refused.count));
  assert(refused.events == std::vector<uint32_t>{1} && refused.count == 2);
  // Independent original branch order for every low 20-bit flag word. Begin
  // supplies mode 5: first flagged primitive must not repeat shader startup.
  for (uint32_t flags = 0; flags < (1u << 20); ++flags) {
    Port port; port.count = 1; port.flags[0] = flags;
    std::vector<uint32_t> expected{1, 2, 1000};
    if ((flags & 0x40000) != 0x40000) expected.push_back(6);
    expected.push_back((flags & 0x40000) == 0x40000 ? 2000 : 2001);
    expected.insert(expected.end(), {3, 4});
    assert(ExecuteDeferredVisuals(port, port.count) && port.events == expected && !port.count);
  }
  Port full; full.count = 512;
  for (uint32_t i = 0; i < 512; ++i) full.flags[i] = (i & 1) ? 0x80000 : 0xc0000;
  assert(ExecuteDeferredVisuals(full, full.count));
  assert(full.events.size() == 4 + 512 * 2 + 511 && !full.count);
  assert(full.events[2] == 1000 && full.events[3] == 2000);
  Port live; live.count = 1; live.flags[0] = 0;
  live.changed = [&](uint32_t event) {
    if (event == 2) live.count = 2;
    if (event == 6) live.flags[1] = 0x40000;
    if (event == 2002) live.count = 1;
  };
  assert(ExecuteDeferredVisuals(live, 1));
  assert(live.events == (std::vector<uint32_t>{1, 2, 1000, 6, 2001, 1001, 5, 2002, 3, 4}));
  Port removed; removed.count = 1;
  removed.changed = [&](uint32_t event) { if (event == 2) removed.count = 0; };
  assert(ExecuteDeferredVisuals(removed, 1));
  assert(removed.events == (std::vector<uint32_t>{1, 2, 3, 4}));
  Port overflow; overflow.count = 1;
  overflow.changed = [&](uint32_t event) { if (event == 2) overflow.count = 513; };
  try { ExecuteDeferredVisuals(overflow, 1); assert(false); } catch (const std::runtime_error &) {}
  assert(overflow.events == (std::vector<uint32_t>{1, 2}) && overflow.count == 513);
  for (uint32_t fail : {1u, 2u, 6u, 2001u, 3u}) {
    Port broken; broken.count = 1;
    broken.changed = [fail](uint32_t event) { if (event == fail) throw 1; };
    try { ExecuteDeferredVisuals(broken, 1); assert(false); } catch (int) {}
    assert(broken.events.back() == fail && broken.count == 1); // no clear/replay while unwinding
  }
}
} // namespace deferred_visuals_test
