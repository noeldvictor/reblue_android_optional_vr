/**
 * @brief Stable queue runs that cannot cross ordered native draw boundaries.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <algorithm>
#include <span>

namespace bd::gpu {
// A barrier owns a singleton run. Both sorting and depth-prepass scheduling
// use these boundaries, so a later legacy prepass cannot leap over native water.
template <class Draw, size_t Extent, class Barrier>
size_t DrawOrderRunEnd(std::span<Draw,Extent> draws, size_t first, Barrier barrier) {
  if (first >= draws.size()) return draws.size();
  size_t end = first+1;
  if (!barrier(draws[first])) while (end < draws.size() && !barrier(draws[end])) ++end;
  return end;
}
template <class Draw, size_t Extent, class Barrier, class Compare>
void StableSortDrawRuns(std::span<Draw,Extent> draws, Barrier barrier, Compare compare) {
  for (size_t first = 0; first < draws.size();) {
    const auto end = DrawOrderRunEnd(draws,first,barrier);
    if (end-first > 1) std::stable_sort(draws.begin()+first,draws.begin()+end,compare);
    first = end;
  }
}
} // namespace bd::gpu
