// Included after assertions are enabled and refraction_material_tests::Words.
#pragma once
#include "gpu/scene/water_material_import.h"
#include <vector>

namespace water_update_tests {
using namespace bd::gpu::scene;
using refraction_material_tests::Words;
constexpr uint32_t kMaterial = 0x10000, kOwner = 0x30000, kBuffer = 0x40000;
constexpr std::array<uint32_t, 20> kSources{
    4660,4664,4668,4672,4676,4724,4728,4732,4736,4692,4696,4700,4704,4712,4716,4720,4740,4744,4748,4752};
constexpr std::array<uint32_t, 20> kDescriptors{
    4768,4788,4808,4828,4848,4868,4888,4908,4928,4960,4980,5000,5020,5060,5080,5100,5120,5140,5160,5180};
inline uint32_t Bits(float value) { return std::bit_cast<uint32_t>(value); }
inline uint32_t Output(uint32_t index) { return kBuffer + index * 256 + (index == 4 ? 48 : 56); }
inline Words Fixture() {
  Words words;
  for (uint32_t offset = 4656; offset < 5200; offset += 4) words.values[kMaterial + offset] = 0;
  for (uint32_t i = 0; i < kSources.size(); ++i) {
    const auto descriptor = kMaterial + kDescriptors[i];
    words.values[descriptor + 4] = kOwner + i * 16;
    words.values[descriptor + 12] = 3;
    words.values[descriptor + 16] = 2;
    words.values[kOwner + i * 16 + 12] = kBuffer + i * 256;
    for (uint32_t offset = 0; offset < 128; offset += 4) words.values[kBuffer + i * 256 + offset] = 0;
    words.values[kMaterial + kSources[i]] = Bits(float(i + 1));
  }
  for (uint32_t i = 0; i < 4; ++i) words.values[kMaterial + 4676 + i * 4] = Bits(float(11 + i));
  words.values[kMaterial + 4656] = 3;
  words.values[kMaterial + 4708] = 1;
  words.values[kMaterial + 4700] = uint32_t(-3);
  words.values[kMaterial + 4672] = Bits(.9375f);
  words.values[kWaterStep] = Bits(.125f);
  words.values[kWaterWrap] = Bits(1.f);
  words.values[kWaterScale] = Bits(.25f);
  words.values[kWaterDefault] = Bits(.375f);
  words.values[kWaterSettings] = 0x60000;
  words.values[0x60000 + 7024] = 0;
  words.values[kSamplingDemand] = 10;
  words.values[kSamplingDemand + 4] = 20;
  return words;
}
inline void DemandTransitions() {
  for (uint32_t before : {0u,1u,2u,3u,UINT32_MAX}) for (uint32_t after : {0u,1u,2u,3u,UINT32_MAX})
    for (uint32_t count : {0u, 1u, UINT32_MAX}) {
      Words words{{{0x1000,before},{kSamplingDemand,count},{kSamplingDemand+4,count}}};
      const auto saved = words.values;
      const auto plan = BuildSamplingDemandUpdate(0x1000, after, words.Reader());
      assert(plan && words.values == saved);
      plan->Apply([&](uint32_t at, uint32_t value) { words.values[at] = value; });
      assert(words.values[0x1000] == after && plan->Matches(words.Reader()));
      const auto count_for = [&](uint32_t mode) { return count + uint32_t(after == mode) - uint32_t(before == mode); };
      assert(words.values[kSamplingDemand] == count_for(1));
      assert(words.values[kSamplingDemand + 4] == count_for(3));
      if (before == after) assert(plan->count == 0);
      else assert(plan->writes[0].address == 0x1000 && plan->writes[0].value == after);
    }
  Words alias{{{kSamplingDemand,3},{kSamplingDemand+4,9}}};
  auto plan = BuildSamplingDemandUpdate(kSamplingDemand, 1, alias.Reader());
  assert(plan && plan->count == 3);
  plan->Apply([&](uint32_t at, uint32_t value) { alias.values[at] = value; });
  assert(alias.values[kSamplingDemand] == 2 && alias.values[kSamplingDemand + 4] == 8);
}
inline void Publications() {
  for (bool tick : {false, true}) for (bool detail : {false, true}) {
    auto words = Fixture(); words.values[0x60000 + 7024] = detail;
    const auto saved = words.values;
    const auto plan = BuildWaterMaterialUpdate(kMaterial, tick, words.Reader());
    assert(plan && words.values == saved && plan->parameters == 23 && plan->mode == 3);
    assert(plan->phase == (tick ? 2 : 0) && plan->count == (tick ? 28 : 26));
    const auto required_reads = words.reads;
    for (const auto at : required_reads) {
      auto truncated = Fixture(); truncated.values[0x60000 + 7024] = detail; truncated.values.erase(at);
      const auto before = truncated.values;
      assert(!BuildWaterMaterialUpdate(kMaterial, tick, truncated.Reader()));
      assert(truncated.values == before); // even the final missing word has no partial publication
    }
    plan->Apply([&](uint32_t at, uint32_t value) { words.values[at] = value; });
    assert(plan->Matches(words.Reader()));
    assert(words.values[kMaterial + 4656] == 1 && words.values[kSamplingDemand] == 11 && words.values[kSamplingDemand+4] == 19);
    for (uint32_t i = 0; i < kSources.size(); ++i) {
      float expected = std::bit_cast<float>(saved.at(kMaterial + kSources[i]));
      if (i == 3) expected = tick ? .0625f : .9375f;
      if (i == 7) expected *= .25f;
      if (i == 11) expected = -3.f;
      if (i == 16 && !detail) expected = .375f;
      assert(words.values[Output(i)] == Bits(expected));
    }
    for (uint32_t i = 0; i < 4; ++i) assert(words.values[Output(4)+i*4] == Bits(float(11+i)));
    words.values[Output(19)] ^= 1;
    assert(!plan->Matches(words.Reader()));
  }
}
inline void AliasesAndBounds() {
  auto words = Fixture();
  words.values[kMaterial + kDescriptors[0] + 12] = 0;
  words.values[kMaterial + kDescriptors[0] + 16] = 0;
  words.values[kOwner + 12] = kMaterial + 4664; // first publication changes next source
  words.values[kMaterial + 4660] = Bits(.75f);
  words.values[kOwner + 4 * 16 + 12] = kMaterial + 4680;
  words.values[kMaterial + kDescriptors[4] + 12] = 0; // forward-overlapping vector reads are sequential
  auto plan = BuildWaterMaterialUpdate(kMaterial, false, words.Reader());
  assert(plan);
  plan->Apply([&](uint32_t at, uint32_t value) { words.values[at] = value; });
  assert(words.values[Output(1)] == Bits(.75f) && words.values[Output(9)] == Bits(11.f));
  assert(plan->Matches(words.Reader()));
  words = Fixture();
  words.values[kMaterial + kDescriptors[0] + 12] = 0;
  words.values[kMaterial + kDescriptors[0] + 16] = 0;
  words.values[kOwner + 12] = kMaterial + kDescriptors[1] + 12;
  words.values[kMaterial + 4660] = 4; // first write changes next descriptor's row
  plan = BuildWaterMaterialUpdate(kMaterial, false, words.Reader());
  assert(plan);
  plan->Apply([&](uint32_t at, uint32_t value) { words.values[at] = value; });
  assert(words.values[kBuffer + 256 + 72] == Bits(2.f));
  words = Fixture(); words.values[kOwner + 12] = UINT32_MAX - 3;
  assert(!BuildWaterMaterialUpdate(kMaterial, true, words.Reader()));
  words = Fixture(); words.values[kMaterial + kDescriptors[0] + 4] = 0;
  assert(!BuildWaterMaterialUpdate(kMaterial, true, words.Reader()));
  words = Fixture(); words.values[kWaterSettings] = 0;
  assert(!BuildWaterMaterialUpdate(kMaterial, true, words.Reader()));
  assert(!BuildWaterMaterialUpdate(UINT32_MAX, true, words.Reader()));
  assert(!BuildWaterMaterialUpdate(0, true, words.Reader()));
  auto reader = words.Reader();
  WaterUpdateBuilder<decltype(reader)> bounded{reader};
  for (uint32_t i = 0; i < 33; ++i) bounded.Store(kMaterial + 4660, i);
  assert(!bounded.Finish() && bounded.plan.count == 32);
}
inline void PhaseEdges() {
  for (float phase : {-1.f, .875f, 1.f, 2.f, INFINITY, -INFINITY, std::bit_cast<float>(0x7fc01234u)}) {
    auto words = Fixture(); words.values[kMaterial+4672] = Bits(phase);
    const auto plan = BuildWaterMaterialUpdate(kMaterial, true, words.Reader());
    assert(plan);
    const float next = float(double(phase) + .125);
    const float expected = next > 1 ? float(double(next) - 1) : next;
    plan->Apply([&](uint32_t at, uint32_t value) { words.values[at] = value; });
    const float actual = std::bit_cast<float>(words.values[kMaterial+4672]);
    assert((std::isnan(expected) && std::isnan(actual)) || Bits(actual) == Bits(expected));
    assert(plan->phase == (next > 1 ? 2 : 1));
  }
}
inline void Run() { DemandTransitions(); Publications(); AliasesAndBounds(); PhaseEdges(); }
} // namespace water_update_tests
