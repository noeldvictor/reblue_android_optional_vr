/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
struct PPCContext;
namespace bd::gpu::hooks {
void DrawNativeSortedVisuals(PPCContext &ctx, uint8_t *base);
}
