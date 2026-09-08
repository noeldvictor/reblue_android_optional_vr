/**
 * @file    gpu/occlusion_cull.h
 * @brief   Host occlusion culling on the scene walk: at the end of the scene
 *          pass a world-space cube proxy per owned node is drawn under an
 *          occlusion query, depth-tested against the pass's own depth; a
 *          node whose proxy passed no sample two frames running is not
 *          drawn when current owned inputs still match. Results arrive after
 *          the frame-slot fence; unsupported inputs keep drawing.
 *
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license   BSD 3-Clause License
 *            See LICENSE file in the project root for full license text.
 */
#pragma once

#include <plume_render_interface.h>
#include <rex/types.h>
#include "gpu/native_occlusion.h"

namespace bd::gpu {

struct VideoState;
namespace scene { class NativeSceneCommands; }

// Frame slot lifecycle, beside the fragment census: reset at command list
// begin, results read after the slot's fence.
void OcclusionCullFrameBegin(plume::RenderDevice *device,
                             plume::RenderCommandList *cmd, u32 slot);
void OcclusionCullCollect(u32 slot);

// Called only for a preflighted native primitive. Registers its current
// bounds and requests a query even when history permits skipping this draw.
bool OcclusionCullRequest(NativeOcclusionIdentity identity,
    const std::optional<NativeOcclusionView> &view, const std::optional<scene::NativeBounds> &bounds);

// At the scene pass's end, with its framebuffer still bound: draws this
// frame's proxies under queries. No-op unless bd_occlusion_cull and the
// scene pass's projection are known.
void OcclusionCullEmit(VideoState &s, const scene::NativeSceneCommands &commands);

} // namespace bd::gpu
