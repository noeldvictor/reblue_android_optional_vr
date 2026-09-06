/**
 * @brief Immediate UI boundary and independent original-upload observer.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
struct PPCContext;
namespace bd::gpu::hooks {
void DrawNativeImmediateUi(PPCContext &ctx, uint8_t *base);
void ObserveOriginalImmediateUi(uint32_t device, uint32_t primitive,
                                uint32_t count, uint32_t stride, const uint8_t *bytes);
} // namespace bd::gpu::hooks
