/**
 * @brief Checked three-light publication and bounded compatibility write plan.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_selected_lights.h"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

namespace bd::gpu::scene {
// Outbound compatibility format only. Native packets use LitLight directly.
inline std::array<float,16> NativeSelectedLightWords(const LitLight &light) {
  return {light.position.x,light.position.y,light.position.z,float(light.kind),
      light.direction.x,light.direction.y,light.direction.z,light.cone_strength,
      light.colour.x,light.colour.y,light.colour.z,light.inverse_range,
      light.cone_cosine,0,0,0};
}
inline bool MatchesNativeLightParameterWrite(const NativeSelectedLights &lights,
    uint32_t first, std::span<const uint32_t> words) {
  for (size_t n = 0; n < words.size(); ++n) {
    const uint64_t word = uint64_t(first)*4+n;
    if (word < 80 || word >= 128) continue;
    const auto slot = (word-80)/16, component = (word-80)%16;
    const auto &light = lights[slot];
    // Disabled/unused original lanes may retain irrelevant previous bytes.
    bool consumed = component == 3 || (component >= 8 && component <= 10);
    if (light.kind != LitDisabled) {
      consumed |= (light.kind == LitDirectional || light.kind == LitSpot) && component >= 4 && component <= 6;
      consumed |= (light.kind == LitPoint || light.kind == LitSpot) && (component < 3 || component == 11);
      consumed |= light.kind == LitSpot && (component == 7 || component == 12);
    }
    if (consumed && std::bit_cast<float>(words[n]) != NativeSelectedLightWords(light)[component]) return false;
  }
  return true;
}
// Source identities stop here. A native consumer gets only NativeSelectedLights.
struct SelectedLightSourceState {
  uint32_t owner = 0, known = 0;
  std::array<int32_t, 3> ids{};
  NativeSelectedLights lights{};
};
struct SelectedLightWrite { uint32_t address = 0, word = 0; bool cosine = false, identity = false; };
struct SelectedLightPublication {
  SelectedLightSourceState state;
  std::array<SelectedLightWrite, 42> writes{};
  size_t count = 0, changed = 0;
};

// sub_8218B0F0 selects three signed IDs at selection+8+view*12 (stride4),
// reads 76-byte authored snapshot records, and writes only CHANGED slots.
// Unknown unchanged values cannot be recovered from a current source snapshot:
// that would invent an update the original did not perform.
template <class Read, class Cosine>
std::optional<SelectedLightPublication> PrepareSelectedLights(uint32_t owner,
    uint32_t selection, uint32_t view, uint32_t records, uint32_t record_count,
    float strength_numerator, const SelectedLightSourceState &previous,
    Read read, Cosine cosine) {
  if (!owner || !selection || !records || (owner & 3) || (selection & 3) ||
      (records & 3) || view >= 16 || record_count > 300 || !std::isfinite(strength_numerator)) return {};
  SelectedLightPublication result;
  if (previous.owner == owner) result.state = previous;
  result.state.owner = owner;
  std::array<uint32_t, 128> dependencies{};
  size_t dependency_count = 0;
  bool valid = true;
  auto word = [&](uint64_t address, bool dependency = true) {
    if (!address || (address & 3) || address > UINT32_MAX - 3) { valid = false; return 0u; }
    const auto value = read(address);
    if (!value || dependency_count == dependencies.size()) { valid = false; return 0u; }
    if (dependency) dependencies[dependency_count++] = uint32_t(address);
    return value.value_or(0);
  };
  auto scalar = [&](uint64_t address) { return std::bit_cast<float>(word(address)); };
  auto vector = [&](uint64_t address) { return LitVec(scalar(address), scalar(address+4), scalar(address+8)); };
  auto destination = [&](uint64_t descriptor, bool component = false) -> uint64_t {
    const auto buffer = word(descriptor + 4), index = word(descriptor + 12);
    const auto begin = buffer ? word(uint64_t(buffer) + 12) : 0;
    const auto lane = component ? word(descriptor + 16) : 0;
    if (!begin || lane > 3) valid = false;
    return uint64_t(begin) + uint64_t(index)*16 + lane*4;
  };
  auto write = [&](uint64_t address, uint32_t value, bool is_cosine = false, bool identity = false) {
    if (!address || (address & 3) || address > UINT32_MAX - 3 ||
        result.count == result.writes.size() || !read(address)) { valid = false; return; }
    result.writes[result.count++] = {uint32_t(address), value, is_cosine, identity};
  };
  auto floating = [&](uint64_t address, float value, bool is_cosine = false) {
    write(address, std::bit_cast<uint32_t>(value), is_cosine);
  };
  auto xyz = [&](uint64_t address, LitVector value) {
    floating(address, value.x); floating(address+4, value.y); floating(address+8, value.z);
  };
  for (uint32_t slot = 0; slot < 3; ++slot) {
    const int32_t id = int16_t(word(uint64_t(selection) + 8 + view*12 + slot*4) >> 16);
    const auto cache = uint64_t(owner) + 276 + slot*4;
    const auto old_id = std::bit_cast<int32_t>(word(cache, false));
    if (!valid) return {};
    if (id == old_id) {
      if (result.state.ids[slot] != id) result.state.known &= ~(1u << slot);
      continue;
    }
    const auto position = destination(uint64_t(owner) + 36 + slot*20);
    const auto colour = destination(uint64_t(owner) + 156 + slot*20);
    LitLight light{};
    if (id >= 0) {
      if (uint32_t(id) >= record_count) return {};
      const uint64_t source = uint64_t(records) + uint32_t(id)*76;
      NativeLightDefinition input;
      input.kind = word(source) & 3;
      input.position = vector(source+20); input.direction = vector(source+32);
      input.colour = vector(source+44); input.cone_angle = scalar(source+56);
      input.cone_softness = scalar(source+60); input.range = scalar(source+72);
      if (!valid || !std::isfinite(input.cone_angle) || !std::isfinite(input.cone_softness) ||
          !std::isfinite(input.range)) return {};
      const float strength = float(double(strength_numerator) / double(float(input.cone_softness + 1.0f)));
      const float cone = float(cosine(double(input.cone_angle)));
      const auto semantic = ComposeSelectedLight(input, strength, cone);
      if (!semantic) return {};
      light = *semantic;
      const auto direction = destination(uint64_t(owner) + 96 + slot*20);
      const auto parameter = destination(uint64_t(owner) + 216 + slot*20, true);
      xyz(position, input.position); floating(position+12, float(input.kind));
      xyz(direction, input.direction); floating(direction+12, strength);
      xyz(colour, input.colour); floating(colour+12, float(1.0 / double(input.range)));
      floating(parameter, cone, true);
    } else {
      xyz(colour, {}); floating(position+12, 0);
    }
    write(cache, std::bit_cast<uint32_t>(id), false, true);
    result.state.ids[slot] = id; result.state.lights[slot] = light;
    result.state.known |= 1u << slot;
    ++result.changed;
  }
  if (!valid) return {};
  // Refuse source/control/destination aliases before any side effect. The
  // original rereads descriptors between stores; snapshotting an alias is unsafe.
  for (size_t i = 0; i < result.count; ++i) {
    for (size_t n = 0; n < dependency_count; ++n)
      if (result.writes[i].address == dependencies[n]) return {};
    for (size_t n = 0; n < i; ++n)
      if (result.writes[i].address == result.writes[n].address) return {};
    for (uint32_t slot = 0; slot < 3; ++slot)
      if (result.writes[i].address == uint64_t(owner)+276+slot*4 &&
          !result.writes[i].identity) return {};
  }
  return result;
}
// Outgoing adapter for retained consumers after a direct native draw. All light
// values come from the native ticket; descriptors only locate destinations.
// Invalidate original selection/node caches so a later non-null callback cannot
// skip a necessary update. No original selected slot or register value is input.
struct NativeLightMirrorPlan {
  std::array<SelectedLightWrite,44> writes{};
  size_t count = 0;
};
template <class Read>
std::optional<NativeLightMirrorPlan> PrepareNativeLightMirror(
    uint32_t owner, const NativeSelectedLights &lights, Read read) {
  if (!owner || (owner & 3)) return {};
  NativeLightMirrorPlan result;
  std::array<uint32_t,48> dependencies{};
  size_t dependency_count = 0;
  bool valid = true;
  const auto word = [&](uint64_t address) {
    const auto value = !address || (address & 3) || address > UINT32_MAX-3 ? std::nullopt : read(address);
    if (!value || dependency_count == dependencies.size()) { valid = false; return 0u; }
    dependencies[dependency_count++] = uint32_t(address);
    return *value;
  };
  const auto destination = [&](uint64_t descriptor, bool component) {
    const auto buffer = word(descriptor+4), index = word(descriptor+12);
    const auto begin = buffer ? word(uint64_t(buffer)+12) : 0;
    const auto lane = component ? word(descriptor+16) : 0;
    if (!begin || lane > 3) valid = false;
    return uint64_t(begin)+uint64_t(index)*16+lane*4;
  };
  const auto write = [&](uint64_t address, uint32_t value) {
    if (!address || (address & 3) || address > UINT32_MAX-3 ||
        result.count == result.writes.size() || !read(address)) { valid = false; return; }
    result.writes[result.count++] = {uint32_t(address),value};
  };
  for (uint32_t slot = 0; slot < 3; ++slot) {
    const auto &light = lights[slot];
    if (light.kind < LitDisabled || light.kind > LitPoint) return {};
    const auto values = NativeSelectedLightWords(light);
    for (const auto value : values) if (!std::isfinite(value)) return {};
    for (uint32_t vector = 0; vector < 4; ++vector) {
      const auto at = destination(uint64_t(owner)+36+vector*60+slot*20,vector == 3);
      for (uint32_t lane = 0; lane < (vector == 3 ? 1u : 4u); ++lane)
        write(at+lane*4,std::bit_cast<uint32_t>(values[vector*4+lane]));
    }
    write(uint64_t(owner)+276+slot*4,std::bit_cast<uint32_t>(int32_t(-2)));
  }
  write(uint64_t(owner)+288,0); write(uint64_t(owner)+292,~0u);
  if (!valid) return {};
  for (size_t n = 0; n < result.count; ++n) {
    for (size_t d = 0; d < dependency_count; ++d)
      if (result.writes[n].address == dependencies[d]) return {};
    for (size_t d = 0; d < n; ++d)
      if (result.writes[n].address == result.writes[d].address) return {};
  }
  return result;
}
} // namespace bd::gpu::scene
