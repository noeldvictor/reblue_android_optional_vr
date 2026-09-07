/**
 * @brief Bounded authored-light selection boundary and transactional write plan.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_light_selection.h"
#include "gpu/scene/native_selected_lights_source.h"
#include <bit>

namespace bd::gpu::scene {
struct NativeLightSelectionSource {
  uint32_t manager = 0, selection = 0, live_owner = 0, view = 0;
  bool primary_thread = false, special_scene = false;
  NativeLightScoreParameters scoring;
};
struct NativeLightSelectionWrite { uint32_t address = 0, before = 0, after = 0; };
struct NativeLightSelectionPlan {
  NativeLightSelection selection;
  std::array<NativeLightSelectionWrite, 5> writes{};
  size_t candidates = 0;
  bool rebuilt = false;
};
inline bool LightSelectionOutput(uint64_t address, uint32_t selection, uint32_t view) {
  if (view >= 16) return true;
  return address == uint64_t(selection)+4 ||
      (address >= uint64_t(selection)+8+view*12 && address < uint64_t(selection)+20+view*12) ||
      address == uint64_t(selection)+216+(view & ~3u);
}

// Reads are completed before stores. No shader registers, scratch buffers or
// retained source aliases enter NativeLightSelection / NativeLightCandidate.
template <class Read>
std::optional<NativeLightSelectionPlan> PrepareNativeLightSelection(
    const NativeLightSelectionSource &source, Read read) {
  if (!source.manager || !source.selection || !source.live_owner ||
      (source.manager & 3) || (source.selection & 3) || (source.live_owner & 3) || source.view >= 16) return {};
  bool valid = true;
  const auto word = [&](uint64_t address, bool state = false) {
    if (!address || (address & 3) || address > UINT32_MAX-3 ||
        (!state && LightSelectionOutput(address, source.selection, source.view))) { valid = false; return 0u; }
    const auto value = read(address);
    if (!value) valid = false;
    return value.value_or(0);
  };
  const auto scalar = [&](uint64_t address) { return std::bit_cast<float>(word(address)); };
  const auto vector = [&](uint64_t address) { return LitVec(scalar(address), scalar(address+4), scalar(address+8)); };
  NativeLightSelectionPlan result;
  const uint64_t selection = source.selection, manager = source.manager;
  result.writes[0] = {uint32_t(selection+4), word(selection+4, true)};
  for (size_t slot = 0; slot < 3; ++slot) {
    const auto address = selection+8+source.view*12+slot*4;
    const auto value = word(address, true);
    result.writes[1+slot] = {uint32_t(address), value};
    result.selection.slots[slot] = {int16_t(value >> 16), uint8_t(value >> 8), uint8_t(value)};
  }
  const auto category_address = selection+216+(source.view & ~3u);
  const auto category_word = word(category_address, true);
  const uint32_t shift = (3-(source.view & 3))*8;
  result.writes[4] = {uint32_t(category_address), category_word};
  result.selection.category = uint8_t(category_word >> shift);
  if (!valid) return {};
  NativeLightSelectionInputs input;
  input.view = source.view; input.special_scene = source.special_scene;
  input.object_class = word(selection); input.mode = word(manager);
  input.centre = vector(selection+200); input.radius = scalar(selection+212);
  const uint32_t priority_offset = (source.primary_thread ? 12007 : 12008)*4;
  input.priority_light = std::bit_cast<int32_t>(word(manager+priority_offset));
  const auto live_priority = std::bit_cast<int32_t>(word(uint64_t(source.live_owner)+priority_offset));
  const auto classify = [&](const NativeLightSelection &value) -> std::optional<uint8_t> {
    const auto id = value.slots[0].id;
    if (id < 0) return uint8_t(0);
    if (id >= 300) return {};
    uint8_t category = 3;
    if (value.slots[1].id < 0) {
      const auto kind = word(uint64_t(source.live_owner)+uint32_t(id)*76+8) & 3;
      category = kind == LitDirectional ? 1 : kind == LitSpot ? 2 : 3;
    }
    if (id == live_priority) category |= 4;
    return valid ? std::optional(category) : std::nullopt;
  };
  const auto candidate = [&](uint64_t address) -> std::optional<NativeLightCandidate> {
    NativeLightCandidate value;
    const auto flags = word(address);
    value.kind = flags & 3; value.enabled = flags & 16; value.priority = flags & 32;
    value.excluded_objects = word(address+4); value.views = word(address+8);
    value.id = std::bit_cast<int32_t>(word(address+12));
    // Only scoring consumes these fields; invalid unused numeric values do not
    // turn a disabled/different-view candidate into a selection change.
    value.position = vector(address+20); value.direction = vector(address+32);
    value.cone_angle = scalar(address+56); value.intensity = scalar(address+64); value.range = scalar(address+72);
    ++result.candidates;
    return valid ? std::optional(value) : std::nullopt;
  };
  const auto insert = [&](uint64_t address) {
    const auto value = candidate(address);
    return value && InsertNativeLight(result.selection, *value, input, source.scoring, classify);
  };
  const auto rebuild = [&] {
    const int32_t count = std::bit_cast<int32_t>(word(manager+46816));
    if (!valid || count > 300) return false;
    result.selection = {}; result.rebuilt = true;
    for (int32_t n = 0; n < count; ++n)
      if (!insert(manager+24016+uint32_t(n)*76)) return false;
    return true;
  };
  if (result.writes[0].before & (1u << source.view)) {
    if (!rebuild()) return {};
  } else {
    const int32_t count = std::bit_cast<int32_t>(word(manager+48020));
    if (!valid || count > 300) return {};
    for (int32_t n = 0; n < count; ++n) {
      const auto address = word(manager+46820+uint32_t(n)*4);
      if (!address) return {};
      const auto id = std::bit_cast<int32_t>(word(uint64_t(address)+12));
      bool selected = false;
      for (const auto &slot : result.selection.slots) selected |= slot.id == id;
      // Compare with the EVOLVING selection, not just the original three IDs.
      if (selected) { if (!rebuild()) return {}; break; }
      if (!insert(address)) return {};
    }
  }
  if (!valid) return {};
  result.writes[0].after = result.writes[0].before & ~(result.rebuilt ? 1u << source.view : 0u);
  for (size_t slot = 0; slot < 3; ++slot) {
    const auto &value = result.selection.slots[slot];
    result.writes[1+slot].after = (uint32_t(uint16_t(value.id)) << 16) | (uint32_t(value.weight) << 8) | value.priority;
  }
  result.writes[4].after = (category_word & ~(255u << shift)) | (uint32_t(result.selection.category) << shift);
  return result;
}
// Direct node preflight shares both production cores, without committing their
// compatibility writes. A failed later material/image admission leaves source
// state untouched. Unknown unchanged slots remain unavailable, not fresh defaults.
template <class Read, class Cosine>
std::optional<NativeSelectedLights> PreviewSelectedLightValues(const NativeLightSelectionPlan &plan,
    uint32_t owner, uint32_t selection, uint32_t view, uint32_t records, uint32_t record_count,
    float strength, const SelectedLightSourceState &previous, Read read, Cosine cosine) {
  const auto overlay = [&](uint64_t address) -> std::optional<uint32_t> {
    for (const auto &write : plan.writes) if (write.address == address) return write.after;
    return read(address);
  };
  const auto publication = PrepareSelectedLights(owner, selection, view, records, record_count,
      strength, previous, overlay, cosine);
  if (!publication || publication->state.known != 7) return {};
  return publication->state.lights;
}
} // namespace bd::gpu::scene
