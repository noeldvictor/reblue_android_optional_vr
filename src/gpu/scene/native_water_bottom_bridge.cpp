/**
 * @brief Native water-bottom pass: depth ownership through ordered sampling.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_water_bottom.h"
#include "gpu/scene/native_pass_bridge.h"
#include "gpu/scene/native_scene_framebuffer.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/guest_scene.h"
#include "gpu/device.h"
#include "gpu/draw_queue.h"
#include "gpu/frame.h"
#include "gpu/frame_stats.h"
#include "gpu/host_resource_heap.h"
#include "gpu/host_targets.h"
#include "gpu/resource_bridge.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>
#include <vector>

REX_EXTERN(__imp__sub_82187878);
REX_EXTERN(__imp__sub_82187A00);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(sub_82184A88);
REX_EXTERN(sub_821795D8);
REX_EXTERN(bdBuildViewMatrix);
REX_EXTERN(sub_82186840);
REXCVAR_DECLARE(bool, bd_native_scene_passes);
REXCVAR_DECLARE(bool, bd_host_targets);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kBottom = (uint32_t(-32137) << 16) + 28452;
constexpr uint32_t kEngine = (uint32_t(-32034) << 16) - 19936;
constexpr uint32_t kPassMode = (uint32_t(-32036) << 16) - 5536;
struct BottomPass {
  uint32_t source = 0, view = ~0u;
  GuestTexture *depth = nullptr, *output = nullptr;
  size_t nesting = 0;
  NativeSceneFramebufferHandle framebuffer;
  std::optional<NativeSceneCommands> commands;
  NativeImageLease image;
};
thread_local std::vector<BottomPass> passes;
thread_local std::optional<NativeWaterBottom> completed;
thread_local uint32_t completed_frame = ~0u;
struct Stats {
  uint64_t begins = 0, ends = 0, compatibility_begin = 0, compatibility_end = 0;
  uint64_t outputs = 0, empty_clears = 0, projections = 0, missing_projection = 0, faults = 0;
  uint32_t frame = 0;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (frame - stats.frame < 300) return;
  BD_INFO("[native-water-bottom] begins {} ends {} active {}; compatibility {} {}; "
      "depth publications {} empty clears {} owned projections {} missing {} faults {}; "
      "native depth/framebuffer/clear, no console surface or resolve; camera fit and caster adapters remain",
      stats.begins, stats.ends, passes.size(), stats.compatibility_begin, stats.compatibility_end,
      stats.outputs, stats.empty_clears, stats.projections, stats.missing_projection, stats.faults);
  stats.frame = frame;
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address + bytes - 1 > UINT32_MAX ||
      !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) { return !(address & 3) && Range(address, bytes); }
void Check(bool valid, const char *message) {
  if (!valid) { ++stats.faults; throw std::runtime_error(message); }
}
GuestTexture *Texture(uint32_t address) {
  ResourceType type;
  if (!HostResourceHeap::GetType(address, &type) || type != ResourceType::Texture) return nullptr;
  auto *image = HostResourceHeap::FromGuest<GuestTexture>(address);
  return image && image->texture ? image : nullptr;
}
struct CallFrame {
  PPCContext &ctx;
  uint64_t saved;
  explicit CallFrame(PPCContext &context) : ctx(context), saved(ctx.r1.u64) {
    ctx.r1.u32 -= 128;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved));
    ctx.fpscr.disableFlushMode();
  }
  ~CallFrame() { ctx.r1.u64 = saved; ctx.fpscr.disableFlushMode(); }
};
void SetState(PPCContext &ctx, uint8_t *base, uint32_t offset, uint32_t value) {
  ctx.r3.u64 = offset; ctx.r4.u64 = value;
  bdSetRenderState(ctx, base); // native intent; outgoing getter/cache adapter
}
bool Begin(PPCContext &ctx, uint8_t *base, uint32_t source) {
  if (!REXCVAR_GET(bd_native_scene_passes) || !REXCVAR_GET(bd_host_targets) ||
      !Words(source,40) || !Words(kBottom,176) || !Words(kEngine,54624) ||
      !Range(kPassMode,1) || !Words(kRenderViewIdVa,4) || !CanEnterNativePass() ||
      bd::mem::load<uint32_t>(source+36) || ctx.r1.u32 < 2048 ||
      (ctx.r1.u32 & 15) || !Words(uint64_t(ctx.r1.u32)-2048,2120)) return false;
  const auto camera = bd::mem::load<uint32_t>(source+12);
  const auto view = bd::mem::load<uint32_t>(kRenderViewIdVa);
  auto *output = Texture(bd::mem::load<uint32_t>(kBottom+12));
  if (!Words(camera,312) || view >= 16 || !output) return false;
  // This named pass is depth-only and mono even in stereo: it projects world
  // geometry from above. Do not alias the sun slot or infer its role by size.
  auto *depth = HostTargetAcquireNative(HostTargetClass::WaterBottomDepth,
      {256,256,1,plume::RenderFormat::D32_FLOAT_S8_UINT,1});
  if (!depth) return false;
  const auto image = NativeImageLease::From(depth->nativeTarget);
  auto framebuffer = AcquireNativeSceneFramebuffer({NativeTargetImageHandle{},depth->nativeTarget},nullptr);
  auto commands = framebuffer ? NativeSceneCommands::CreateDepthOnly(
      depth->nativeTarget,framebuffer->framebuffer.get()) : std::nullopt;
  if (!commands || !Video::CanPublishNativeImage(image,output)) {
    ReleaseResourceAdapter(depth->selfVa); return false;
  }
  // After publication, any failure is terminal for this scope. Never replay
  // the old create/clear/resolve lifecycle after native side effects.
  CallFrame frame(ctx);
  SetState(ctx,base,60,0); SetState(ctx,base,72,6); SetState(ctx,base,76,7); SetState(ctx,base,40,1);
  sub_82184A88(ctx,base);
  bd::mem::store<uint32_t>(source+36,depth->selfVa);
  uint32_t result = 0;
  Check(EnterNativePass(nullptr,depth,result),"Native water bottom could not enter its pass");
  Check(RetainResourceAdapter(output->selfVa) != 0,"Native water bottom lost its output adapter");
  passes.push_back({source,view,depth,output,NativePassDepth(),std::move(framebuffer),std::move(commands),image});
  {
    auto &s = state(); std::lock_guard lock(s.mutex);
    s.frame_present_committed = false;
    BeginCommandList(s);
    Check(s.command_list_open,"Native water bottom cannot record commands");
    s.draw_framebuffer_bound = false;
    s.clear_pending = false; s.clear_flags = 0;
  }
  SetState(ctx,base,212,0);
  ctx.r3.u64 = kBottom; ctx.r4.u64 = camera+288; ctx.r5.u64 = camera+300;
  sub_821795D8(ctx,base); // remaining authored top-down camera fit, explicitly not a native producer
  ctx.r3.u64 = 0; ctx.r4.u64 = kBottom+16; ctx.r5.u64 = kBottom+80;
  bdBuildViewMatrix(ctx,base); // existing native transform producer owns the resulting camera
  for (uint32_t axis = 0; axis < 3; ++axis)
    bd::mem::store<float>(kEngine+54608+axis*4,bd::mem::load<float>(camera+288+axis*4));
  bd::mem::store<float>(kEngine+54620,1.f);
  bd::mem::store<uint32_t>(kEngine+4,1);
  ctx.r3.u64 = 3;
  sub_82186840(ctx,base); // existing native frustum/cache producer; preserve its result
  bd::mem::store<uint8_t>(kPassMode,0);
  ++stats.begins;
  return true;
}
bool End(PPCContext &ctx, uint8_t *base, uint32_t source) {
  if (passes.empty() || !passes.back().depth) return false;
  auto &pass = passes.back();
  Check(source == pass.source && NativePassDepth() == pass.nesting && Words(source,40) &&
      bd::mem::load<uint32_t>(source+36) == pass.depth->selfVa &&
      bd::mem::load<uint32_t>(kBottom+12) == pass.output->selfVa &&
      !state().render_target && state().depth_stencil == pass.depth,
      "Native water bottom attachment, output or nesting changed");
  if (pass.commands->ClearPending() || pass.depth->hostClearFlags) {
    Check(Video::BindDrawFramebuffer(),"Native water bottom empty clear failed");
    ++stats.empty_clears;
  }
  {
    auto &s = state(); std::lock_guard lock(s.mutex);
    DrawQueueFlush(s.command_list);
    const bool barrier = pass.depth->nativeTarget->layout != plume::RenderTextureLayout::SHADER_READ;
    Check(FinishNativeWaterBottom(*s.command_list,*pass.commands),"Native water bottom completion failed");
    if (barrier) NoteBarrierCall(1,BarrierSite::Resolve);
    s.plume_framebuffer_bound = s.draw_framebuffer_bound = false;
    s.bound_fb_rt = s.bound_fb_ds = nullptr;
  }
  completed = ReadNativeWaterBottom(*pass.commands,FrameStatFrameCount(),pass.view);
  completed_frame = FrameStatFrameCount();
  stats.projections += bool(completed); stats.missing_projection += !completed;
  Check(Video::PublishNativeImage(pass.image,pass.output,false),"Native water bottom output publication failed");
  Check(pass.output->nativeImage == pass.image && !pass.output->sourceSurface &&
      &pass.output->layout.Get() == pass.image.image.layout,"Native water bottom output borrowed a different owner");
  ++stats.outputs;
  CallFrame frame(ctx);
  SetState(ctx,base,212,7);
  uint32_t result = 0;
  Check(LeaveNativePass(result),"Native water bottom could not restore its previous pass");
  ctx.r3.u64 = ReleaseResourceAdapter(pass.depth->selfVa);
  bd::mem::store<uint32_t>(source+36,0);
  ReleaseResourceAdapter(pass.output->selfVa);
  bd::mem::store<uint8_t>(kPassMode,0);
  passes.pop_back(); ++stats.ends;
  return true;
}
} // namespace
NativeSceneCommands *ActiveNativeWaterBottomCommands(plume::RenderTexture *color, plume::RenderTexture *depth) {
  if (passes.empty() || !passes.back().commands || NativePassDepth() != passes.back().nesting) return nullptr;
  auto &commands = *passes.back().commands;
  return commands.Matches(color,depth) ? &commands : nullptr;
}
std::optional<NativeWaterBottom> FindCompletedNativeWaterBottom() {
  if (completed_frame != FrameStatFrameCount() || !completed ||
      *completed->image.image.layout != plume::RenderTextureLayout::SHADER_READ) return {};
  return completed;
}
} // namespace bd::gpu::scene
REX_HOOK_RAW(sub_82187878) {
  using namespace bd::gpu::scene;
  completed.reset(); completed_frame = ~0u;
  const auto source = ctx.r3.u32;
  if (!Begin(ctx,base,source)) {
    ++stats.compatibility_begin;
    __imp__sub_82187878(ctx,base);
    passes.push_back({source});
  }
  Report();
}
REX_HOOK_RAW(sub_82187A00) {
  using namespace bd::gpu::scene;
  const auto source = ctx.r3.u32;
  if (!End(ctx,base,source)) {
    Check(passes.empty() || passes.back().source == source,"Compatibility water bottom end does not match begin");
    ++stats.compatibility_end;
    __imp__sub_82187A00(ctx,base);
    if (!passes.empty()) passes.pop_back();
  }
  Report();
}
