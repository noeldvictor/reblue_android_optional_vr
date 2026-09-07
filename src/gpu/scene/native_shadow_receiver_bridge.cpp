/**
 * @brief Host receiver setup and an owned image/camera/colour publication.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_shadow_receiver_bridge.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/host_parameter_bridge.h"
#include "gpu/scene/guest_scene.h"
#include "gpu/resource_bridge.h"
#include "gpu/native_texture_mirror.h"
#include "gpu/resources.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "core/memory_helpers.h"
#include "core/logging.h"
#include "core/settings.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <cmath>
#include <bit>
#include <stdexcept>

REXCVAR_DEFINE_BOOL(bd_native_shadow_receiver, true, kCvarGroup,
    "Host receiver setup and retained primary shadow inputs; legacy shader publication remains for unconverted draws.");
REXCVAR_DECLARE(bool, bd_native_rigid_hard_off);
REX_EXTERN(__imp__sub_82176708);
namespace bd::gpu::scene {
namespace {
constexpr uint32_t kPrimary = (uint32_t(-32035) << 16) + 24832;
constexpr uint32_t kMode = (uint32_t(-32137) << 16) + 16476;
constexpr uint32_t kLegacyLighting = (uint32_t(-32034) << 16) - 32552;
thread_local NativeReceiverPublication receiver;
struct Stats {
  uint64_t native = 0, ignored = 0, original = 0, refused = 0;
  uint64_t bindings = 0, parameters = 0, published = 0, reads = 0, missing = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame-stats.frame < 300) return;
  BD_INFO("[native-shadow-receiver] frame {} native {} ignored {} original {} refused {}; compatibility bindings {} parameters {}; owned packets {} reads {} missing {}; authored fields and legacy shader publication remain",
      frame,stats.native,stats.ignored,stats.original,stats.refused,stats.bindings,stats.parameters,stats.published,stats.reads,stats.missing);
  stats.reported = true; stats.frame = frame;
}
void Require(bool value, const char *reason) {
  if (value) return;
  ++stats.refused;
  BD_ERROR("[native-shadow-receiver] refused: {}; no original fallback after native setup",reason);
  throw std::runtime_error(reason);
}
std::optional<uint32_t> Word(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX-3) return {};
  const auto *value = bd::mem::try_at<const be_u32>(uint32_t(address));
  return value ? std::optional(uint32_t(*value)) : std::nullopt;
}
uint32_t ReadWord(uint64_t address) {
  const auto value = Word(address);
  Require(value.has_value(),"receiver source changed after preflight");
  return *value;
}
LightingVector ReadColour(uint32_t source) {
  LightingVector colour;
  for (uint32_t n=0;n<4;++n) colour[n] = std::bit_cast<float>(ReadWord(uint64_t(source)+52+n*4));
  return colour;
}
void Publish(uint32_t source, uint32_t visual, std::optional<LightingVector> colour = {}) {
  receiver.Reset();
  const auto mode = Word(kMode), tech = Word(uint64_t(visual)+3000), view = Word(kRenderViewIdVa);
  const auto output = Word(uint64_t(source)+12);
  const auto shadow = FindCompletedNativePrimaryShadow();
  if (source != kPrimary || !visual || !mode || !tech || !view || !output || !shadow ||
      !ImportReceiverParticipation(*tech,*mode)) return;
  // A later resource replacement must not attach this colour to a different
  // image. Resolve the temporary header only here, never in native submission.
  const auto *image = ResolveGuestTexture(*output);
  if (!image || image->nativeImage.owner != shadow->image ||
      image->texture != shadow->image->image.get()) return;
  receiver.Publish({shadow->image,shadow->camera.world_to_clip,colour ? *colour : ReadColour(source)},visual,FrameStatFrameCount(),*view);
  if (receiver.Read(visual,FrameStatFrameCount(),*view)) ++stats.published;
}
struct Adapter {
  uint32_t source, visual, stack;
  GuestTexture *image = nullptr;
  void Reset() { receiver.Reset(); }
  void Preflight() {
    Require(stack >= 256 && !(stack & 15),"invalid receiver stack boundary");
    const auto output = Word(uint64_t(source)+12), descriptor = Word(uint64_t(source)+356);
    Require(output && descriptor,"receiver image/parameter source unavailable");
    Require(uint64_t(source)+360 <= stack-256 || source >= stack,"receiver source aliases legacy stack writes");
    for (uint32_t n=0;n<4;++n)
      Require(Word(uint64_t(source)+52+n*4).has_value() && Word(kLegacyLighting+192+n*4).has_value(),"receiver colour storage unavailable");
    Require(Word(kLegacyLighting+376).has_value() && Word(kLegacyLighting+408).has_value(),"receiver compatibility staging unavailable");
    // Resolve can wait for IO; never hold the video mutex around it. Preserve
    // the existing unsupported-image marker contract for compatibility draws.
    image = ResolveGuestTexture(*output);
    if (!image && *output) image = GetOrCreateDebugTexture();
    Require(CanFlushHostParameterDescriptor(*descriptor,stack-96),"receiver parameter descriptor cannot execute natively");
  }
  void BindCompatibilityImage() { Video::SetTexture(6,image); ++stats.bindings; }
  void FlushCompatibilityParameters() {
    Require(FlushHostParameterDescriptor(ReadWord(uint64_t(source)+356),stack-96),"receiver parameter source changed or native publication disabled");
    ++stats.parameters;
  }
  LightingVector ReadColour() { return bd::gpu::scene::ReadColour(source); }
  void PublishCompatibilityColour(const LightingVector &colour) {
    // Remaining engine getters/translated draws. The native packet uses the
    // local value, never this staging image or its dirty/revision fields.
    bd::mem::store<float>(kLegacyLighting+192,colour[0]);
    bd::mem::store<float>(kLegacyLighting+196,colour[1]);
    bd::mem::store<uint32_t>(kLegacyLighting+376,1);
    bd::mem::store<float>(kLegacyLighting+200,colour[2]);
    bd::mem::store<uint32_t>(kLegacyLighting+408,ReadWord(kLegacyLighting+408)+1);
    bd::mem::store<float>(kLegacyLighting+204,colour[3]);
  }
  void PublishNative(const LightingVector &colour) { Publish(source,visual,colour); }
};
}
std::optional<NativePrimaryReceiver> FindNativePrimaryReceiver(uint32_t visual, uint32_t view) {
  auto result = receiver.Read(visual,FrameStatFrameCount(),view);
  ++(result ? stats.reads : stats.missing); Report();
  return result;
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(sub_82176708) {
  using namespace bd::gpu::scene;
  const auto source = ctx.r3.u32, visual = ctx.r4.u32;
  if (!REXCVAR_GET(bd_native_shadow_receiver)) {
    Require(!REXCVAR_GET(bd_native_rigid_hard_off),"hard-off requires native receiver setup");
    ++stats.original;
    __imp__sub_82176708(ctx,base);
    Publish(source,visual); Report(); return;
  }
  const auto technique = ReadWord(uint64_t(visual)+3000);
  // Technique14 exits without reading phase or touching bindings/descriptors.
  const bool enabled = technique != 14 && ImportReceiverParticipation(technique,ReadWord(kMode));
  Adapter adapter{source,visual,ctx.r1.u32};
  RunNativeReceiverSetup(enabled,adapter);
  ++stats.native; stats.ignored += !enabled;
  ctx.r3.u64 = 1;
  Report();
}
