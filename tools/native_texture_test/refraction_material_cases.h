// CPU-only water/refraction contracts. Included after the test's NDEBUG override.
#pragma once
#include "gpu/scene/native_refraction_material.h"
#include "gpu/scene/refraction_material_import.h"
#include <bit>
#include <cmath>
#include <map>
#include <stdexcept>

namespace refraction_material_tests {
using namespace bd::gpu::scene;
struct Calls {
  std::vector<int> events;
  bool enabled = false, enable_on_bind = false, fail_flush = false;
  int first = 0, second = 0;
  void PublishSceneFactor() { events.push_back(1); first = 2; }
  void FlushWaterParameters(uint32_t index) {
    events.push_back(index ? second : first);
    second = 3; // the later descriptor must be read after the first flush
    if (fail_flush) throw std::runtime_error("test parameter failure");
  }
  void EnableSourceAlphaBlending() { events.push_back(4); }
  void EnableDepthTest() { events.push_back(5); }
  void BindPlanarReflection() { events.push_back(6); }
  void BindSceneImage() { events.push_back(7); enabled = enable_on_bind; }
  bool WantsSnapshot() { events.push_back(8); return enabled; }
  void Snapshot() { events.push_back(9); }
  void FlushRefractionParameters() { events.push_back(10); }
};
struct Words {
  std::map<uint64_t, uint32_t> values;
  std::vector<uint64_t> reads;
  std::optional<uint32_t> Read(uint64_t address) {
    reads.push_back(address);
    if (!address || (address & 3) || address > UINT32_MAX - 3) return {};
    const auto it = values.find(address);
    return it == values.end() ? std::nullopt : std::optional(it->second);
  }
  auto Reader() { return [this](uint64_t address) { return Read(address); }; }
};
inline void SequenceAndScalars() {
  for (bool initially_enabled : {false, true}) for (bool after_bind : {false, true}) {
    Calls calls;
    calls.enabled = initially_enabled; calls.enable_on_bind = after_bind;
    PrepareWaterMaterial(calls);
    std::vector<int> expected{1, 2, 3, 4, 5, 6, 7, 8};
    if (after_bind) expected.push_back(9);
    assert(calls.events == expected);
  }
  Calls refract;
  PrepareRefractionMaterial(refract);
  assert((refract.events == std::vector<int>{10, 9})); // unconditional, no water state reset
  Calls failed;
  failed.fail_flush = true;
  bool threw = false;
  try { PrepareWaterMaterial(failed); } catch (const std::runtime_error &) { threw = true; }
  assert(threw && (failed.events == std::vector<int>{1, 2})); // no replay/later side effects
  for (bool scene : {false, true}) for (bool force : {false, true})
    for (int32_t authored : {INT32_MIN, -1, 0, 1, 2, INT32_MAX}) {
      const float expected = scene && (force || authored != 1) ? float(double(authored)) : .375f;
      assert(WaterSceneFactor(scene, force, authored, .375f) == expected);
    }
  assert(WaterSceneFactor(true, false, 1) == 1.f);
  for (float before : {-INFINITY, -2.f, -0.f, 0.f, .5f, 1.f, 1000.f, INFINITY}) {
    const float after = ClampWaterHighlight(before);
    if (before > 1.f) assert(after == 1.f);
    else assert(std::bit_cast<uint32_t>(after) == std::bit_cast<uint32_t>(before));
  }
  const float nan = std::bit_cast<float>(0x7fc01234u);
  assert(std::bit_cast<uint32_t>(ClampWaterHighlight(nan)) == 0x7fc01234u);
}
inline void ImageSelection() {
  for (bool active : {false, true}) for (uint32_t count : {0u, 1u, 2u})
    for (uint32_t offset : {0u, 1u, 2u, UINT32_MAX}) {
      Words words{{{kActiveTextureTable + 80, 0x1000}, {kActiveTextureTable + 4, active ? 0x1000u : 0x2000u},
                   {kActiveTextureTable, offset}, {kActiveTextureTable + 32, 0xfeed},
                   {0x1000, count}, {0x1004, 0x3000}, {0x3018, 0xa000}, {0x3034, 0xb000}}};
      const uint32_t index = active ? offset : 0;
      const uint32_t expected = index >= count ? 0xfeed : index ? 0xb000 : 0xa000;
      assert(ReadWaterSceneImage(words.Reader()) == expected);
      const auto required = words.reads;
      for (const auto address : required) {
        auto truncated = words; truncated.values.erase(address);
        assert(!ReadWaterSceneImage(truncated.Reader()));
      }
      // Successful null image is distinct from an unreadable selector.
      words.values[index >= count ? kActiveTextureTable + 32 : 0x3018 + index * 28] = 0;
      assert(ReadWaterSceneImage(words.Reader()) == 0);
    }
  Words null_table{{{kActiveTextureTable + 80, 0}}};
  assert(ReadWaterSceneImage(null_table.Reader()) == 0 && null_table.reads.size() == 1);
  Words empty;
  assert(!ReadWaterSceneImage(empty.Reader()));
  Words words{{{kActiveTextureTable + 80, 0x1000}, {kActiveTextureTable + 4, 0x2000},
               {0x1000, 1}, {0x1004, 0}}};
  assert(!ReadWaterSceneImage(words.Reader())); // missing entries, not a null image
  words.values[0x1004] = UINT32_MAX - 3;
  assert(!ReadWaterSceneImage(words.Reader())); // reject address overflow, never wrap into memory
  words.values[0x1004] = 0x3001;
  assert(!ReadWaterSceneImage(words.Reader())); // unaligned word refused by checked reader
  words.values[kActiveTextureTable + 4] = 0x1000;
  words.values[kActiveTextureTable] = 0x40000000;
  words.values[0x1000] = UINT32_MAX;
  words.values[0x1004] = 0x3000;
  assert(!ReadWaterSceneImage(words.Reader())); // wide index multiplication, no wrapped read
}
inline void FactorDestination() {
  constexpr uint32_t material = 0x1000, owner = 0x4000, buffer = 0x6000;
  Words words{{{material + 5052, 3}, {material + 5056, 2}, {material + 5044, owner}, {owner + 12, buffer}}};
  assert(ReadWaterFactorDestination(material, words.Reader()) == buffer + 56);
  const auto required = words.reads;
  for (const auto address : required) {
    auto truncated = words; truncated.values.erase(address);
    assert(!ReadWaterFactorDestination(material, truncated.Reader()));
  }
  for (uint32_t row : {0u, 1u, 255u, 0x40000000u, UINT32_MAX})
    for (uint32_t component : {0u, 1u, 3u, 0x40000000u, UINT32_MAX}) {
      words.values[material + 5052] = row; words.values[material + 5056] = component;
      const uint32_t offset = uint32_t(uint64_t(row) * 16 + uint64_t(component) * 4);
      const uint64_t address = uint64_t(buffer) + offset;
      const auto actual = ReadWaterFactorDestination(material, words.Reader());
      if (address <= UINT32_MAX - 3) assert(actual == address);
      else assert(!actual);
    }
  words.values[material + 5044] = 0;
  assert(!ReadWaterFactorDestination(material, words.Reader()));
  words.values[material + 5044] = owner; words.values[owner + 12] = 0;
  assert(!ReadWaterFactorDestination(material, words.Reader()));
  words.values[material + 5044] = UINT32_MAX - 3;
  assert(!ReadWaterFactorDestination(material, words.Reader()));
  assert(!ReadWaterFactorDestination(UINT32_MAX - 3, words.Reader()));
}
inline void Run() { SequenceAndScalars(); ImageSelection(); FactorDestination(); }
} // namespace refraction_material_tests
