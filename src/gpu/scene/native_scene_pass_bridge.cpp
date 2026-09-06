/**
 * @file    native_scene_pass_bridge.cpp
 * @brief   Whole scene begin/end ownership, with counted remaining producers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_scene_pass.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/native_scene_framebuffer.h"
#include "gpu/scene/scene_precision_import.h"
#include "gpu/scene_image.h"
#include "gpu/scene/native_pass_bridge.h"
#include "gpu/scene/native_view_bridge.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "gpu/frame.h"
#include "gpu/host_resource_heap.h"
#include "gpu/host_targets.h"
#include "gpu/native_target_images.h"
#include "gpu/foveation.h"
#include "gpu/resource_bridge.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>
#include <vector>

REX_EXTERN(__imp__sub_82186BA0);
REX_EXTERN(__imp__sub_82187010);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(sub_82184A88);
REX_EXTERN(bdBuildViewMatrix);
REX_EXTERN(sub_82186840);
REX_EXTERN(sub_821CCF48);
REX_EXTERN(sub_821764F8);
REX_EXTERN(sub_82179868);
REXCVAR_DECLARE(bool, bd_native_scene_passes);
REXCVAR_DECLARE(bool, bd_host_targets);
REXCVAR_DECLARE(i32, bd_render_scale);
REXCVAR_DECLARE(bool, bd_stereo_multiview);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kSettings = (uint32_t(-32035) << 16) - 26552;
constexpr uint32_t kTile = (uint32_t(-32035) << 16) - 26710;
constexpr uint32_t kView = (uint32_t(-32035) << 16) - 26424;
constexpr uint32_t kStack = (uint32_t(-32034) << 16) - 23232;
constexpr uint32_t kEngine = (uint32_t(-32034) << 16) - 19936;
constexpr uint32_t kViewCache = (uint32_t(-32035) << 16) + 32568;
constexpr uint32_t kFrustum = (uint32_t(-32033) << 16) - 30608;
constexpr uint32_t kPrimary = (uint32_t(-32035) << 16) + 24832;
constexpr uint32_t kArray = (uint32_t(-32035) << 16) + 25248;
constexpr uint32_t kCount = (uint32_t(-32137) << 16) + 16748;
constexpr uint32_t kLast = (uint32_t(-32035) << 16) + 28608;
constexpr uint32_t kSecondary = (uint32_t(-32137) << 16) + 28452;
constexpr uint32_t kHdr = (uint32_t(-32136) << 16) + 14888 + 12;
constexpr uint32_t kOne = (uint32_t(-32251) << 16) + 20908;
constexpr uint32_t kPhase = (uint32_t(-32137) << 16) + 16476;
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
struct ScenePass {
  uint32_t source = 0;
  GuestTexture *color = nullptr, *depth = nullptr;
  std::size_t nesting = 0;
  NativeSceneResolveHandle resolves;
  std::array<NativeImageLease, 2> source_images;
  NativeSceneFramebufferHandle framebuffer;
  std::optional<NativeSceneCommands> commands;
};
thread_local std::vector<ScenePass> scenes;
thread_local NativeSceneResultScope *current_result = nullptr;
struct Stats {
  uint64_t begins = 0, ends = 0, compatibility_begin = 0, compatibility_end = 0;
  uint64_t refused = 0, outputs = 0, null_outputs = 0, checked = 0, wrong = 0;
  uint64_t camera_calls = 0, state308_calls = 0, parameters = 0, empty_clears = 0;
  uint64_t completed = 0, consumed = 0, materialized_color = 0, materialized_depth = 0;
  uint64_t native_resolve_results = 0;
  uint64_t deferred_color = 0, recovered_color = 0;
  uint64_t native_depth_publications = 0, compatibility_depth_publications = 0;
  uint64_t command_binds = 0, native_clears = 0, compatibility_clears = 0;
  uint64_t precision_getters = 0;
  uint32_t frame = 0;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (frame - stats.frame < 300)
    return;
  BD_INFO("[native-scene] begins {} ends {} active {}; compatibility begin {} end {} "
          "refused {}; explicit outputs {} null {} empty clears {}; ownership checks {} wrong {}; "
          "view-cache entries {} state-308 adapters {} parameter adapters {}; "
          "post-chain/getter/resource adapters and engine traversal remain",
          stats.begins, stats.ends, scenes.size(), stats.compatibility_begin,
          stats.compatibility_end, stats.refused, stats.outputs, stats.null_outputs,
          stats.empty_clears, stats.checked, stats.wrong, stats.camera_calls,
          stats.state308_calls, stats.parameters);
  BD_INFO("[native-scene] completed image results {} consumed {}; materialized colour {} depth {}; "
          "per-view native pins, single-use inputs; output/resource publications remain",
          stats.completed, stats.consumed, stats.materialized_color, stats.materialized_depth);
  BD_INFO("[native-scene] native attachment resolve results {}; initial colour deferred {} recovered {}; "
          "depth getter and final UI boundaries remain",
          stats.native_resolve_results, stats.deferred_color, stats.recovered_color);
  BD_INFO("[native-scene] depth publications native {} compatibility {}; "
          "native depth borrows its image/descriptor/layout, no copy or resolve link",
          stats.native_depth_publications, stats.compatibility_depth_publications);
  BD_INFO("[native-scene] command binds {} native clears {} compatibility clears {}; "
          "precision getters {}; native write layouts and scope-owned clears, no scene clear flags or seed copies",
          stats.command_binds, stats.native_clears, stats.compatibility_clears, stats.precision_getters);
  stats.frame = frame;
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address + bytes - 1 > UINT32_MAX ||
      !bd::mem::try_at<uint8_t>(uint32_t(address)))
    return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096;
       page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page)))
      return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) {
  return !(address & 3) && Range(address, bytes);
}
void Check(bool same, const char *message) {
  ++stats.checked;
  if (!same) {
    ++stats.wrong;
    BD_ERROR("[native-scene] {}", message);
    throw std::runtime_error(message);
  }
}
GuestTexture *Texture(uint32_t address) {
  ResourceType type;
  if (!HostResourceHeap::GetType(address, &type) ||
      (type != ResourceType::Texture && type != ResourceType::RenderTarget &&
       type != ResourceType::DepthStencil))
    return nullptr;
  auto *image = HostResourceHeap::FromGuest<GuestTexture>(address);
  return image && image->texture ? image : nullptr;
}
bool Output(uint32_t container, GuestTexture *&image) {
  if (!Words(container, 8))
    return false;
  const auto address = bd::mem::load<uint32_t>(container + 4);
  image = address ? Texture(address) : nullptr;
  return !address || image;
}
// Reuse the original caller frame for remaining state/parameter ABI adapters.
// This is stack ABI storage, not rendering ownership or an engine allocation.
struct CallFrame {
  PPCContext &ctx;
  uint64_t saved;
  explicit CallFrame(PPCContext &context) : ctx(context), saved(ctx.r1.u64) {
    ctx.r1.u32 -= 256;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved));
    ctx.fpscr.disableFlushMode();
  }
  ~CallFrame() {
    ctx.r1.u64 = saved;
    ctx.fpscr.disableFlushMode();
  }
};
void SetState(PPCContext &ctx, uint8_t *base, uint32_t offset, uint32_t value) {
  ctx.r3.u64 = offset;
  ctx.r4.u64 = value;
  bdSetRenderState(ctx, base); // existing native raster/alpha/blend producers
  stats.state308_calls += offset == 308;
}
void Parameters(PPCContext &ctx, uint8_t *base, uint32_t source) {
  ctx.r3.s64 = int32_t(source);
  sub_821764F8(ctx, base); // complete native producer; engine descriptors remain
  ++stats.parameters;
}
bool Begin(PPCContext &ctx, uint8_t *base, uint32_t source) {
  if (!REXCVAR_GET(bd_native_scene_passes) || !REXCVAR_GET(bd_host_targets) ||
      !Words(source, 40) || !Words(kSettings, 4) || !CanEnterNativePass() ||
      !Words(kDevice, 4) || !Words(kScenePrecisionCache, 4))
    return false;
  const auto precision_device = bd::mem::load<uint32_t>(kDevice);
  if (!Words(precision_device, kScenePrecisionRequest + 4) ||
      bd::mem::load<uint32_t>(precision_device + kScenePrecisionDeviceSlot) != kScenePrecisionSetter)
    return false;
  const auto settings = bd::mem::load<uint32_t>(kSettings);
  const auto camera = bd::mem::load<uint32_t>(source + 12);
  const auto reference = bd::mem::load<uint32_t>(source + 16);
  // An in-flight legacy handle must unwind through its matching compatibility
  // owner. Native attachments never enter the engine's 16-slot allocation list.
  if (!Words(settings, 7140) || !Words(camera, 300) || !Words(reference, 4) ||
      bd::mem::load<uint32_t>(source + 28) || bd::mem::load<uint32_t>(source + 36) ||
      ctx.r1.u32 < 256 || !Words(uint64_t(ctx.r1.u32) - 256, 328) ||
      !Words(kEngine, 54624) || !Words(kCount, 4) || !Words(kPrimary, 420) ||
      !Words(kLast, 420) || !Words(kSecondary, 160) || !Words(kViewCache, 112) ||
      !Words(kFrustum, 160) || !Words(kView, 4) || !Range(kTile, 1) ||
      !Range(kHdr, 1) || !Words(kOne, 4) ||
      bd::mem::load<uint32_t>(kOne) != 0x3F800000)
    return false;
  const auto count = bd::mem::load<int32_t>(kCount);
  if (count > 0 && !Words(kArray, uint64_t(count) * 420))
    return false;
  GuestTexture *output = nullptr, *depth_output = nullptr;
  if (!Output(bd::mem::load<uint32_t>(source + 4), output) || !output ||
      !Output(bd::mem::load<uint32_t>(source + 8), depth_output))
    return false;
  const auto extent = ScaleSceneExtent({output->width, output->height},
                                       Video::BootSupersampling(), REXCVAR_GET(bd_render_scale));
  if (!extent)
    return false;
  const auto samples = uint32_t(Video::CvarMSAASampleCount());
  const auto layers = REXCVAR_GET(bd_stereo_multiview) ? 2u : 1u;
  auto *color = HostTargetAcquireNative(HostTargetClass::SceneColor,
      {extent->width, extent->height, layers, plume::RenderFormat::R16G16B16A16_FLOAT, samples});
  auto *depth = HostTargetAcquireNative(HostTargetClass::SceneDepth,
      {extent->width, extent->height, layers, plume::RenderFormat::D32_FLOAT_S8_UINT, samples});
  if (!color || !depth) {
    if (color) ReleaseResourceAdapter(color->selfVa);
    if (depth) ReleaseResourceAdapter(depth->selfVa);
    return false;
  }
  NativeSceneResolveHandle resolves;
  std::array<NativeImageLease, 2> source_images;
  NativeSceneFramebufferHandle framebuffer;
  plume::RenderTexture *density_map = nullptr;
  {
    // Foveation readiness changes only at frame boundaries. The map store
    // retains these images for device lifetime, independently of binding headers.
    std::lock_guard lock(state().mutex);
    FoveationEnsure(color->width, color->height, color->layers);
    if (FoveationWanted(color->width, color->height, color->layers))
      density_map = FoveationMapFor(color->width, color->height);
  }
  if (samples > 1) {
    SceneResolveSources sources{color->texture, depth->texture,
        HostTargetImageIdentity(color), HostTargetImageIdentity(depth),
        color->width, color->height, color->layers, samples, color->format, depth->format};
    sources.density_map = density_map;
    resolves = AcquireNativeSceneResolves(sources, {color->nativeTarget, depth->nativeTarget});
    if (!resolves) {
      ReleaseResourceAdapter(color->selfVa);
      ReleaseResourceAdapter(depth->selfVa);
      throw std::runtime_error("Native scene attachment resolve capability or residency refused");
    }
  } else {
    framebuffer = AcquireNativeSceneFramebuffer({color->nativeTarget, depth->nativeTarget}, density_map);
    source_images = {NativeImageLease{color->nativeTarget, color->nativeTarget->Sampled()},
        NativeImageLease{depth->nativeTarget, depth->nativeTarget->Sampled()}};
    if (!framebuffer || !source_images[0] || !source_images[1]) {
      ReleaseResourceAdapter(color->selfVa);
      ReleaseResourceAdapter(depth->selfVa);
      throw std::runtime_error("Native single-sample scene framebuffer or ownership refused");
    }
  }
  // From here the native pair owns the scope. Never run the original begin a
  // second time after an observable state/resource publication.
  const bool primary = bd::mem::load<uint32_t>(kView) == 0;
  const auto clear = SceneClearColor(bd::mem::load<uint32_t>(camera + 8), primary);
  const auto resolved = resolves ? resolves->Sampled(1.f) : HostPostInputs{};
  auto commands = NativeSceneCommands::Create({color->nativeTarget, depth->nativeTarget},
      resolves ? resolves->framebuffer.get() : framebuffer->framebuffer.get(),
      {resolved.scene, resolved.depth}, {ArgbToRenderColor(clear), 1.f, 0});
  if (!commands) {
    ReleaseResourceAdapter(color->selfVa);
    ReleaseResourceAdapter(depth->selfVa);
    throw std::runtime_error("Native scene command recipe refused");
  }
  CallFrame frame(ctx);
  bd::mem::store<uint32_t>(source + 20, bd::mem::load<uint32_t>(reference));
  SetState(ctx, base, 60, 1);
  SetState(ctx, base, 56, 0);
  SetState(ctx, base, 72, 6);
  SetState(ctx, base, 76, 7);
  SetState(ctx, base, 40, 1);
  sub_82184A88(ctx, base); // native sampler defaults
  // Getter-only shadows: native allocation does not read an EDRAM/MSAA default.
  bd::mem::store<uint8_t>(kTile, 0);
  if (bd::mem::load<uint32_t>(settings + 7112))
    bd::mem::store<uint32_t>(kStack + 320, 0);
  bd::mem::store<uint32_t>(source + 28, color->selfVa);
  bd::mem::store<uint32_t>(source + 36, depth->selfVa);
  uint32_t result = 0;
  Check(EnterNativePass(color, depth, result), "Native scene could not enter its preflighted pass");
  scenes.push_back({source, color, depth, NativePassDepth(), std::move(resolves),
      std::move(source_images), std::move(framebuffer), std::move(commands)});
  {
    auto &s = state();
    std::lock_guard lock(s.mutex);
    // Temporary frame/getter bridge, not a clear request. Open recording without
    // touching another framebuffer; the native scope holds its clear until bind.
    s.frame_present_committed = false;
    BeginCommandList(s);
    Check(s.command_list_open, "Native scene cannot record commands");
    s.draw_framebuffer_bound = false; // a new scope must consume its own clear
    // Prior compatibility readers have completed before this full scene rewrite.
    HostTargetDropLinks(s, color);
    HostTargetDropLinks(s, depth);
    s.clear_pending = false;
    s.clear_flags = 0;
  }
  // Native storage/clears never switch console surface encodings. Publish only
  // the final getter state for remaining engine clients, without guest dispatch.
  PublishScenePrecisionGetters(precision_device, [](uint32_t address, uint32_t value) {
    bd::mem::store<uint32_t>(address, value);
  });
  ++stats.precision_getters;
  SetState(ctx, base, 212, SceneColorWriteMask(primary));
  ctx.r3.u64 = 0;
  ctx.r4.u64 = camera + 160;
  ctx.r5.u64 = camera + 224;
  bdBuildViewMatrix(ctx, base); // native view/matrix producer
  ctx.fpscr.disableFlushMode();
  const float x = bd::mem::load<float>(camera + 288);
  const float y = bd::mem::load<float>(camera + 292);
  const float z = bd::mem::load<float>(camera + 296);
  bd::mem::store<float>(kEngine + 54608, x);
  bd::mem::store<float>(kEngine + 54620, 1.0f);
  bd::mem::store<uint32_t>(kEngine + 4, 1);
  bd::mem::store<float>(kEngine + 54616, z);
  bd::mem::store<float>(kEngine + 54612, y);
  ctx.r3.u64 = 0;
  sub_82186840(ctx, base); // native complete camera/frustum-cache producer
  ++stats.camera_calls;
  if (bd::mem::load<uint32_t>(settings + 7136) &&
      bd::mem::load<uint32_t>(kViewCache + 56) &&
      !PublishCachedViewFrustum(ctx, 1)) {
    ctx.fpscr.disableFlushMode();
    for (uint32_t i = 0; i < 13; ++i)
      bd::mem::store<float>(kFrustum + i * 4,
                            bd::mem::load<float>(kViewCache + 60 + i * 4));
    ctx.r3.s64 = int32_t(kFrustum);
    ctx.r4.s64 = int32_t(kFrustum + 64);
    ctx.r5.s64 = int32_t(kFrustum + 80);
    ctx.r6.s64 = int32_t(kFrustum + 96);
    ctx.r7.s64 = int32_t(kFrustum + 112);
    ctx.r8.s64 = int32_t(kFrustum + 128);
    ctx.r9.s64 = int32_t(kFrustum + 144);
    sub_821CCF48(ctx, base); // native six-plane publication
  }
  Parameters(ctx, base, kPrimary);
  for (int32_t i = 0; i < count; ++i)
    Parameters(ctx, base, kArray + uint32_t(i) * 420);
  Parameters(ctx, base, kLast);
  ctx.r3.s64 = int32_t(kSecondary);
  sub_82179868(ctx, base); // preserves the original final result
  ++stats.parameters;
  ++stats.begins;
  return true;
}
bool End(PPCContext &ctx, uint32_t source) {
  if (scenes.empty() || !scenes.back().color)
    return false;
  const auto pass = scenes.back();
  Check(pass.source == source && NativePassDepth() == pass.nesting,
        "Native scene end does not match the active scene scope");
  Check(Words(source, 40) &&
        bd::mem::load<uint32_t>(source + 28) == pass.color->selfVa &&
        bd::mem::load<uint32_t>(source + 36) == pass.depth->selfVa &&
        state().render_target == pass.color && state().depth_stencil == pass.depth,
        "Native scene attachment getter or live binding changed");
  GuestTexture *color_output = nullptr, *depth_output = nullptr;
  Check(Output(bd::mem::load<uint32_t>(source + 4), color_output) &&
        Output(bd::mem::load<uint32_t>(source + 8), depth_output),
        "Native scene output adapter no longer names a live texture");
  // An empty pass still publishes its clear, never the persistent image's old
  // contents. Binding consumes the pair's held clears before any output read.
  if (pass.commands->ClearPending() || pass.color->hostClearFlags || pass.depth->hostClearFlags) {
    Check(Video::BindDrawFramebuffer(), "Native scene clear could not bind its attachments");
    ++stats.empty_clears;
  }
  // Complete native resolves before publishing either image to external readers.
  // No redundant shader depth resolve is needed for matching native depth getters.
  {
    auto &s = state();
    std::lock_guard lock(s.mutex);
    DrawQueueFlush(s.command_list);
    if (pass.resolves) FinishNativeSceneResolves(s, *pass.resolves);
    else {
      // Flush the pass (including zero-draw clears), then expose the actual
      // single-sample images. There is no copy, getter import or resolve here.
      s.command_list->setFramebuffer(nullptr);
      std::array<plume::RenderTextureBarrier, 2> barriers;
      uint32_t count = 0;
      for (const auto &lease : pass.source_images) {
        const auto &image = lease.image;
        if (*image.layout != plume::RenderTextureLayout::SHADER_READ) {
          barriers[count++] = {image.texture, plume::RenderTextureLayout::SHADER_READ};
          *image.layout = plume::RenderTextureLayout::SHADER_READ;
        }
      }
      if (count) {
        s.command_list->barriers(plume::RenderBarrierStage::GRAPHICS, barriers.data(), count);
        NoteBarrierCall(count, BarrierSite::Resolve);
      }
    }
    s.plume_framebuffer_bound = false;
    s.draw_framebuffer_bound = false;
    s.bound_fb_rt = s.bound_fb_ds = nullptr;
  }
  const float exposure = SceneOutputExposure(bd::mem::load<uint8_t>(kHdr) != 0);
  const HostPostInputs native_inputs = pass.resolves ? pass.resolves->Sampled(exposure) :
      HostPostInputs{pass.source_images[0].image, pass.source_images[1].image, exposure};
  const bool complete_scene = current_result && Words(kPhase, 4) &&
      bd::mem::load<int32_t>(kPhase) == 3;
  // A final native result retains its exact source until post either publishes
  // a replacement or explicitly recovers this colour. Other views still publish
  // immediately. Depth's remaining getter readers borrow the native image below.
  const bool deferred_color = complete_scene &&
      current_result->Complete(bd::mem::load<uint32_t>(source + 4),
          bd::mem::load<uint32_t>(source + 8), native_inputs,
          pass.color, pass.depth, color_output, depth_output, pass.resolves, true);
  stats.deferred_color += deferred_color;
  SceneImage sampled_color, sampled_depth;
  if (depth_output) {
    const NativeImageLease depth_image = pass.resolves ?
        NativeImageLease{pass.resolves, pass.resolves->Sampled(1.f).depth} : pass.source_images[1];
    if (Video::CanPublishNativeImage(depth_image, depth_output)) {
      Check(Video::PublishNativeImage(depth_image, depth_output),
            "Native depth image lost its preflighted getter");
      sampled_depth = {depth_output, 1.f};
      ++stats.native_depth_publications;
    } else {
      // Scaled/other-format adapters remain counted until their own
      // image owners and consumers are migrated; never silently drop depth.
      Check(Video::PublishSceneOutput(pass.depth, depth_output, 1.0f, true, &sampled_depth),
            "Compatibility scene depth publication failed");
      ++stats.compatibility_depth_publications;
    }
    ++stats.outputs;
  } else ++stats.null_outputs;
  if (color_output && !deferred_color) {
    Check(Video::PublishSceneOutput(pass.color, color_output, exposure, true, &sampled_color),
          "Native scene colour publication failed");
    ++stats.outputs;
  } else if (!color_output) ++stats.null_outputs;
  uint32_t result = 0;
  Check(LeaveNativePass(result), "Native scene could not restore its previous pass");
  // Final phase 3 ends through this vtable entry before the caller's focus and
  // post tail. Publish exact images while the native attachments are still held.
  if (complete_scene && !deferred_color)
    current_result->Complete(bd::mem::load<uint32_t>(source + 4),
        bd::mem::load<uint32_t>(source + 8),
        native_inputs,
        pass.color, pass.depth, color_output, depth_output, pass.resolves);
  ReleaseResourceAdapter(pass.color->selfVa);
  bd::mem::store<uint32_t>(source + 28, 0);
  ctx.r3.u64 = ReleaseResourceAdapter(pass.depth->selfVa);
  bd::mem::store<uint32_t>(source + 36, 0);
  ctx.fpscr.disableFlushMode();
  scenes.pop_back();
  ++stats.ends;
  return true;
}
} // namespace

const NativeSceneResolves *ActiveNativeSceneResolves(plume::RenderTexture *color,
                                                     plume::RenderTexture *depth) {
  if (scenes.empty()) return nullptr;
  const auto &pass = scenes.back();
  return pass.resolves && pass.resolves->sources.color == color && pass.resolves->sources.depth == depth
      ? pass.resolves.get() : nullptr;
}

plume::RenderFramebuffer *ActiveNativeSceneFramebuffer(plume::RenderTexture *color,
                                                      plume::RenderTexture *depth) {
  if (const auto *resolved = ActiveNativeSceneResolves(color, depth))
    return resolved->framebuffer.get();
  if (scenes.empty()) return nullptr;
  const auto &pass = scenes.back();
  return pass.framebuffer && pass.framebuffer->Matches(color, depth)
      ? pass.framebuffer->framebuffer.get() : nullptr;
}

NativeSceneCommands *ActiveNativeSceneCommands(plume::RenderTexture *color, plume::RenderTexture *depth) {
  if (scenes.empty() || !scenes.back().commands) return nullptr;
  auto &commands = *scenes.back().commands;
  return commands.Matches(color, depth) ? &commands : nullptr;
}
void BindNativeSceneCommands(VideoState &s, NativeSceneCommands &commands) {
  const auto barriers = commands.Bind(*s.command_list);
  if (barriers) NoteBarrierCall(barriers, BarrierSite::DrawFb);
  ++stats.command_binds;
}
void ApplyNativeSceneClear(VideoState &s, NativeSceneCommands &commands) {
  stats.native_clears += commands.ApplyClear(*s.command_list);
  // Other, not-yet-converted clear producers remain counted at this boundary.
  stats.compatibility_clears += s.clear_pending ||
      (s.render_target && s.render_target->hostClearFlags) ||
      (s.depth_stencil && s.depth_stencil->hostClearFlags);
}

CompletedSceneImages::CompletedSceneImages(CompletedSceneImages &&other) noexcept {
  *this = std::move(other);
}
CompletedSceneImages &CompletedSceneImages::operator=(CompletedSceneImages &&other) noexcept {
  if (this != &other) {
    Reset();
    inputs = std::exchange(other.inputs, {});
    resolves = std::move(other.resolves);
    output = std::exchange(other.output, nullptr);
    pending_scene_color = std::exchange(other.pending_scene_color, false);
    source_pins = std::exchange(other.source_pins, {});
    output_references = std::exchange(other.output_references, {});
  }
  return *this;
}
CompletedSceneImages::~CompletedSceneImages() { Reset(); }
void CompletedSceneImages::PublishPendingColor() {
  if (!pending_scene_color) return;
  Check(Video::PublishSceneOutput(source_pins[0], output, inputs.exposure),
        "Unconsumed native scene colour publication failed");
  pending_scene_color = false;
  ++stats.recovered_color;
}
void CompletedSceneImages::Reset() {
  for (auto &address : output_references)
    if (address) ReleaseResourceAdapter(std::exchange(address, 0));
  for (auto &image : source_pins)
    if (image) HostTargetUnpin(std::exchange(image, nullptr));
  inputs = {};
  resolves.reset();
  output = nullptr;
  pending_scene_color = false;
}
NativeSceneResultScope::NativeSceneResultScope(uint32_t view)
    : previous_(current_result), view_(view), frame_(FrameStatFrameCount()) {
  // Only boundary association tokens, never image handles or sampled identities.
  if (Words(view, 8)) {
    color_getter_ = bd::mem::load<uint32_t>(view);
    depth_getter_ = bd::mem::load<uint32_t>(view + 4);
  }
  current_result = this;
}
NativeSceneResultScope::~NativeSceneResultScope() {
  // Normal view return calls Clear explicitly, so skipped post still publishes.
  // An exception releases owners here without recording GPU work while unwinding.
  current_result = previous_;
}
void NativeSceneResultScope::Clear() {
  if (auto pending = result_.Take(frame_)) pending->PublishPendingColor();
}
bool NativeSceneResultScope::Complete(uint32_t color_getter, uint32_t depth_getter,
    const HostPostInputs &inputs, GuestTexture *color, GuestTexture *depth,
    GuestTexture *output, GuestTexture *depth_output, NativeSceneResolveHandle resolves,
    bool pending_scene_color) {
  Clear();
  if (frame_ != FrameStatFrameCount() || !color_getter_ || !depth_getter_ ||
      color_getter != color_getter_ || depth_getter != depth_getter_ ||
      !inputs.scene || !inputs.depth || !output || !depth_output)
    return false;
  CompletedSceneImages result;
  result.inputs = inputs;
  result.resolves = std::move(resolves);
  result.output = output;
  result.pending_scene_color = pending_scene_color;
  const std::array sources{color, depth};
  const std::array outputs{output, depth_output};
  for (size_t i = 0; i < sources.size(); ++i) {
    Check(HostTargetPin(sources[i]), "Completed native scene could not pin its source");
    result.source_pins[i] = sources[i];
    Check(RetainResourceAdapter(outputs[i]->selfVa) != 0,
          "Completed native scene lost its output adapter");
    result.output_references[i] = outputs[i]->selfVa;
  }
  stats.materialized_color += inputs.scene.texture != color->texture;
  stats.materialized_depth += inputs.depth.texture != depth->texture;
  stats.native_resolve_results += bool(result.resolves);
  result_.Complete(frame_, std::move(result));
  ++stats.completed;
  return true;
}
std::optional<CompletedSceneImages> NativeSceneResultScope::Take(uint32_t view) {
  if (view != view_) return {}; // a nested/foreign caller cannot consume this view
  if (frame_ != FrameStatFrameCount()) {
    Clear(); // retain getter correctness even when a stale result cannot feed post
    return {};
  }
  auto result = result_.Take(FrameStatFrameCount());
  stats.consumed += result.has_value();
  return result;
}
std::optional<CompletedSceneImages> TakeCompletedSceneImages(uint32_t view) {
  return current_result ? current_result->Take(view) : std::nullopt;
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(sub_82186BA0) {
  using namespace bd::gpu::scene;
  if (current_result) current_result->Clear(); // before any native/legacy reuse
  const auto source = ctx.r3.u32;
  if (!Begin(ctx, base, source)) {
    ++stats.compatibility_begin;
    stats.refused += REXCVAR_GET(bd_native_scene_passes);
    __imp__sub_82186BA0(ctx, base);
    scenes.push_back({source}); // match the original end even if the cvar changes
  }
  Report();
}
REX_HOOK_RAW(sub_82187010) {
  using namespace bd::gpu::scene;
  const auto source = ctx.r3.u32;
  if (!End(ctx, source)) {
    Check(scenes.empty() || scenes.back().source == source,
          "Compatibility scene end does not match its begin");
    ++stats.compatibility_end;
    __imp__sub_82187010(ctx, base);
    if (!scenes.empty()) scenes.pop_back();
  }
  Report();
}
