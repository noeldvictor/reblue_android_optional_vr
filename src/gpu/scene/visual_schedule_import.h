/**
 * @brief Temporary authored visual flags and scalar publication semantics.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>

namespace bd::gpu::scene {
struct VisualBlendImport { uint32_t source, destination, mode; };
inline VisualBlendImport ImportVisualBlend(uint32_t flags) {
  // Equality, not bit priority: combinations select the ordinary alpha recipe.
  switch (flags & 0xff00) {
  case 0x100: return {6, 1, 1};
  case 0x200: return {0, 4, 2};
  case 0x400: return {0, 7, 4};
  case 0x800: return {9, 1, 3};
  case 0x1000: return {0, 5, 5};
  default: return {6, 7, 0};
  }
}
// Scalar load/store quiets signaling NaNs; raw matrix/vertex transport does not.
inline uint32_t VisualScalarWord(uint32_t word) {
  return (word & 0x7f800000u) == 0x7f800000u && (word & 0x7fffffu) ?
      word | 0x400000u : word;
}

// Complete model preparation order. The port exposes live authored fields,
// including mutations made by the two visual callbacks and bone initialization.
// No snapshot of flags, colours or scaling is retained across those calls.
template <class Port>
void PrepareSortedVisualModel(Port &port) {
  if (port.Entry(92) & 0x100000) {
    port.Store(3440, 1); port.Store(3000, 0);
    for (uint32_t i = 0; i < 4; ++i)
      port.Store(3444 + i * 4, VisualScalarWord(port.Entry(96 + (i & 1) * 4)));
  } else {
    const auto table = port.Table();
    port.Store(3000, 6);
    port.Store(4932, VisualScalarWord(port.Entry(96)));
    const auto callback = port.Method(table, 20);
    for (uint32_t i = 1; i < 8; ++i)
      port.Store(4932 + i * 4, VisualScalarWord(port.Entry(96 + (i & 1) * 4)));
    port.Callback(callback);
  }
  port.CopyMatrix(); port.InitBones();
  for (uint32_t i = 0; i < 4; ++i)
    port.Store(3004 + i * 4, VisualScalarWord(port.Entry(76 + i * 4)));
  port.State(48, !(port.Entry(92) & 8));
  if (port.DepthPolicy()) port.State(40, !(port.Entry(92) & 128));
  port.Store(3040, (port.Entry(92) >> 4) & 1);
  const auto blend = port.Blend(port.Entry(92));
  const auto callback = port.Method(port.Table(), 4);
  port.Store(1864, blend);
  port.Callback(callback);
}
} // namespace bd::gpu::scene
