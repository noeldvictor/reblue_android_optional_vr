/**
 * @file    native_view_schedule_bridge.cpp
 * @brief   Whole-view host scheduling with counted remaining engine boundaries.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_view_schedule_bridge.h"
#include "gpu/scene/native_view_schedule.h"
#include "gpu/scene/view_schedule_geometry.h"
#include "gpu/scene/native_view.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "gpu/frame_stats.h"
#include "gpu/resource_bridge.h"
#include <array>
#include <cstring>
#include <initializer_list>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/xthread.h>
#include <stdexcept>

#pragma clang fp contract(off)

REX_EXTERN(sub_82173DF8);
REX_EXTERN(sub_821824A0);
REX_EXTERN(bdShaderSystemBeginFrame);
REX_EXTERN(bdShaderSystemFlush);
REX_EXTERN(bdSceneNodeSubmitRenderList);
REX_EXTERN(bdSceneSubmitRenderList);
REX_EXTERN(bdRenderTargetRelease);
REX_EXTERN(bdBuildMirrorViewProjection);
REX_EXTERN(bdBuildViewMatrix);
REX_EXTERN(bdCameraRenderMotionBlur);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(sub_826BF5B8);
REX_EXTERN(sub_8221C9A0);
REX_EXTERN(bdEffectSlotArrayApply);
REX_EXTERN(sub_826BEF30);
bool bdNativeScenePostHook(PPCRegister &view);
REXCVAR_DECLARE(bool, bd_native_scene_passes);
REXCVAR_DECLARE(bool, bd_native_passes);

namespace bd::gpu::scene {
namespace {
// Temporary authored-data/getter boundary. The native ordering and geometry
// contain no addresses, console state packets, GPR allocations or tile rules.
constexpr uint32_t kSettings = (uint32_t(-32035) << 16) - 26552;
constexpr uint32_t kEngine = (uint32_t(-32034) << 16) - 19936;
constexpr uint32_t kPhase = (uint32_t(-32137) << 16) + 16476;
constexpr uint32_t kEffects = (uint32_t(-32136) << 16) + 14936;
constexpr uint32_t kEffectList = (uint32_t(-32136) << 16) + 14888;
constexpr uint32_t kIndexed = (uint32_t(-32035) << 16) + 25248;
constexpr uint32_t kIndexedCount = (uint32_t(-32137) << 16) + 16748;
constexpr uint32_t kSelectedIndex = (uint32_t(-32137) << 16) + 16740;
constexpr uint32_t kShadowType = (uint32_t(-32137) << 16) + 16744;
constexpr uint32_t kPrimary = (uint32_t(-32035) << 16) + 24832;
constexpr uint32_t kCubeShadow = (uint32_t(-32137) << 16) + 28048;
constexpr uint32_t kAuxiliary = (uint32_t(-32035) << 16) + 28608;
constexpr uint32_t kPrimaryReflection = (uint32_t(-32035) << 16) + 29040;
constexpr uint32_t kReflections = (uint32_t(-32035) << 16) + 29320;
constexpr uint32_t kEnvironment = (uint32_t(-32137) << 16) + 28268;
constexpr uint32_t kPassMode = (uint32_t(-32036) << 16) - 5536;
constexpr uint32_t kShadowMode = (uint32_t(-32035) << 16) - 26168;
constexpr uint32_t kThread = (uint32_t(-32035) << 16) - 26664;
constexpr uint32_t kClip = (uint32_t(-32034) << 16) - 32552;
constexpr uint32_t kCameraSettings = (uint32_t(-32034) << 16) - 22320;
constexpr uint32_t kFocus = (uint32_t(-32034) << 16) - 31876 + 4;
constexpr uint32_t kFloats = (uint32_t(-32250) << 16) - 30428;
constexpr uint32_t kSmall = (uint32_t(-32250) << 16) + 8116;
constexpr uint32_t kZeroMin = (uint32_t(-32247) << 16) - 5560;
constexpr uint32_t kFocusLimit = (uint32_t(-32247) << 16) - 3068;
constexpr uint32_t kBaseTable = (uint32_t(-32249) << 16) - 14232;
constexpr uint32_t kTargetTable = (uint32_t(-32249) << 16) - 16108;
constexpr std::array<uint32_t, 9> kPassTables{
    (uint32_t(-32249) << 16) - 14120, // indexed auxiliary
    (uint32_t(-32249) << 16) - 14176, // primary shadow
    (uint32_t(-32249) << 16) - 14148, // cube shadow
    (uint32_t(-32249) << 16) - 14092, // auxiliary
    (uint32_t(-32249) << 16) - 14036, // shadow volume
    (uint32_t(-32249) << 16) - 14064, // planar reflection
    (uint32_t(-32249) << 16) - 14008, // environment cube
    (uint32_t(-32249) << 16) - 13980, // additional scene
    (uint32_t(-32249) << 16) - 14204  // main scene
};
struct Stats {
  uint64_t views = 0, compatibility = 0, refused = 0, faults = 0;
  uint64_t pass_adapters = 0, list_adapters = 0, preparation_adapters = 0;
  uint64_t reflection_tests = 0, reflection_rejected = 0, post_adapters = 0;
  uint64_t legacy_exports = 0;
  uint32_t frame = 0;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (frame - stats.frame < 300) return;
  BD_INFO("[native-view-schedule] views {} compatibility {} refused {} faults {}; "
          "pass adapters {} list adapters {} preparation adapters {}; "
          "reflection tests {} rejected {} legacy post scopes {} export refusals {}; "
          "native order/geometry, imported authored data, descriptors, registry and remaining callbacks",
          stats.views, stats.compatibility, stats.refused, stats.faults,
          stats.pass_adapters, stats.list_adapters, stats.preparation_adapters,
          stats.reflection_tests, stats.reflection_rejected, stats.post_adapters, stats.legacy_exports);
  stats.frame = frame;
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address + bytes - 1 > UINT32_MAX ||
      !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096;
       page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) { return !(address & 3) && Range(address, bytes); }
void Check(bool valid) {
  if (!valid) {
    ++stats.faults;
    throw std::runtime_error("Native view schedule lost a validated engine import");
  }
}
template <class T> T Read(uint32_t address) {
  Check(Range(address, sizeof(T)) && (sizeof(T) < 4 || !(address & 3)));
  return bd::mem::load<T>(address);
}
template <class T> void Write(uint32_t address, T value) {
  Check(Range(address, sizeof(T)) && (sizeof(T) < 4 || !(address & 3)));
  bd::mem::store<T>(address, value);
}
template <size_t N> std::array<float, N> Floats(uint32_t address) {
  std::array<float, N> result;
  for (uint32_t i = 0; i < N; ++i) result[i] = Read<float>(address + i * 4);
  return result;
}
bool Prepare(PPCContext &ctx) {
  if (!Words(ctx.r3.u32, 16) || !Words(kSettings, 4) ||
      !rex::system::XThread::GetCurrentThread() || ctx.r1.u32 < 4096 ||
      (ctx.r1.u32 & 15) || !Words(uint64_t(ctx.r1.u32) - 4096, 4192)) return false;
  const auto camera = bd::mem::load<uint32_t>(ctx.r3.u32 + 8);
  if (!Words(camera, 404) || !Words(bd::mem::load<uint32_t>(camera), 1660) ||
      !Words(bd::mem::load<uint32_t>(kSettings), 7088)) return false;
  for (auto [address, bytes] : std::array<std::pair<uint32_t, uint32_t>, 22>{{
      {kEffects, 17}, {kEngine, 54624}, {kIndexedCount, 4}, {kSelectedIndex, 8},
      {kPrimary, 420}, {kCubeShadow, 220}, {kAuxiliary, 432},
      {kPrimaryReflection, 280}, {kReflections, 2800}, {kEnvironment, 184},
      {kClip, 412}, {kCameraSettings, 48}, {kFocus, 24},
      {kPassMode, 2}, {kShadowMode, 4}, {kThread, 4}, {kFloats, 4},
      {kFloats - 14200, 4}, {kFloats - 17272, 4}, {kSmall, 4},
      {kZeroMin, 4}, {kFocusLimit, 4}}})
    if (!Range(address, bytes)) return false;
  // This is an obsolete Xbox debug DDS export, not a rendered game pass. Keep
  // it an explicit pre-effect compatibility case until native capture replaces it.
  if (bd::mem::load<uint8_t>(kEnvironment + 180)) {
    ++stats.legacy_exports;
    return false;
  }
  auto *dispatcher = REX_KERNEL_STATE()->function_dispatcher();
  for (auto table : kPassTables) {
    if (!Words(table, 24)) return false;
    for (uint32_t slot : {12u, 16u, 20u})
      if (!dispatcher->GetFunction(bd::mem::load<uint32_t>(table + slot))) return false;
  }
  return true;
}
struct ViewAdapter {
  PPCContext &ctx;
  uint8_t *base;
  const uint32_t view, authored;
  const uint64_t saved_stack;
  std::array<uint8_t, 4> effect_flags;
  bool restored = false;
  explicit ViewAdapter(PPCContext &context, uint8_t *memory)
      : ctx(context), base(memory), view(ctx.r3.u32),
        authored(Read<uint32_t>(Read<uint32_t>(view + 8))), saved_stack(ctx.r1.u64) {
    ctx.r1.u32 -= 2048;
    Write<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
    effect_flags = {Read<uint8_t>(kEffects + 13), Read<uint8_t>(kEffects + 15),
                    Read<uint8_t>(kEffects + 14), Read<uint8_t>(kEffects + 16)};
  }
  ~ViewAdapter() { ctx.r1.u64 = saved_stack; ctx.fpscr.disableFlushMode(); }
  uint32_t Call(PPCFunc *fn, std::initializer_list<uint32_t> arguments = {}) {
    const std::array<PPCRegister *, 8> registers{
        &ctx.r3, &ctx.r4, &ctx.r5, &ctx.r6, &ctx.r7, &ctx.r8, &ctx.r9, &ctx.r10};
    size_t i = 0;
    Check(arguments.size() <= registers.size());
    for (auto value : arguments) registers[i++]->u64 = value;
    fn(ctx, base);
    ctx.fpscr.disableFlushMode();
    return ctx.r3.u32;
  }
  uint32_t Virtual(uint32_t object, uint32_t slot, uint32_t argument = 0) {
    const auto address = Read<uint32_t>(Read<uint32_t>(object) + slot);
    auto *fn = REX_KERNEL_STATE()->function_dispatcher()->GetFunction(address);
    Check(fn != nullptr);
    ctx.last_indirect_target = address;
    return Call(fn, {object, argument});
  }
  uint32_t Camera() { return Read<uint32_t>(view + 8); }
  bool Setting(uint32_t offset) { return Read<uint32_t>(Read<uint32_t>(kSettings) + offset) != 0; }
  bool Authored(uint32_t offset) { return Read<uint8_t>(authored + offset) != 0; }
  uint32_t Bank() { return rex::system::XThread::GetCurrentThreadId() == Read<uint32_t>(kThread) ? 0 : 1; }
  ScheduledViewRay Ray() {
    const auto camera = Camera();
    return BuildScheduledViewRay(Floats<3>(camera + 288), Floats<3>(camera + 300),
                                 Read<float>(kZeroMin), Read<float>(kSmall));
  }
  void PrepareView() {
    const auto camera = Camera();
    Check(Words(camera + 288, 24));
    if (!std::memcmp(bd::mem::at<uint8_t>(camera + 288), bd::mem::at<uint8_t>(camera + 300), 12))
      Write<float>(camera + 304, Read<float>(camera + 304) + Read<float>(kFloats - 14200));
    const std::array<uint32_t, 4> selectors{1, 3, 2, 4};
    const std::array<uint32_t, 4> settings{7016, 7024, 7020, 7064};
    const std::array<uint32_t, 4> authored_offsets{1652, 1653, 1654, 1658};
    for (size_t i = 0; i < 4; ++i)
      if (Setting(settings[i])) Call(sub_82173DF8, {kEffects, Read<uint8_t>(authored + authored_offsets[i]), selectors[i]});
    Call(sub_821824A0); // scene preparation/remaining object submission, not native registry ownership
    ++stats.preparation_adapters;
  }
  void RestoreView() {
    const std::array<uint32_t, 4> selectors{1, 3, 2, 4};
    for (size_t i = 0; i < 4; ++i) Call(sub_82173DF8, {kEffects, effect_flags[i], selectors[i]});
    restored = true;
  }
  struct Pass {
    ViewAdapter &owner;
    const uint32_t record;
    Pass(ViewAdapter &adapter, uint32_t kind, std::optional<uint32_t> mode, uint32_t extra = 0)
        : owner(adapter), record(adapter.ctx.r1.u32 + 128) {
      if (mode) Write<uint32_t>(kEngine + 20, *mode);
      Write<uint32_t>(record, kPassTables[kind]);
      for (uint32_t i = 0; i < 4; ++i) Write<uint32_t>(record + 4 + i * 4, Read<uint32_t>(owner.view + i * 4));
      Write<uint32_t>(record + 20, 0);
      Write<uint32_t>(record + 24, kTargetTable);
      Write<uint32_t>(record + 28, 0);
      Write<uint32_t>(record + 32, kTargetTable);
      Write<uint32_t>(record + 36, 0);
      Write<uint32_t>(record + 40, extra);
    }
    void Begin(uint32_t phase, uint32_t extra) {
      Write<uint32_t>(record + 40, extra);
      if (Read<uint32_t>(kEngine + 24) != 1) Write<uint32_t>(kEngine + 24, 1);
      owner.Call(bdShaderSystemBeginFrame, {record, phase});
      ++stats.pass_adapters;
    }
    void Draw(uint32_t list_offset, bool nodes = false) {
      const auto list = Read<uint32_t>(owner.Camera() + list_offset);
      if (nodes) owner.Call(bdSceneNodeSubmitRenderList, {record, list});
      else {
        owner.Call(bdSceneSubmitRenderList, {record + 20, Read<uint32_t>(Read<uint32_t>(record + 12) + 52), list});
        owner.Virtual(record, 16, list);
      }
      ++stats.list_adapters;
    }
    void End() { owner.Call(bdShaderSystemFlush, {record}); }
    void Release() {
      Write<uint32_t>(record, kBaseTable);
      for (uint32_t slot : {32u, 24u}) {
        Write<uint32_t>(record + slot, kTargetTable);
        const auto target = Read<uint32_t>(record + slot + 4);
        if (target) {
          owner.Call(bdRenderTargetRelease, {target});
          Write<uint32_t>(record + slot + 4, 0);
        }
      }
    }
  };
  struct SavedMode {
    const uint8_t cull = Read<uint8_t>(kPassMode + 1);
    const uint32_t shadow = Read<uint32_t>(kShadowMode);
    void Set(uint8_t value) { Write<uint8_t>(kPassMode + 1, value); Write<uint32_t>(kShadowMode, 1); }
    void Restore() { Write<uint8_t>(kPassMode + 1, cull); Write<uint32_t>(kShadowMode, shadow); }
  };
  int32_t IndexedViewCount() { return Read<int32_t>(kIndexedCount); }
  bool IndexedViewEnabled(uint32_t i) {
    const uint64_t record = uint64_t(kIndexed) + uint64_t(i) * 420;
    Check(Words(record, 420));
    return Read<uint8_t>(uint32_t(record) + 8) != 0;
  }
  void RenderIndexedView(uint32_t i) {
    Write<uint32_t>(kSelectedIndex, i);
    Pass pass(*this, 0, 4, i);
    pass.Begin(7, i);
    if (Setting(7036)) pass.Draw(396, true);
    pass.End(); pass.Release();
  }
  void SelectPrimaryView() { Write<uint32_t>(kSelectedIndex, uint32_t(-99)); }
  bool SunShadowRequested() { return Read<uint8_t>(kPrimary + 8) && Authored(1652) && Read<uint32_t>(kShadowType) == 1; }
  bool CubeShadowRequested() { return Read<uint8_t>(kCubeShadow + 8) && Authored(1652) && Read<uint32_t>(kShadowType) == 2; }
  bool AuxiliaryRequested() { return Read<uint8_t>(kAuxiliary + 8) != 0; }
  bool ShadowVolumeRequested() { return Setting(7024) && Authored(1653); }
  bool ReflectionsRequested() { return Authored(1654); }
  bool EnvironmentRequested() { return Setting(7028) && Authored(1655); }
  bool AdditionalSceneRequested() { return Setting(7032) && Authored(1656); }
  bool PostRequested() { return Setting(7072); }
  void RenderSunShadow() {
    SavedMode mode;
    Pass pass(*this, 1, 2);
    pass.Begin(1, 0);
    if (Setting(7016)) pass.Draw(384, true);
    pass.End(); mode.Restore(); pass.Release();
  }
  void RenderCubeShadow() {
    SavedMode mode;
    Pass pass(*this, 2, 5);
    for (uint32_t face = 0; face < 6; ++face) {
      pass.Begin(8, face);
      if (Setting(7016)) pass.Draw(384);
      pass.End();
    }
    mode.Restore(); pass.Release();
  }
  void RenderAuxiliary() {
    Pass pass(*this, 3, 0);
    pass.Begin(9, 0);
    if (Setting(7040)) pass.Draw(400);
    pass.End(); pass.Release();
  }
  void RenderShadowVolume() {
    SavedMode mode;
    mode.Set(1);
    Pass pass(*this, 4, 2);
    pass.Begin(4, 0); pass.Draw(392); pass.End();
    mode.Restore(); pass.Release();
  }
  void BuildMirror(uint32_t reflection) {
    const auto camera = Camera();
    Call(bdBuildMirrorViewProjection, {reflection, camera + 288, camera + 300});
  }
  void PublishReflectionClip(uint32_t reflection) {
    if (!Read<uint8_t>(reflection + 8)) return;
    Write<uint32_t>(kClip + 376, 1);
    Write<uint32_t>(kClip + 408, Read<uint32_t>(kClip + 408) + 1);
    const auto plane = Floats<4>(reflection + 188);
    for (uint32_t c = 0; c < 4; ++c) Write<float>(kClip + 208 + c * 4, plane[c]);
  }
  void DrawReflection(uint32_t reflection, bool build_mirror = false) {
    Pass pass(*this, 5, std::nullopt, reflection);
    if (build_mirror) BuildMirror(reflection);
    pass.Begin(0, reflection);
    if (Setting(7020)) { PublishReflectionClip(reflection); pass.Draw(380); }
    pass.End(); pass.Release();
  }
  void RenderReflections() {
    SavedMode mode;
    Write<uint32_t>(kEngine + 20, 3);
    mode.Set(2);
    if (Read<uint8_t>(kPrimaryReflection + 8) && Read<uint32_t>(kPrimaryReflection + 160)) {
      DrawReflection(kPrimaryReflection, true);
    }
    ctx.fpscr.enableFlushMode();
    const auto frustum = BuildFrustumPlanes(BuildViewFrustumShape(Floats<16>(Camera() + 160), Floats<16>(Camera() + 224)));
    ctx.fpscr.disableFlushMode();
    const auto ray = Ray();
    for (uint32_t i = 0; i < 10; ++i) {
      const auto reflection = kReflections + i * 280;
      if (!Read<uint8_t>(reflection + 8)) continue;
      ++stats.reflection_tests;
      if (Read<uint32_t>(reflection + 248 + Bank() * 4)) {
        const auto object = Read<uint32_t>(reflection + 248 + Bank() * 4);
        const auto world = Floats<16>(object + 2388 + Bank() * 64);
        const auto sphere = TransformReflectionSphere(Floats<4>(object + 3424), world);
        ctx.fpscr.enableFlushMode();
        const bool visible = ReflectionSphereVisible(sphere, frustum);
        ctx.fpscr.disableFlushMode();
        if (!visible) { ++stats.reflection_rejected; continue; }
        BuildMirror(reflection);
        ctx.f1.f64 = float(Read<float>(kCameraSettings + 44) * Read<float>(kFloats - 17272));
        Call(sub_826BF5B8); // authored trigonometric threshold, not pass scheduling
        const float threshold = float(ctx.f1.f64) - Read<float>(kSmall);
        const auto plane = Floats<3>(reflection + 188);
        const float facing = (plane[0] * ray.direction[0] + plane[1] * ray.direction[1]) + plane[2] * ray.direction[2];
        if (facing > threshold) { ++stats.reflection_rejected; continue; }
      }
      if (Read<uint8_t>(reflection + 272 + Bank())) DrawReflection(reflection);
    }
    mode.Restore();
  }
  void RenderEnvironment() {
    Pass pass(*this, 6, 0);
    if (Read<uint32_t>(kEnvironment + 168)) {
      const auto target = Floats<3>(Camera() + 300);
      for (uint32_t c = 0; c < 3; ++c) Write<float>(kEnvironment + 148 + c * 4, target[c]);
    }
    for (uint32_t face = 0; face < 6; ++face) {
      pass.Begin(5, face); pass.Draw(376); pass.End();
    }
    pass.Release();
  }
  void RenderAdditionalScene() {
    Pass pass(*this, 7, 0);
    pass.Begin(6, 0); pass.Draw(376); pass.End(); pass.Release();
  }
  void RenderMainScene() {
    if (Setting(7084)) Call(bdSetRenderState, {200, 1});
    Pass pass(*this, 8, Read<uint32_t>(kShadowType) == 2 ? 1 : 0);
    pass.Begin(3, 0); pass.Draw(376);
    Call(bdCameraRenderMotionBlur, {Camera()});
    pass.End();
    if (Setting(7084)) Call(bdSetRenderState, {200, 0});
    pass.Release();
  }
  void DestroyPostContainer(uint32_t container) {
    if (const auto image = Read<uint32_t>(container)) {
      ReleaseResourceAdapter(image);
      Write<uint32_t>(container, 0);
    }
    if (const auto array = Read<uint32_t>(container + 4)) {
      if (Read<uint32_t>(array - 4)) Virtual(array, 0, 3);
      else Call(sub_826BEF30, {array - 4});
      Write<uint32_t>(container + 4, 0);
    }
    Write<uint32_t>(container + 8, 0); Write<uint32_t>(container + 12, 0);
    if (const auto array = Read<uint32_t>(container + 16)) Call(sub_826BEF30, {array});
    Write<uint32_t>(container + 16, 0); Write<uint32_t>(container + 20, 0);
    Write<uint32_t>(container + 24, 0); Write<uint32_t>(container + 28, UINT32_MAX);
  }
  void RenderPost() {
    const auto camera = Camera();
    Call(bdBuildViewMatrix, {0, camera + 160, camera + 224});
    const auto eye = Floats<3>(Camera() + 288);
    for (uint32_t c = 0; c < 3; ++c) Write<float>(kEngine + 54608 + c * 4, eye[c]);
    Write<float>(kEngine + 54620, Read<float>(kFloats - 14200));
    Write<uint32_t>(kEngine + 4, 1);
    const auto focus = ScheduledFocusPoint(Ray(), Floats<3>(Camera() + 300),
                                         Read<float>(kFocusLimit), Read<float>(kFloats));
    const auto destination = kFocus + Bank() * 12;
    for (uint32_t c = 0; c < 3; ++c) Write<float>(destination + c * 4, focus[c]);
    PPCRegister descriptor;
    descriptor.u64 = view;
    if (bdNativeScenePostHook(descriptor)) return;
    // A post refusal cannot replay the parent after its scene has rendered.
    // Preserve the complete isolated compatibility sequence and its containers.
    const uint32_t depth = ctx.r1.u32 + 256, color = ctx.r1.u32 + 288;
    Call(sub_8221C9A0, {depth, Read<uint32_t>(Read<uint32_t>(view + 4) + 4)});
    Call(sub_8221C9A0, {color, Read<uint32_t>(Read<uint32_t>(view) + 4)});
    Call(bdEffectSlotArrayApply, {kEffectList, color, depth, Read<uint32_t>(kPhase)});
    DestroyPostContainer(color); DestroyPostContainer(depth);
    ++stats.post_adapters;
  }
};
} // namespace
bool TryScheduleRenderView(PPCContext &ctx, uint8_t *base) {
  const bool enabled = REXCVAR_GET(bd_native_scene_passes) && REXCVAR_GET(bd_native_passes);
  if (!enabled || !Prepare(ctx)) {
    ++stats.compatibility;
    stats.refused += enabled;
    Report();
    return false;
  }
  ViewAdapter adapter(ctx, base);
  ScheduleRenderView(adapter);
  Check(adapter.restored);
  ++stats.views;
  Report();
  return true;
}
} // namespace bd::gpu::scene
