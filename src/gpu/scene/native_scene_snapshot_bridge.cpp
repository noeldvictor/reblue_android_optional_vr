/**
 * @brief   Water/refraction snapshots from explicitly owned native scene images.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_scene_snapshot.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/native_post_images.h"
#include "gpu/native_texture_mirror.h"
#include "gpu/frame.h"
#include "gpu/draw_queue.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <mutex>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_8221D2C8);
REXCVAR_DECLARE(bool, bd_native_scene_passes);

namespace bd::gpu::scene {
namespace {
// Temporary authored scheduling/getter inputs. They never select a GPU source
// by tiles, packed surface dimensions, resolve flags or a prior sampled slot.
constexpr uint32_t kPhase = (uint32_t(-32137) << 16) + 16476;
constexpr uint32_t kLastSubject = (uint32_t(-32035) << 16) - 26272;
constexpr uint32_t kShared = (uint32_t(-32035) << 16) - 26268;
constexpr uint32_t kReady = (uint32_t(-32035) << 16) - 26709;
constexpr uint32_t kSceneGetter = (uint32_t(-32035) << 16) - 26284;
constexpr uint32_t kReflectionGetter = (uint32_t(-32035) << 16) - 26280;
struct Stats {
  uint64_t calls = 0, copies = 0, reused = 0, inactive = 0, compatibility = 0, faults = 0;
  uint64_t input_refusals = 0, scope_refusals = 0, shape_refusals = 0, output_refusals = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-scene-snapshot] calls {} copies {} reused {} inactive {} compatibility {} faults {}; "
          "refused inputs {} scope {} shape {} output {}; native image copy/lease, authored timing/getter adapter remains",
          stats.calls, stats.copies, stats.reused, stats.inactive, stats.compatibility, stats.faults,
          stats.input_refusals, stats.scope_refusals, stats.shape_refusals, stats.output_refusals);
  stats.frame = frame;
  stats.reported = true;
}
template <class T> std::optional<T> Read(uint32_t address) {
  const auto *value = bd::mem::try_at<const T>(address);
  return value ? std::optional<T>(bd::mem::load<T>(address)) : std::nullopt;
}
void Check(bool value) {
  if (!value) { ++stats.faults; throw std::runtime_error("Native scene snapshot lost a validated boundary"); }
}
NativeSceneCommands *Active(VideoState &s) {
  return s.render_target && s.depth_stencil && s.render_target->texture && s.depth_stencil->texture
      ? ActiveNativeSceneCommands(s.render_target->texture, s.depth_stencil->texture) : nullptr;
}
bool TrySnapshot(PPCContext &ctx) {
  if (!REXCVAR_GET(bd_native_scene_passes)) return false;
  ++stats.calls;
  const auto phase_value = Read<uint32_t>(kPhase);
  if (!phase_value) { ++stats.input_refusals; return false; }
  const auto phase = *phase_value == 3 ? SceneSnapshotPhase::Scene :
      *phase_value == 5 ? SceneSnapshotPhase::Reflection : SceneSnapshotPhase::Inactive;
  if (phase == SceneSnapshotPhase::Inactive) { ++stats.inactive; return true; }
  const auto last = Read<uint32_t>(kLastSubject);
  const auto shared = Read<uint8_t>(kShared), ready = Read<uint8_t>(kReady);
  const auto getter = Read<uint32_t>(phase == SceneSnapshotPhase::Scene ? kSceneGetter : kReflectionGetter);
  if (!last || !shared || !ready || !getter || !*getter) { ++stats.input_refusals; return false; }
  const auto subject = ctx.r4.u32;
  const auto plan = PlanSceneSnapshot(phase, *last == subject, *shared != 0, *ready != 0);
  auto *destination = ResolveGuestTexture(*getter); // may wait for IO; never under video lock
  if (!destination) { ++stats.input_refusals; return false; }
  if (!plan.refresh) {
    {
      auto &s = state();
      std::lock_guard lock(s.mutex);
      // An inherited pre-conversion snapshot is not interchangeable with the
      // current live scene. Leave its exact contents to the compatibility path.
      if (!destination->nativeImage || !destination->nativeImage.Fits(
          destination->width, destination->height, destination->layers)) {
        ++stats.output_refusals; return false;
      }
    }
    Video::SetTexture(13, destination);
    ++stats.reused;
    return true;
  }
  SampledImage source;
  {
    auto &s = state();
    std::lock_guard lock(s.mutex);
    auto *scope = Active(s);
    if (!s.ready || !scope) { ++stats.scope_refusals; return false; }
    source = scope->ColorReadImage();
    if (!source) { ++stats.shape_refusals; return false; }
  }
  const auto snapshot = AcquireNativePostImage(source.width, source.height, source.layers);
  const NativeImageLease lease{snapshot, snapshot ? snapshot->Output().image : SampledImage{}};
  // The getter was originally allocated at a fixed design-canvas size. The
  // native scene now owns the snapshot extent, including both stereo layers.
  constexpr auto extent = NativeImageExtentPolicy::AdoptSource;
  if (!snapshot || !Video::CanPublishNativeImage(lease, destination, extent)) {
    ++stats.output_refusals; return false;
  }
  {
    auto &s = state();
    std::lock_guard lock(s.mutex);
    auto *scope = Active(s);
    if (!scope || scope->ColorReadImage().texture != source.texture || !CanCopySceneSnapshot(*scope, lease.image)) {
      ++stats.scope_refusals; return false; // still before any GPU recording/publication
    }
    BeginCommandList(s);
    Check(s.command_list_open);
    DrawQueueFlushAt(s.command_list, BD_FLUSH_SITE);
    BindNativeSceneCommands(s, *scope);
    ApplyNativeSceneClear(s, *scope);
    Check(CopySceneSnapshot(*s.command_list, *scope, lease.image));
    NoteBarrierCall(3, BarrierSite::Resolve);
    s.plume_framebuffer_bound = s.draw_framebuffer_bound = false;
    s.bound_fb_rt = s.bound_fb_ds = nullptr;
  }
  // The adapter borrows this image/descriptor/layout; no D3D resolve, mutable
  // backing copy, tile publication or sourceSurface relationship is created.
  Check(Video::PublishNativeImage(lease, destination, false, extent));
  if (plan.publish_cache) {
    bd::mem::store<uint32_t>(kLastSubject, subject);
    bd::mem::store<uint8_t>(kReady, 1);
  }
  Video::SetTexture(13, destination);
  ++stats.copies;
  return true;
}
} // namespace
} // namespace bd::gpu::scene
REX_HOOK_RAW(sub_8221D2C8) {
  using namespace bd::gpu::scene;
  if (!TrySnapshot(ctx)) { ++stats.compatibility; __imp__sub_8221D2C8(ctx, base); }
  Report();
}
