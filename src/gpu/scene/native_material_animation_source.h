/**
 * @brief Explicit material timeline import/export and exact late-writer guard.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_animation.h"
#include "gpu/scene/native_image_catalog_source.h"
#include <bit>

namespace bd::gpu::scene::material_animation_source {
using Boundary=std::array<uint32_t,9>;
inline std::optional<Boundary> Export(const NativeMaterialAnimation &state,
    const material_image_source::LoadedCatalog &catalog) {
  if (!state.Valid() || state.catalog.get() != &catalog.catalog) return {};
  Boundary result{state.ids[0],state.ids[1],state.loop_bits,std::bit_cast<uint32_t>(state.time),
      std::bit_cast<uint32_t>(state.speed),0,0,state.queued[0],state.queued[1]};
  for (size_t n=0; n<2; ++n) if (state.cues[n] != UINT32_MAX) result[5+n]=catalog.exports[state.cues[n]].node;
  return result;
}
struct Publication { NativeMaterialAnimation state; Boundary boundary; };
template<class Read>
std::optional<Publication> ReadState(uint32_t visual,
    const std::shared_ptr<const material_image_source::LoadedCatalog> &catalog, Read read) {
  if (!catalog || !visual || (visual&3) || visual > UINT32_MAX-2247) return {};
  Publication result;
  for (size_t n=0; n<result.boundary.size(); ++n) {
    const auto word=read(uint64_t(visual)+2212+n*4);
    if (!word) return {};
    result.boundary[n]=*word;
  }
  const auto &words=result.boundary;
  auto &state=result.state;
  state.catalog=std::shared_ptr<const NativeImageCatalog>(catalog,&catalog->catalog);
  state.ids={words[0],words[1]}; state.loop_bits=words[2];
  state.time=std::bit_cast<float>(words[3]); state.speed=std::bit_cast<float>(words[4]);
  state.queued={words[7],words[8]};
  for (size_t n=0; n<2; ++n) {
    if (!words[5+n]) continue;
    for (uint32_t ordinal=0; ordinal<catalog->exports.size(); ++ordinal)
      if (catalog->exports[ordinal].node == words[5+n]) { state.cues[n]=ordinal; break; }
    if (state.cues[n] == UINT32_MAX) return {}; // stale/foreign selected entry, never invent a cue
  }
  return result;
}
template<class Read> bool Matches(uint32_t visual, const Boundary &boundary, Read read) {
  for (size_t n=0; n<boundary.size(); ++n)
    if (read(uint64_t(visual)+2212+n*4) != std::optional(boundary[n])) return false;
  return true;
}
} // namespace bd::gpu::scene::material_animation_source
