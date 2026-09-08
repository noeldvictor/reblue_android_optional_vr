/**
 * @brief Checked temporary callback boundary for ordinary native sorted work.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "gpu/scene/native_visual_inputs.h"

namespace bd::gpu::scene {
// This is a compatibility import, never a native asset or packet layout. The
// verified executable registers lights before shader selection for models;
// shadow participants precede the shader's terminating visual callback. Check
// live slots, not only a remembered vtable address or an initializer census.
template <class Read>
bool CheckNativeDeferredRegistry(Read read) {
  constexpr uint32_t registry = (uint32_t(-32030) << 16) - 31132;
  constexpr uint32_t shader = 0x82783A58, lights = 0x82E246F4;
  constexpr uint32_t noop = 0x820DFA50;
  const auto method = [&](uint32_t object, uint32_t slot) -> std::optional<uint32_t> {
    const auto table = object ? read(object) : std::nullopt;
    return table && *table ? read(uint64_t(*table) + slot) : std::nullopt;
  };
  const auto model_count = read(registry + 8), models = read(registry);
  if (model_count != 2 || !models || !*models ||
      read(*models) != lights || read(uint64_t(*models) + 4) != shader ||
      method(lights, 0) != 0x8218B310 || method(lights, 4) != noop ||
      method(shader, 0) != 0x82174270 || method(shader, 4) != noop) return {};
  const auto count = read(registry + 20), entries = read(registry + 12);
  if (!count || !*count || *count > 11 || !entries || !*entries) return {};
  std::array<uint32_t, 11> seen{};
  for (uint32_t i = 0; i < *count; ++i) {
    const auto participant = read(uint64_t(*entries) + i * 4);
    if (!participant || !*participant) return {};
    for (uint32_t j = 0; j < i; ++j) if (seen[j] == *participant) return {};
    seen[i] = *participant;
    uint32_t begin = 0, end = noop;
    if (i + 1 == *count) {
      if (*participant != shader) return {};
      begin = 0x82174648; end = 0x82174C60;
    } else if (*participant == 0x82DD6100) begin = 0x82176708;
    else if (*participant == 0x82DD6FC0) begin = 0x820D1998;
    else if (*participant >= 0x82DD62A0 && (*participant - 0x82DD62A0) % 420 == 0 &&
             (*participant - 0x82DD62A0) / 420 < 8) begin = 0x82177650;
    else return {};
    if (method(*participant, 8) != begin || method(*participant, 12) != end) return {};
  }
  return true;
}
// Only these resource callbacks may be OMITTED by native model consumption.
template <class Read>
bool CheckDeferredVisualResource(uint32_t visual, Read read) {
  if (!visual) return true;
  const auto table = read(visual);
  return table && *table && read(uint64_t(*table) + 32) == 0x820DFA50 &&
      read(uint64_t(*table) + 36) == 0x820DFA50;
}
// Water resource callbacks still EXECUTE for the legacy draw. Their whole-function
// producer refreshes the active native input publication after all writes,
// including indirect parameter aliases. Other unknown writers remain refused.
template <class Read>
bool CheckDeferredBatchResource(uint32_t visual, Read read) {
  if (CheckDeferredVisualResource(visual, read)) return true;
  const auto table = visual ? read(visual) : std::nullopt;
  return table && *table && read(uint64_t(*table) + 32) == 0x82454720 &&
      read(uint64_t(*table) + 36) == 0x824548A8;
}
template <class Read>
std::optional<uint32_t> CheckNativeDeferredContract(uint32_t visual, Read read) {
  // PrepareEffectModel's input+4 is the visual (entry+244).
  if (!visual || read(uint64_t(visual) + 3000) != 0 ||
      !CheckDeferredVisualResource(visual, read) || !CheckNativeDeferredRegistry(read)) return {};
  const auto blend_mode = read(uint64_t(visual) + 1864);
  return blend_mode && *blend_mode <= 5 ? blend_mode : std::nullopt;
}
template <class Read>
std::optional<NativeVisualInputs> ReadNativeDeferredVisualInputs(
    NativeVisualIdentity identity, uint32_t visual, Read read) {
  if (!identity) return {};
  const auto mode = CheckNativeDeferredContract(visual, read);
  const auto category = visual ? read(uint64_t(visual) + 3132) : std::nullopt;
  return mode && category ? std::optional(NativeVisualInputs{identity, NativeVisualBlend(*mode), *category}) : std::nullopt;
}
} // namespace bd::gpu::scene
