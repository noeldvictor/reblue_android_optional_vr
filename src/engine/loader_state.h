/**
 * @file    loader_state.h
 * @brief   Testable observations at the temporary engine-state import boundary.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once

#include <cstdint>

namespace bd::engine {

// bdLoaderInit initializes 128 inline records; sub_82129030 visits the same
// range. Each record is 124 bytes, starting at Loader+144. Not eight slots.
inline constexpr uint32_t kAssetSlotCount = 128;

template <typename ReadState>
bool AnyAssetSlotLoading(ReadState &&read_state) {
  for (uint32_t i = 0; i < kAssetSlotCount; ++i)
    // bdAssetSlotCheckLoaded: unsigned (state - 1) <= 2.
    if (read_state(i) - 1u <= 2u)
      return true;
  return false;
}

// Loader::vf03 (0x82129660..0x82129724): the persistent D2AnimeTask is
// shown/hidden, not destroyed. Fade+132 gates both it and the fallback strip.
// Observes the current task visibility, rather than predicting its next update.
inline bool LoadingIconVisible(float fade, bool task_present,
                               uint32_t task_visible, bool force_strip,
                               int32_t strip_tick) {
  if (!(fade > 0.0f))
    return false;
  return force_strip || !task_present ? strip_tick >= 4 : task_visible != 0;
}

} // namespace bd::engine
