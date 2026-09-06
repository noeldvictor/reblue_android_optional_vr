/**
 * @brief Temporary authored-call boundary for native Toon material callbacks.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
struct PPCContext;
namespace bd::gpu::scene {
// False means no side effects: unknown/disabled/refused input remains at the
// counted compatibility dispatcher. Native pass dispatch uses this directly.
bool TryNativeToonMaterial(uint32_t callback, PPCContext &ctx, uint8_t *base);
} // namespace bd::gpu::scene
