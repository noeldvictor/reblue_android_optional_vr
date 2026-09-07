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
  constexpr uint32_t override_byte = (uint32_t(-32035) << 16) - 26711;
  const auto override_word = read(override_byte & ~3u);
  const auto direct = read(defaults + 60), sorted = read(defaults + 64);
  const auto special = read(uint64_t(visual) + 3120), blocked = read(scene + 212);
  // This global mode preserves externally supplied state instead of applying
  // the ordinary node recipe. It is not an ordinary cutout pass.
  if (!override_word || ((*override_word >> ((3 - (override_byte & 3)) * 8)) & 255) ||
      !direct || !sorted || !special || !blocked) return {};
  NativeMaterialAlphaInputs result{*direct, *sorted};
  if (*special && !*blocked) {
    result.object_reference = read(uint64_t(visual) + 3124);
    if (!result.object_reference) return {};
  }
  return result;
}
} // namespace bd::gpu::scene
