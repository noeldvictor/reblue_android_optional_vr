/**
 * @brief   Whole-view host scheduling with explicit temporary engine imports.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
struct PPCContext;
namespace bd::gpu::scene {
// Refusal is before effects. Never re-enter the original parent after a pass.
bool TryScheduleRenderView(PPCContext &ctx, uint8_t *base);
} // namespace bd::gpu::scene
