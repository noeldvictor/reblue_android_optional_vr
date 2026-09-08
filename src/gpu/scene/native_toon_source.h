/**
 * @brief Authored visual/scene Toon inputs, before native node consumption.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_toon_surface.h"
#include <bit>
#include <cmath>
#include <optional>

namespace bd::gpu::scene {
// sub_82174648 combines scene +68/+84 and visual +3752/+3768 additions,
// and multiplies scene +100/+116 by visual +3784/+3800. Its disabled branch
// explicitly publishes zero additions and unit multipliers. No shader reads.
// sub_82174270 selects fur/outline variants at +3660/+3668; +3652 enables
// lattice deformation. Those consumers are not the ordinary Toon surface.
template <class Read>
std::optional<NativeToonSurface> ReadNativeToonSurface(uint32_t visual, Read read) {
  constexpr uint32_t scene = 0x82783A58;
  if (!visual || (visual & 3) || visual > UINT32_MAX - 3815 ||
      read(uint64_t(visual)+3000) != 1 || read(uint64_t(visual)+3652) != 0 ||
      read(uint64_t(visual)+3660) != 0 || read(uint64_t(visual)+3668) != 0) return {};
  const auto table = read(scene), enabled = read(scene+132);
  if (!table || !*table || read(uint64_t(*table)+8) != 0x82174648 || !enabled) return {};
  NativeToonSurface result;
  result.diffuse_scale = result.ambient_scale = {1,1,1,1};
  if (*enabled) {
    LightingVector *values[]{&result.diffuse_add,&result.ambient_add,&result.diffuse_scale,&result.ambient_scale};
    for (uint32_t n=0;n<4;++n) for (uint32_t c=0;c<4;++c) {
      const auto a = read(scene+68+n*16+c*4), b = read(uint64_t(visual)+3752+n*16+c*4);
      if (!a || !b) return {};
      const float x = std::bit_cast<float>(*a), y = std::bit_cast<float>(*b);
      if (!std::isfinite(x) || !std::isfinite(y)) return {};
      (*values[n])[c] = n < 2 ? x+y : x*y;
      if (!std::isfinite((*values[n])[c])) return {};
    }
  }
  // Per-node tint initialization/ordered texture writes are composed with the
  // existing texture recipe, not captured here. Ordinary texture classification
  // excludes the volume branch that temporarily enables ignore-alpha.
  return result;
}
} // namespace bd::gpu::scene
