/**
 * @file    gpu/occlusion_cull.cpp
 * @brief   Native depth queries, owned observations and fence-gated history.
 *
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license   BSD 3-Clause License
 *            See LICENSE file in the project root for full license text.
 */
#include "gpu/occlusion_cull.h"
#include "gpu/native_occlusion_program.h"
#include "gpu/scene/native_scene_commands.h"
#include "gpu/device.h"
#include "gpu/constant_buffers.h"
#include "gpu/draw_bindings_bridge.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include <rex/cvar.h>
#include <mutex>
#include <vector>

REXCVAR_DECLARE(bool, bd_occlusion_cull);
REXCVAR_DECLARE(bool, bd_occlusion_diag);

namespace bd::gpu {
namespace {
struct Slot {
  std::unique_ptr<plume::RenderQueryPool> pool;
  std::vector<NativeOcclusionObservation> queries;
  bool pending = false;
};
struct Pipeline {
  NativeTargetShape shape;
  std::unique_ptr<plume::RenderPipeline> pipeline;
};
struct State {
  std::mutex mutex; // VideoState -> occlusion; never acquire VideoState while held.
  Slot slots[kNumFrames];
  uint32_t active = ~0u, diag_frame = 0;
  bool unsupported = false;
  NativeOcclusionTracker tracker;
  NativeOcclusionProgram program;
  std::vector<Pipeline> pipelines; // Four mono sample-count variants, bounded.
  uint64_t noted = 0, emitted = 0, collected = 0, zeros = 0, skipped = 0;
};
State &Get() { static State result; return result; }

plume::RenderPipeline *PipelineFor(State &o, VideoState &s, NativeTargetShape shape) {
  // Extent does not change the graphics pipeline; validate before canonicalizing.
  if (!shape.Bytes(512ull << 20) || shape.layers != 1) return nullptr;
  shape.width = shape.height = 1;
  for (const auto &entry : o.pipelines) if (entry.shape == shape) return entry.pipeline.get();
  if (o.pipelines.size() >= 4) return nullptr;
  if (!o.program) o.program = CreateNativeOcclusionProgram(*s.device);
  auto pipeline = CreateNativeOcclusionPipeline(*s.device, o.program, shape);
  auto *result = pipeline.get();
  o.pipelines.push_back({shape, std::move(pipeline)});
  if (!result) BD_ERROR("[native-occ] native query pipeline creation failed");
  return result;
}
}
void OcclusionCullFrameBegin(plume::RenderDevice *device, plume::RenderCommandList *cmd, u32 slot) {
  auto &o = Get();
  std::lock_guard lock(o.mutex);
  o.active = ~0u;
  o.tracker.Begin(FrameStatFrameCount());
  if (!REXCVAR_GET(bd_occlusion_cull) || o.unsupported || !device || !cmd || slot >= kNumFrames) return;
  auto &sl = o.slots[slot];
  if (!sl.pool) {
    sl.pool = device->createOcclusionQueryPool(NativeOcclusionTracker::kQueries);
    if (!sl.pool) {
      o.unsupported = true;
      BD_INFO("[native-occ] occlusion queries unavailable; nodes keep drawing");
      return;
    }
    sl.queries.reserve(NativeOcclusionTracker::kQueries);
  }
  sl.queries.clear();
  sl.pending = false;
  cmd->resetQueryPool(sl.pool.get(), 0, NativeOcclusionTracker::kQueries);
  o.active = slot;
}
void OcclusionCullCollect(u32 slot) {
  auto &o = Get();
  std::lock_guard lock(o.mutex);
  if (slot >= kNumFrames) return;
  auto &sl = o.slots[slot];
  if (!sl.pending || !sl.pool || sl.queries.empty()) return;
  sl.pending = false;
  sl.pool->queryResults(uint32_t(sl.queries.size()));
  const auto *results = sl.pool->getResults();
  if (!results) return;
  for (uint32_t n = 0; n < sl.queries.size(); ++n) {
    o.tracker.Collect(sl.queries[n], results[n] == 0);
    ++o.collected;
    o.zeros += results[n] == 0;
  }
  const auto frame = FrameStatFrameCount();
  if (frame-o.diag_frame >= 300 && REXCVAR_GET(bd_occlusion_diag)) {
    BD_INFO("[native-occ] frame {} noted {} queried {} fence-collected {} zero {} native-skipped {}; history {}; owned world bounds/camera/depth, no translated bindings",
        frame, o.noted, o.emitted, o.collected, o.zeros, o.skipped, o.tracker.HistoryCount());
    o.diag_frame = frame;
  }
}
void OcclusionCullNote(NativeOcclusionIdentity identity,
    const std::optional<NativeOcclusionView> &view, const std::array<float, 4> &sphere) {
  auto &o = Get();
  std::lock_guard lock(o.mutex);
  if (o.active >= kNumFrames) return;
  o.tracker.Note(identity, view, sphere);
  ++o.noted;
}
bool OcclusionCullOccluded(NativeOcclusionIdentity identity, const NativeOcclusionView &view) {
  auto &o = Get();
  std::lock_guard lock(o.mutex);
  const bool culled = o.active < kNumFrames && REXCVAR_GET(bd_occlusion_cull) && o.tracker.Occluded(identity, view);
  o.skipped += culled;
  return culled;
}
void OcclusionCullEmit(VideoState &s, const scene::NativeSceneCommands &commands) {
  auto &o = Get();
  std::lock_guard lock(o.mutex);
  const auto view = commands.OcclusionView(FrameStatFrameCount());
  if (o.active >= kNumFrames || !view || !s.command_list || !s.device) {
    o.tracker.EndPass();
    return;
  }
  auto *pipeline = PipelineFor(o, s, *commands.ColorShape());
  if (!pipeline) { o.tracker.EndPass(); return; }
  auto &sl = o.slots[o.active];
  auto *cmd = s.command_list;
  const auto resume = EngineGraphicsBindings(s); // Existing interop handoff, never query inputs.
  bool bound = false;
  WithNativeOcclusionBindings(*cmd, o.program.layout.get(), resume, [&] {
    o.tracker.Queries(*view, [&](const NativeOcclusionObservation &query) {
      if (sl.queries.size() >= NativeOcclusionTracker::kQueries) return;
      if (!bound) {
        commands.Bind(*cmd);
        cmd->setViewports(plume::RenderViewport(0, 0, float(view->scope.width), float(view->scope.height)));
        cmd->setScissors(plume::RenderRect(0, 0, view->scope.width, view->scope.height));
        cmd->setPipeline(pipeline);
        bound = true;
      }
      const auto index = uint32_t(sl.queries.size());
      sl.queries.push_back(query); // Immutable originating frame, not collection frame.
      cmd->setGraphicsPushConstants(0, &query.packet, 0, sizeof(query.packet));
      cmd->beginQuery(sl.pool.get(), index);
      cmd->drawInstanced(36, 1, 0, 0);
      cmd->endQuery(sl.pool.get(), index);
      ++o.emitted;
    });
  });
  sl.pending = !sl.queries.empty();
  o.tracker.EndPass();
  if (bound) {
    s.dirtyStates.pipelineState = true;
    InvalidateSharedBinding(); // Remaining interop consumers must rebind after us.
  }
}
} // namespace bd::gpu
