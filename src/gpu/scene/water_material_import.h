/**
 * @brief Bounded, checked water-property publication at the temporary material ABI.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_water_update.h"
#include <array>
#include <bit>
#include <optional>

namespace bd::gpu::scene {
constexpr uint32_t kWaterStep = (uint32_t(-32250) << 16) + 8116;
constexpr uint32_t kWaterWrap = (uint32_t(-32247) << 16) - 4696;
constexpr uint32_t kWaterScale = (uint32_t(-32247) << 16) - 5532;
constexpr uint32_t kWaterDefault = (uint32_t(-32251) << 16) + 21040;
constexpr uint32_t kWaterSettings = (uint32_t(-32035) << 16) - 26552;
constexpr uint32_t kSamplingDemand = (uint32_t(-32035) << 16) - 26264;
struct WaterWordWrite { uint32_t address = 0, value = 0; };
struct WaterUpdatePlan {
  std::array<WaterWordWrite, 32> writes{}; // <=23 parameters +2 phase +3 mode words
  uint32_t count = 0, parameters = 0, phase = 0, mode = 0;
  template <class Store> void Apply(Store store) const {
    for (uint32_t i = 0; i < count; ++i) store(writes[i].address, writes[i].value);
  }
  template <class Read> bool Matches(Read read) const {
    // Aliased destinations compare only their final write.
    for (uint32_t i = 0; i < count; ++i) {
      bool final = true;
      for (uint32_t j = i + 1; j < count; ++j) final &= writes[j].address != writes[i].address;
      if (final && read(writes[i].address) != std::optional(writes[i].value)) return false;
    }
    return true;
  }
};
// In-memory read-through overlay: later source/control reads see earlier writes.
// A refusal never publishes partial state; there is no heap or disk allocation.
template <class Read> struct WaterUpdateBuilder {
  Read read;
  WaterUpdatePlan plan;
  bool valid = true;
  uint32_t Word(uint64_t address) {
    if (!valid || !address || (address & 3) || address > UINT32_MAX - 3) { valid = false; return 0; }
    for (uint32_t i = plan.count; i > 0; --i)
      if (plan.writes[i - 1].address == address) return plan.writes[i - 1].value;
    const auto value = read(address);
    if (!value) valid = false;
    return value.value_or(0);
  }
  void Store(uint64_t address, uint32_t value) {
    Word(address); // validate even write-only destinations before committing anything
    if (plan.count == plan.writes.size()) valid = false;
    if (valid) plan.writes[plan.count++] = {uint32_t(address), value};
  }
  float Float(uint64_t address) { return std::bit_cast<float>(Word(address)); }
  void StoreFloat(uint64_t address, float value) { Store(address, std::bit_cast<uint32_t>(value)); }
  uint64_t Destination(uint64_t descriptor, bool vector) {
    const uint32_t row = Word(descriptor + 12);
    const uint32_t component = vector ? 0 : Word(descriptor + 16);
    const uint32_t owner = Word(descriptor + 4);
    if (!owner) valid = false;
    const uint32_t buffer = Word(uint64_t(owner) + 12);
    if (!buffer) valid = false;
    const uint32_t offset = (row * 4u + component) * 4u;
    return uint64_t(buffer) + offset; // final address overflow is refused by Store
  }
  void SamplingMode(uint64_t address, uint32_t next) {
    const auto change = ChangeSamplingDemand(Word(address), next);
    if (!change.changed) return;
    Store(address, next); ++plan.mode;
    if (change.add >= 0) {
      const uint32_t at = kSamplingDemand + uint32_t(change.add) * 4;
      Store(at, Word(at) + 1u); ++plan.mode;
    }
    if (change.remove >= 0) {
      const uint32_t at = kSamplingDemand + uint32_t(change.remove) * 4;
      Store(at, Word(at) - 1u); ++plan.mode;
    }
  }
  std::optional<WaterUpdatePlan> Finish() const { return valid ? std::optional(plan) : std::nullopt; }
};
template <class Read>
std::optional<WaterUpdatePlan> BuildSamplingDemandUpdate(uint32_t address, uint32_t mode, Read read) {
  WaterUpdateBuilder<Read> builder{read};
  builder.SamplingMode(address, mode);
  return builder.Finish();
}
template <class Read>
std::optional<WaterUpdatePlan> BuildWaterMaterialUpdate(uint32_t material, bool tick, Read read) {
  if (!material || uint64_t(material) + 5200 > uint64_t(UINT32_MAX) + 1) return {};
  WaterUpdateBuilder<Read> builder{read};
  builder.SamplingMode(uint64_t(material) + 4656, builder.Word(uint64_t(material) + 4708) == 1);
  if (tick) {
    const float next = AddWaterPhase(builder.Float(uint64_t(material) + 4672), builder.Float(kWaterStep));
    builder.StoreFloat(uint64_t(material) + 4672, next); ++builder.plan.phase;
    const float limit = builder.Float(kWaterWrap); // read after first store, as in the source
    if (next > limit) {
      builder.StoreFloat(uint64_t(material) + 4672, WrapWaterPhase(next, limit)); ++builder.plan.phase;
    }
  }
  enum class Kind { Scalar, Vector, Scaled, Signed, OptionalDetail };
  struct Binding { uint32_t source, descriptor; Kind kind = Kind::Scalar; };
  // Material member/descriptor offsets exist only in this temporary import.
  constexpr std::array bindings{
      Binding{4660,4768}, Binding{4664,4788}, Binding{4668,4808}, Binding{4672,4828},
      Binding{4676,4848,Kind::Vector}, Binding{4724,4868}, Binding{4728,4888},
      Binding{4732,4908,Kind::Scaled}, Binding{4736,4928}, Binding{4692,4960},
      Binding{4696,4980}, Binding{4700,5000,Kind::Signed}, Binding{4704,5020},
      Binding{4712,5060}, Binding{4716,5080}, Binding{4720,5100},
      Binding{4740,5120,Kind::OptionalDetail}, Binding{4744,5140}, Binding{4748,5160}, Binding{4752,5180}};
  for (const auto binding : bindings) {
    const bool vector = binding.kind == Kind::Vector;
    const uint64_t destination = builder.Destination(uint64_t(material) + binding.descriptor, vector);
    for (uint32_t component = 0; component < (vector ? 4u : 1u); ++component) {
      const uint64_t source = uint64_t(material) + binding.source + component * 4;
      float value;
      if (binding.kind == Kind::Signed) value = float(int32_t(builder.Word(source)));
      else if (binding.kind == Kind::OptionalDetail) {
        const auto settings = builder.Word(kWaterSettings);
        if (!settings) builder.valid = false;
        value = builder.Word(uint64_t(settings) + 7024) ? builder.Float(source) : builder.Float(kWaterDefault);
      } else {
        value = builder.Float(source);
        if (binding.kind == Kind::Scaled) value = ScaleWaterParameter(value, builder.Float(kWaterScale));
      }
      builder.StoreFloat(destination + component * 4, value);
      ++builder.plan.parameters;
    }
  }
  return builder.Finish();
}
} // namespace bd::gpu::scene
