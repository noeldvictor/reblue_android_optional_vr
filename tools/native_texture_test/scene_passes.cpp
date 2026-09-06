/**
 * @file    scene_passes.cpp
 * @brief   Full-size native scene policy, overflow refusal and authored exposure.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_scene_pass.h"
#include "gpu/scene/native_scene_result.h"
#include "gpu/scene/scene_precision_import.h"
#include "gpu/native_image_layers.h"
#include <array>
#include <stdexcept>
using namespace bd::gpu::scene;
namespace {
void Require(bool value) {
  if (!value) throw std::runtime_error("native scene policy check failed");
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
