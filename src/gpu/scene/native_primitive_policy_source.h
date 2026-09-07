/**
 * @brief Checked object-pass input adapter for native primitive policies.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_primitive_policy.h"
#include <optional>
namespace bd::gpu::scene {
template <class Read>
std::optional<PrimitivePolicyInputs> ReadPrimitivePolicyInputs(
    uint32_t context, uint32_t visual, Read read) {
  if (!context || (context & 3) || context > UINT32_MAX - 35 ||
      !visual || (visual & 3) || visual > UINT32_MAX - 3535) return {};
  const auto phase = read(uint64_t(context) + 16), mode = read(uint64_t(context) + 32);
  const auto tech = read(uint64_t(visual) + 3000), special = read(uint64_t(visual) + 3120);
  const auto modulate = read(uint64_t(visual) + 3068);
  constexpr uint32_t cull_source = (uint32_t(-32036) << 16) - 5536;
  constexpr uint32_t scene_source = (uint32_t(-32036) << 16) - 7768;
  constexpr uint32_t effects_source = (uint32_t(-32101) << 16) + 22688;
  const auto cull = read(cull_source), blocked = read(scene_source + 212), effects = read(effects_source);
  if (!phase || *phase > 2 || !mode || !tech || !special || !modulate || !cull || !blocked || !effects) return {};
  PrimitivePolicyInputs result;
  result.phase = *phase; result.pass_mode = *mode; result.technique = *tech;
  const auto winding = *cull >> 24; // original unsigned byte, not the adjacent pass fields
  result.pass_cull = winding == 0 ? PrimitiveCull::Front :
                     winding == 1 ? PrimitiveCull::Back : PrimitiveCull::None;
  result.special_shadow_block = *special && *blocked;
  result.shadow_modulates_colour = *modulate != 0;
  result.texture_effects = *effects != 0;
  if (*tech == 3) {
    const auto wind = read(uint64_t(visual) + 3532);
    if (!wind) return {};
    if (*wind) {
      const auto policy = read(*wind);
      if (!policy) return {};
      result.wind_rejects_shadow = *policy == 1;
      result.wind_forces_sorted = *policy == 2;
    }
  }
  return result;
}
} // namespace bd::gpu::scene
