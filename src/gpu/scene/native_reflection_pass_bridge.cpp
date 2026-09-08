/**
 * @brief Native planar reflection attachments, clear, snapshot scope and output.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_reflection_pass.h"
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
#include "gpu/native_post_images.h"
#include "gpu/resource_bridge.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>
#include <vector>

REX_EXTERN(__imp__sub_821875F8);
REX_EXTERN(__imp__sub_821877C8);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(sub_82184A88);
REX_EXTERN(bdPlaneReflectUpdateTexture);
REX_EXTERN(bdBuildViewMatrix);
REX_EXTERN(sub_82186840);
REX_EXTERN(sub_821764F8);
REXCVAR_DECLARE(bool, bd_native_scene_passes);
REXCVAR_DECLARE(bool, bd_host_targets);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kEngine = (uint32_t(-32034)<<16)-19936;
constexpr uint32_t kPassMode = (uint32_t(-32036)<<16)-5536;
constexpr uint32_t kPrimary = (uint32_t(-32035)<<16)+24832;
// sub_82454720 always selects this plane, not the most recently drawn plane.
constexpr uint32_t kWaterPlane = (uint32_t(-32035)<<16)+29040;
struct ReflectionPass {
  uint32_t source = 0, plane = 0;
  GuestTexture *color = nullptr, *depth = nullptr, *output = nullptr;
  size_t nesting = 0;
  NativeSceneFramebufferHandle framebuffer;
  std::optional<NativeSceneCommands> commands;
  NativeImageLease image;
};
thread_local std::vector<ReflectionPass> passes;
thread_local NativeReflectionPublication water_reflection;
struct Stats {
  uint64_t begins = 0, ends = 0, compatibility_begin = 0, compatibility_end = 0;
  uint64_t outputs = 0, empty_clears = 0, cameras = 0, missing_camera = 0, faults = 0;
  uint64_t water_outputs = 0, water_reads = 0;
  uint32_t frame = 0;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (frame-stats.frame < 300) return;
  BD_INFO("[native-reflection-pass] begins {} ends {} active {}; compatibility {} {}; "
      "publications {} empty clears {} cameras {} missing {} faults {}; "
      "native HDR/depth/framebuffer/clear/output, no console surface or resolve; "
      "authored camera/extent/getter and draw adapters remain",
      stats.begins,stats.ends,passes.size(),stats.compatibility_begin,stats.compatibility_end,
      stats.outputs,stats.empty_clears,stats.cameras,stats.missing_camera,stats.faults);
  stats.frame = frame;
  BD_INFO("[native-reflection-images] frame {} water publications {} reads {}; exact completed water-plane lease, no sampled-getter import",
      frame,stats.water_outputs,stats.water_reads);
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address+bytes-1 > UINT32_MAX ||
      !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095))+4096; page < address+bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) { return !(address & 3) && Range(address,bytes); }
void Check(bool valid, const char *message) {
  if (!valid) { ++stats.faults; Report(); throw std::runtime_error(message); }
}
GuestTexture *Texture(uint32_t address) {
  ResourceType type;
  if (!HostResourceHeap::GetType(address,&type) || type != ResourceType::Texture) return nullptr;
  auto *image = HostResourceHeap::FromGuest<GuestTexture>(address);
  return image && image->texture ? image : nullptr;
}
struct CallFrame {
  PPCContext &ctx;
  uint64_t saved;
  explicit CallFrame(PPCContext &context) : ctx(context), saved(ctx.r1.u64) {
    ctx.r1.u32 -= 128; bd::mem::store<uint32_t>(ctx.r1.u32,uint32_t(saved)); ctx.fpscr.disableFlushMode();
  }
  ~CallFrame() { ctx.r1.u64 = saved; ctx.fpscr.disableFlushMode(); }
};
void SetState(PPCContext &ctx, uint8_t *base, uint32_t offset, uint32_t value) {
  ctx.r3.u64 = offset; ctx.r4.u64 = value; bdSetRenderState(ctx,base);
}
bool Begin(PPCContext &ctx, uint8_t *base, uint32_t source) {
  if (!REXCVAR_GET(bd_native_scene_passes) || !REXCVAR_GET(bd_host_targets) ||
      !Words(source,44) || !Words(kEngine,54624) || !Range(kPassMode,1) ||
      !Words(kRenderViewIdVa,4) || !Words(kPrimary,420) || !CanEnterNativePass() ||
      bd::mem::load<uint32_t>(source+28) || bd::mem::load<uint32_t>(source+36) ||
      ctx.r1.u32 < 2048 || (ctx.r1.u32 & 15) || !Words(uint64_t(ctx.r1.u32)-2048,2120)) return false;
  const auto camera = bd::mem::load<uint32_t>(source+12);
  const auto plane = bd::mem::load<uint32_t>(source+40);
  if (!Words(camera,300) || !Words(plane,280) ||
      bd::mem::load<uint32_t>(kRenderViewIdVa) >= 16) return false;
  // Eligibility ends here. The original extent/getter update is still an
  // authored adapter and may replace its output header; never replay it after
  // native side effects. Its sampleable allocation is replaced at publication.
  CallFrame frame(ctx);
  for (auto [offset,value] : std::array<std::pair<uint32_t,uint32_t>,7>{{
      {60,1},{72,6},{76,7},{104,6},{96,1},{100,8},{40,1}}}) SetState(ctx,base,offset,value);
  sub_82184A88(ctx,base);
  ctx.r3.u64 = plane; bdPlaneReflectUpdateTexture(ctx,base);
  Check(bd::mem::load<uint32_t>(source+40) == plane,"Reflection plane changed during extent publication");
  const auto width = bd::mem::load<uint32_t>(plane+168), height = bd::mem::load<uint32_t>(plane+172);
  auto *output = Texture(bd::mem::load<uint32_t>(plane+12));
  Check(width && width <= 1280 && height && height <= 720 && output,"Native reflection extent/getter unavailable");
  // An earlier plane/getter/queued reader must never be overwritten. The
  // existing exclusive HDR pool supplies the actual render AND sampled image.
  const auto reflection = AcquireNativePostImage(width,height,1);
  const auto image = NativeImageLease::From(reflection);
  auto *color = CreateNativeColorAttachmentAdapter(image);
  auto *depth = HostTargetAcquireNative(HostTargetClass::ReflectionDepth,
      {width,height,1,plume::RenderFormat::D32_FLOAT_S8_UINT,1});
  if (!color || !depth) {
    if (color) ReleaseResourceAdapter(color->selfVa);
    if (depth) ReleaseResourceAdapter(depth->selfVa);
    Check(false,"Native reflection attachment ownership unavailable");
  }
  auto framebuffer = AcquireNativeLeasedColorFramebuffer(image,depth->nativeTarget);
  const uint32_t clear = bd::mem::load<uint32_t>(camera+8);
  const plume::RenderColor rgba(float((clear>>16)&255)/255.f,float((clear>>8)&255)/255.f,
      float(clear&255)/255.f,float(clear>>24)/255.f);
  auto commands = framebuffer ? NativeSceneCommands::CreateLeasedColor(image,depth->nativeTarget,
      framebuffer->framebuffer.get(),{rgba,1.f,0}) : std::nullopt;
  if (!commands || !Video::CanPublishNativeImage(image,output,NativeImageExtentPolicy::AdoptSource)) {
    ReleaseResourceAdapter(color->selfVa); ReleaseResourceAdapter(depth->selfVa);
    Check(false,"Native reflection framebuffer/output preflight failed");
  }
  bd::mem::store<uint32_t>(source+28,color->selfVa); bd::mem::store<uint32_t>(source+36,depth->selfVa);
  uint32_t result = 0;
  Check(EnterNativePass(color,depth,result),"Native reflection pass could not enter");
  Check(RetainResourceAdapter(output->selfVa) != 0,"Native reflection lost its output adapter");
  passes.push_back({source,plane,color,depth,output,NativePassDepth(),std::move(framebuffer),std::move(commands),image});
  {
    auto &s = state(); std::lock_guard lock(s.mutex);
    s.frame_present_committed = false; BeginCommandList(s);
    Check(s.command_list_open,"Native reflection cannot record commands");
    s.draw_framebuffer_bound = false; s.clear_pending = false; s.clear_flags = 0;
  }
  ctx.r3.u64 = 0; ctx.r4.u64 = plane+16; ctx.r5.u64 = plane+80; bdBuildViewMatrix(ctx,base);
  for (uint32_t axis=0; axis<3; ++axis)
    bd::mem::store<float>(kEngine+54608+axis*4,bd::mem::load<float>(camera+288+axis*4));
  bd::mem::store<float>(kEngine+54620,1.f); bd::mem::store<uint32_t>(kEngine+4,1);
  ctx.r3.u64 = 2; sub_82186840(ctx,base);
  ctx.r3.u64 = kPrimary; sub_821764F8(ctx,base); // existing native receiver parameter producer
  bd::mem::store<uint8_t>(kPassMode,1);
  const auto owned_camera = passes.back().commands->Camera(FrameStatFrameCount(),bd::mem::load<uint32_t>(kRenderViewIdVa));
  stats.cameras += bool(owned_camera); stats.missing_camera += !owned_camera;
  ++stats.begins;
  return true;
}
bool End(PPCContext &ctx, uint8_t *base, uint32_t source) {
  if (passes.empty() || !passes.back().commands) return false;
  auto &pass = passes.back();
  Check(source == pass.source && Words(source,44) && NativePassDepth() == pass.nesting &&
      bd::mem::load<uint32_t>(source+28) == pass.color->selfVa &&
      bd::mem::load<uint32_t>(source+36) == pass.depth->selfVa &&
      bd::mem::load<uint32_t>(source+40) == pass.plane &&
      bd::mem::load<uint32_t>(pass.plane+12) == pass.output->selfVa &&
      state().render_target == pass.color && state().depth_stencil == pass.depth,
      "Native reflection attachment, output or nesting changed");
  if (pass.commands->ClearPending() || pass.color->hostClearFlags || pass.depth->hostClearFlags) {
    Check(Video::BindDrawFramebuffer(),"Native reflection empty clear failed"); ++stats.empty_clears;
  }
  {
    auto &s = state(); std::lock_guard lock(s.mutex);
    DrawQueueFlush(s.command_list);
    const bool barrier = *pass.image.image.layout != plume::RenderTextureLayout::SHADER_READ;
    Check(FinishNativeReflection(*s.command_list,*pass.commands,pass.image),"Native reflection completion failed");
    if (barrier) NoteBarrierCall(1,BarrierSite::Resolve);
    s.plume_framebuffer_bound = s.draw_framebuffer_bound = false; s.bound_fb_rt = s.bound_fb_ds = nullptr;
  }
  Check(Video::PublishNativeImage(pass.image,pass.output,false,NativeImageExtentPolicy::AdoptSource),
      "Native reflection output publication failed");
  Check(pass.output->nativeImage == pass.image && !pass.output->sourceSurface &&
      &pass.output->layout.Get() == pass.image.image.layout,"Native reflection output lost its owner");
  if (pass.plane == kWaterPlane) {
    Check(water_reflection.Complete(FrameStatFrameCount(),*pass.commands,pass.image),
        "Native water reflection completion lost its producing scope");
    ++stats.water_outputs;
  }
  ++stats.outputs;
  uint32_t result = 0; Check(LeaveNativePass(result),"Native reflection could not restore its previous pass");
  ReleaseResourceAdapter(pass.color->selfVa); ctx.r3.u64 = ReleaseResourceAdapter(pass.depth->selfVa);
  bd::mem::store<uint32_t>(source+28,0); bd::mem::store<uint32_t>(source+36,0);
  ReleaseResourceAdapter(pass.output->selfVa); bd::mem::store<uint8_t>(kPassMode,0);
  passes.pop_back(); ++stats.ends;
  return true;
}
} // namespace
NativeImageLease FindCompletedNativeWaterReflection() {
  auto image = water_reflection.Read(FrameStatFrameCount());
  stats.water_reads += bool(image);
  return image;
}
NativeSceneCommands *ActiveNativeReflectionCommands(plume::RenderTexture *color, plume::RenderTexture *depth) {
  if (passes.empty() || !passes.back().commands || NativePassDepth() != passes.back().nesting) return nullptr;
  auto &commands = *passes.back().commands;
  return commands.Matches(color,depth) ? &commands : nullptr;
}
} // namespace bd::gpu::scene
REX_HOOK_RAW(sub_821875F8) {
  using namespace bd::gpu::scene;
  const auto source = ctx.r3.u32;
  const bool water_plane = Words(source,44) && bd::mem::load<uint32_t>(source+40) == kWaterPlane;
  if (water_plane) water_reflection.Begin(bd::gpu::FrameStatFrameCount());
  if (!Begin(ctx,base,source)) {
    // An unknown/compatibility writer cannot leave a previous native publication
    // available as if it were the current completed water plane.
    water_reflection.Reset();
    ++stats.compatibility_begin; __imp__sub_821875F8(ctx,base); passes.push_back({source});
  }
  Report();
}
REX_HOOK_RAW(sub_821877C8) {
  using namespace bd::gpu::scene;
  const auto source = ctx.r3.u32;
  if (!End(ctx,base,source)) {
    Check(passes.empty() || passes.back().source == source,"Compatibility reflection end does not match begin");
    ++stats.compatibility_end; __imp__sub_821877C8(ctx,base);
    if (!passes.empty()) passes.pop_back();
  }
  Report();
}
