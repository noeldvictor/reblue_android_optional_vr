/**
 * @file    gpu/hooks/draw_dispatch.h
 * @brief   The draw dispatch, callable from host code that issues a draw of
 *          its own (the host-issued node draw, gpu/scene/host_draw.cpp).
 *
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license   BSD 3-Clause License
 *            See LICENSE file in the project root for full license text.
 */
#pragma once

#include <rex/types.h>
#include <span>

namespace bd::gpu::hooks {

// The same path a guest DrawIndexedVertices / DrawVertices takes after its
// arguments are decoded: framebuffer bind, state flush, constant uploads, and
// the deferred queue. The caller has set the host state and constant sources
// it wants the draw to see.
void DispatchHostNodeDraw(u32 device_guest, u32 primitive_type, bool indexed,
                          u32 count, u32 start_index, i32 base_vertex,
                          u32 start_vertex);

// CPU-owned six-lane UI triangle strip; copied once into fence-managed host
// upload storage before this returns. No guest scratch or Begin/End state.
// device_guest is still the explicit state/texture/shader ABI adapter.
bool DispatchHostImmediateUi(u32 device_guest, std::span<const u32> words);

} // namespace bd::gpu::hooks
