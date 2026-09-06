/**
 * @brief Complete deferred-visual pass using a native scene snapshot.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_deferred_visuals.h"
#include "gpu/scene/native_scene_snapshot.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/visual_schedule_import.h"
#include "gpu/hooks/native_ui.h"
#include "gpu/native_post_images.h"
#include "gpu/native_texture_mirror.h"
#include "gpu/constant_buffers.h"
#include "gpu/draw_queue.h"
#include "gpu/frame.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <initializer_list>

REX_EXTERN(__imp__sub_824252D0);
REX_EXTERN(bdBeginRenderPass);
REX_EXTERN(bdEndRenderPass);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(bdSetSamplerState);
REX_EXTERN(bdSetVertexDeclarationCached);
REX_EXTERN(bdInitDefaultTextures);
REX_EXTERN(D3DDevice_SetTexture);
REXCVAR_DEFINE_BOOL(bd_native_deferred_visuals, true, kCvarGroup,
    "Host deferred visual scheduling and native scene snapshot at the scene extent.");
REXCVAR_DECLARE(bool, bd_native_scene_passes);

namespace bd::gpu::hooks {
namespace {
using namespace scene;
constexpr uint32_t kQueues = (uint32_t(-32035) << 16) - 26748;
constexpr uint32_t kContext = (uint32_t(-32035) << 16) - 26552;
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
constexpr uint32_t kEngine = (uint32_t(-32034) << 16) - 19936;
constexpr uint32_t kChoices = (uint32_t(-32137) << 16) + 29804;
struct Stats {
  uint64_t native = 0, empty = 0, snapshots = 0, primitives = 0, translated = 0;
  uint64_t switches = 0, compatibility = 0, refused = 0, faults = 0, pass_adapters = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
thread_local bool active = false;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-deferred-visuals] native {} empty {} snapshots {} primitives {} translated {} switches {}; "
          "compatibility {} refused {} faults {} pass adapters {}; native scene-sized snapshot/schedule, "
          "authored queue/vertices and shader/state/texture/getter adapters remain",
      stats.native, stats.empty, stats.snapshots, stats.primitives, stats.translated, stats.switches,
      stats.compatibility, stats.refused, stats.faults, stats.pass_adapters);
  stats.frame = frame; stats.reported = true;
}
void Check(bool valid) {
  if (valid) return;
  ++stats.faults; stats.reported = false; Report();
  throw std::runtime_error("Native deferred visual pass lost a validated input");
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address > UINT32_MAX || bytes > UINT32_MAX ||
      address + bytes - 1 > UINT32_MAX || !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
std::optional<uint32_t> Word(uint64_t address) {
  if ((address & 3) || !Range(address, 4)) return {};
  return bd::mem::load<uint32_t>(uint32_t(address));
}
uint32_t Read(uint64_t address) { const auto value = Word(address); Check(value.has_value()); return *value; }
void Write(uint64_t address, uint32_t value) {
  Check(Word(address).has_value()); bd::mem::store<uint32_t>(uint32_t(address), value);
}
NativeSceneCommands *ActiveScene(VideoState &s) {
  return s.render_target && s.depth_stencil
      ? ActiveNativeSceneCommands(s.render_target->texture, s.depth_stencil->texture) : nullptr;
}
struct Adapter {
  PPCContext &ctx;
  uint8_t *base;
  const uint64_t saved_stack;
  NativePostImageHandle snapshot;
  Adapter(PPCContext &context, uint8_t *memory) : ctx(context), base(memory), saved_stack(ctx.r1.u64) {
    active = true;
    ctx.r1.u32 -= 224;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
  }
  ~Adapter() {
    ctx.r1.u64 = saved_stack; active = false;
    Video::MarkVSConstantsDirty(); Video::MarkPSConstantsDirty();
  }
  void Call(PPCFunc *function, std::initializer_list<uint64_t> arguments) {
    const std::array<PPCRegister *, 8> registers{
        &ctx.r3, &ctx.r4, &ctx.r5, &ctx.r6, &ctx.r7, &ctx.r8, &ctx.r9, &ctx.r10};
    Check(arguments.size() <= registers.size());
    size_t i = 0;
    for (auto argument : arguments) registers[i++]->u64 = argument;
    function(ctx, base);
    ctx.fpscr.disableFlushMode();
  }
  bool Snapshot() {
    // Getter is only an output ABI for remaining texture consumers. It never
    // chooses the source, resolution, sample count or layer count of the copy.
    auto *destination = ResolveGuestTexture(Read(kQueues + 1060));
    if (!destination) return false; // resource lookup may wait; outside video lock
    SampledImage source;
    {
      auto &s = state(); std::lock_guard lock(s.mutex);
      auto *scope = ActiveScene(s);
      if (!s.ready || !scope) return false;
      source = scope->ColorReadImage();
    }
    if (!source) return false;
    snapshot = AcquireNativePostImage(source.width, source.height, source.layers);
    const NativeImageLease lease{snapshot, snapshot ? snapshot->Output().image : SampledImage{}};
    constexpr auto extent = NativeImageExtentPolicy::AdoptSource;
    if (!snapshot || !Video::CanPublishNativeImage(lease, destination, extent)) return false;
    {
      auto &s = state(); std::lock_guard lock(s.mutex);
      auto *scope = ActiveScene(s);
      if (!scope || scope->ColorReadImage().texture != source.texture ||
          !CanCopySceneSnapshot(*scope, lease.image)) return false;
      // Everything after this point is committed: never replay the parent.
      BeginCommandList(s); Check(s.command_list_open);
      DrawQueueFlushAt(s.command_list, BD_FLUSH_SITE);
      BindNativeSceneCommands(s, *scope);
      ApplyNativeSceneClear(s, *scope);
      Check(CopySceneSnapshot(*s.command_list, *scope, lease.image));
      NoteBarrierCall(3, BarrierSite::Resolve);
      s.plume_framebuffer_bound = s.draw_framebuffer_bound = false;
      s.bound_fb_rt = s.bound_fb_ds = nullptr;
    }
    Check(Video::PublishNativeImage(lease, destination, false, extent));
    Video::SetTexture(8, destination);
    Call(bdSetSamplerState, {8, 0, 1}); Call(bdSetSamplerState, {8, 4, 1});
    ++stats.snapshots;
    return true;
  }
  uint32_t Count() { const auto count = Read(kQueues + 1052); Check(count <= kDeferredVisualLimit); return count; }
  uint32_t Entry(uint32_t index) {
    Check(index < kDeferredVisualLimit);
    const auto entry = Read(uint64_t(Read(kQueues)) + index * 4);
    Check(!(entry & 3) && Range(entry, 52)); return entry;
  }
  uint32_t Flags(uint32_t index) { return Read(uint64_t(Entry(index)) + 36); }
  void Begin() { Call(bdBeginRenderPass, {Read(kContext), 5}); ++stats.pass_adapters; }
  void SelectMode(uint32_t mode) {
    const auto offset = mode == 5 ? 72u : 84u;
    const auto declaration = Read(kChoices + offset + 8);
    if (declaration) Call(bdSetVertexDeclarationCached, {declaration});
    Write(kEngine + 96, Read(kChoices + offset));
    Write(kEngine + 100, Read(kChoices + offset + 4));
    Call(bdInitDefaultTextures, {0}); ++stats.switches;
  }
  void Primitive(uint32_t index, bool translated) {
    // Reload after shader callbacks, then retain this entry through its draw.
    const uint64_t entry = Entry(index);
    Call(bdSetRenderState, {48, !(Read(entry + 36) & 8)});
    if (Read(kQueues + 1064)) Call(bdSetRenderState, {40, !(Read(entry + 36) & 128)});
    const auto blend = ImportVisualBlend(Read(entry + 36));
    Call(bdSetRenderState, {72, blend.source}); Call(bdSetRenderState, {76, blend.destination});
    Call(D3DDevice_SetTexture, {Read(kDevice), 0, Read(entry + 8), 1ull << 43});
    Call(DrawNativeImmediateUi, {Read(entry + 32), Read(entry + 12), entry + 16, translated ? entry + 40 : 0});
    ++stats.primitives; stats.translated += translated;
  }
  void End() { Call(bdEndRenderPass, {Read(kContext)}); ++stats.pass_adapters; }
  void Clear() { Write(kQueues + 1052, 0); }
};
bool Prepare(PPCContext &ctx, uint32_t count) {
  if (count > kDeferredVisualLimit || ctx.r1.u32 < 4096 || (ctx.r1.u32 & 15) ||
      !Range(uint64_t(ctx.r1.u32) - 4096, 4096)) return false;
  const auto scratch = uint64_t(ctx.r1.u32) - 224;
  const auto safe = [scratch](uint64_t address, uint64_t bytes) {
    return Range(address, bytes) && !(address < scratch + 224 && scratch < address + bytes);
  };
  for (auto [address, bytes] : std::array<std::pair<uint32_t, uint32_t>, 5>{{
      {kQueues, 1068}, {kContext, 4}, {kDevice, 4}, {kEngine, 104}, {kChoices, 96}}})
    if (!safe(address, bytes)) return false;
  if (!safe(Read(kContext), 16) || !safe(Read(kDevice), 9984)) return false;
  const auto list = Read(kQueues);
  if ((list & 3) || !safe(list, uint64_t(count) * 4)) return false;
  for (uint32_t index = 0; index < count; ++index) {
    const auto entry = Read(uint64_t(list) + index * 4);
    if ((entry & 3) || !safe(entry, 52)) return false;
  }
  return true;
}
} // namespace
} // namespace bd::gpu::hooks

REX_HOOK_RAW(sub_824252D0) {
  using namespace bd::gpu;
  using namespace bd::gpu::hooks;
  const bool enabled = REXCVAR_GET(bd_native_deferred_visuals);
  const auto count = Word(kQueues + 1052);
  if (enabled && !active && count && !*count) { ++stats.empty; Report(); return; }
  if (enabled && REXCVAR_GET(bd_native_scene_passes) && !active && count && Prepare(ctx, *count)) {
    Adapter adapter(ctx, base);
    if (scene::ExecuteDeferredVisuals(adapter, *count)) { ++stats.native; Report(); return; }
  }
  ++stats.compatibility; stats.refused += enabled;
  LegacyShaderParameterScope parameters;
  __imp__sub_824252D0(ctx, base);
  Video::MarkVSConstantsDirty(); Video::MarkPSConstantsDirty(); Report();
}
