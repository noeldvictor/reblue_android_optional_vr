/**
 * @brief Checked object/pass cutoff boundary; no device cache reads.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_alpha.h"

namespace bd::gpu::scene {
template <class Read>
std::optional<NativeMaterialAlphaInputs> ReadMaterialAlphaInputs(uint32_t visual, Read read) {
  if (!visual || (visual & 3) || visual > UINT32_MAX - 3127) return {};
  constexpr uint32_t defaults = (uint32_t(-32036) << 16) - 22280;
  constexpr uint32_t scene = (uint32_t(-32036) << 16) - 7768;
  const auto direct = read(defaults + 60), sorted = read(defaults + 64);
  const auto special = read(uint64_t(visual) + 3120), blocked = read(scene + 212);
  // The pass light-space byte only skips colour writes at 82280740..82280880.
  // Reference defaulting precedes that branch; both setters at 82280988..D0
  // still execute. A primary-shadow pass must not lose its cutoff publication
  // merely because its colour/light-space path differs from the scene pass.
  if (!direct || !sorted || !special || !blocked) return {};
  NativeMaterialAlphaInputs result{*direct, *sorted};
  if (*special && !*blocked) {
    result.object_reference = read(uint64_t(visual) + 3124);
    if (!result.object_reference) return {};
  }
  return result;
}
} // namespace bd::gpu::scene
