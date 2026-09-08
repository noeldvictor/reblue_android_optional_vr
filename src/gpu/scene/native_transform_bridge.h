/**
 * @file    native_transform_bridge.h
 * @brief   Temporary engine import/publication around native transforms.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
#include "gpu/scene/native_transform.h"
struct PPCContext;
namespace bd::gpu::scene {
// r3/r4/r5 optionally supply world/view/projection. Null means inherited engine
// cache. An interpolated/XR view can arrive directly from native CPU memory.
void UpdateRenderTransforms(PPCContext &ctx, uint8_t *base,
                            const float *view_override = nullptr);
// Latest producer-owned values, not a readback of the engine getter matrix.
const RenderTransforms *GetNativeRenderTransforms();
// Native scene producer; only the outgoing compatibility getters/constants
// are exported. No source matrix, scratch allocation or original call.
bool PublishNativeWorld(const RenderMatrix &world);
} // namespace bd::gpu::scene
