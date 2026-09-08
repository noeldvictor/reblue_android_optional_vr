/**
 * @brief Outgoing ordinary-visual material state for unmigrated consumers only.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/visual_schedule_import.h"
#include <initializer_list>

namespace bd::gpu::scene {
// PPC slw uses the low six bits, including the zero-result bit. This is an
// authored class import, not a native shader/register layout.
constexpr bool DeferredDiffuseClassEnabled(uint32_t category, uint32_t mask) {
  return !(category & 32) && (mask & (1u << (category & 31)));
}
template <class Port> void BeginDeferredMaterialCompatibility(Port &port) {
  if (DeferredDiffuseClassEnabled(port.Category(), port.DiffuseMask())) return;
  // The deferred shader callback does not rewrite the copied entry material.
  // Preserve its writes solely for subsequent compatibility draws/getters.
  for (uint32_t offset : {0u, 4u, 8u, 12u, 80u, 84u, 88u, 92u}) port.Store(offset, 0);
  port.Store(372, 1); port.Store(376, 1);
  port.Store(408, port.Staging(408) + 2);
  port.SetRestore(1);
}
template <class Port> void EndDeferredMaterialCompatibility(Port &port) {
  if (port.Restore() != 1) return;
  // Read at end, not begin: intervening legacy material work may change this
  // saved colour. Scalar loads quiet NaNs; transport must not reinterpret it.
  for (uint32_t i = 0; i < 4; ++i) port.Store(i * 4, VisualScalarWord(port.SavedColour(i)));
  port.Store(408, port.Staging(408) + 1); port.Store(372, 1);
  for (uint32_t i = 0; i < 4; ++i) port.Store(80 + i * 4, VisualScalarWord(port.SavedColour(i)));
  port.Store(376, 1); port.Store(408, port.Staging(408) + 1);
  port.SetRestore(0);
}
} // namespace bd::gpu::scene
