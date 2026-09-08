/**
 * @brief Host authored-light publication; remaining shader staging is an adapter.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_selected_lights_source.h"
#include "gpu/scene/native_light_selection_source.h"
#include "gpu/scene/native_scene_lights_source.h"
#include "gpu/scene/native_instance_bridge.h"
#include "gpu/scene/native_lighting_bridge.h"
#include "gpu/scene/native_material_texture_bridge.h"
#include "gpu/scene/host_parameter_bridge.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <cstring>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/xthread.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_8218B0F0);
REX_EXTERN(__imp__sub_8218A8C8);
REX_EXTERN(__imp__sub_8218ADA0);
REXCVAR_DECLARE(bool, bd_native_lighting);
REXCVAR_DECLARE(bool, bd_native_materials_verify);
REXCVAR_DECLARE(bool, bd_native_rigid_scene);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kManager = (uint32_t(-32030) << 16) - 31092;
constexpr uint32_t kPublisher = (uint32_t(-32030) << 16) + 18164;
constexpr uint32_t kView = (uint32_t(-32035) << 16) - 26424;
constexpr uint32_t kStrength = (uint32_t(-32247) << 16) - 5576;
constexpr uint32_t kOne = (uint32_t(-32251) << 16) + 20908;
constexpr uint32_t kZero = (uint32_t(-32251) << 16) + 21040;
thread_local SelectedLightSourceState current;
thread_local uint32_t current_selection = 0, current_frame = ~0u;
thread_local bool exporting_lights = false;
struct Stats {
  uint64_t publications = 0, changes = 0, compatibility = 0, checked = 0, wrong = 0;
  uint64_t snapshots = 0, unavailable = 0;
  uint64_t draw_checks = 0, draw_wrong = 0;
};
thread_local Stats stats;
struct SelectionStats {
  uint64_t updates = 0, rebuilt = 0, candidates = 0, compatibility = 0, checked = 0, wrong = 0;
};
thread_local SelectionStats selection_stats;
struct SceneLights {
  std::mutex mutex;
  NativeSceneLightingPublication current;
  std::vector<NativeLightSourceBinding> sources;
  uint64_t published = 0, refused = 0, unavailable = 0, reads = 0, missing = 0;
  uint64_t inherited = 0, commits = 0, observed = 0, unowned = 0;
  uint32_t reported = 0;
};
SceneLights &Scene() { static SceneLights result; return result; }
void ObserveCompatibilityLightSelection(uint32_t selection, uint32_t view) {
  if (!REXCVAR_GET(bd_native_rigid_scene)) return;
  auto &scene = Scene();
  std::lock_guard lock(scene.mutex);
  const auto it = std::lower_bound(scene.sources.begin(),scene.sources.end(),selection,
      [](const auto &binding, uint32_t key) { return binding.selection < key; });
  const auto frame = FrameStatFrameCount();
  const auto ticket = it != scene.sources.end() && it->selection == selection
      ? scene.current.Prepare(frame,scene.current.Update(frame),it->instance,it->model_generation,it->node,view)
      : std::nullopt;
  // The observer computes from handoff-owned inputs. The compatibility result
  // is only a guard: stale/unsupported selection cannot seed native inheritance.
  // It is never copied into a native ticket or used to recover missing inputs.
  if (ticket && current.known == 7 && SameNativeSelectedLights(ticket->lights,current.lights) &&
      scene.current.Commit(frame,*ticket)) ++scene.observed;
  else { scene.current.InvalidateInherited(); ++scene.unowned; }
}
std::optional<uint32_t> Word(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX-3) return {};
  const auto *value = bd::mem::try_at<const be_u32>(uint32_t(address));
  return value ? std::optional(uint32_t(*value)) : std::nullopt;
}
void Report() {
  BD_INFO("[native-selected-lights] {} publications {} changed slots {} compatibility; "
          "{} checks wrong {}; {} object snapshots {} unavailable; {} draw checks wrong {}; authored selection and shader staging adapters remain",
      stats.publications, stats.changes, stats.compatibility, stats.checked, stats.wrong,
      stats.snapshots, stats.unavailable, stats.draw_checks, stats.draw_wrong);
  BD_INFO("[native-light-selection] {} updates {} rebuilds {} candidates {} compatibility; {} checks wrong {}; authored snapshot/storage adapters remain",
      selection_stats.updates, selection_stats.rebuilt, selection_stats.candidates,
      selection_stats.compatibility, selection_stats.checked, selection_stats.wrong);
}
std::optional<NativeLightSelectionPlan> PrepareSelection(uint32_t selection, uint32_t stack) {
  if (!rex::system::XThread::GetCurrentThread()) return {};
  const auto view = Word(kView);
  if (!view || *view >= 16) return {};
  // Includes the deepest original scoring/ray stack, for comparison and safe
  // unsupported fallback. Reject aliases before any selection write occurs.
  const auto safe_word = [&](uint64_t address) -> std::optional<uint32_t> {
    if (stack && address < stack && address+4 > uint64_t(stack)-704) return {};
    return Word(address);
  };
  const auto control = [&](uint32_t address) -> std::optional<uint32_t> {
    return LightSelectionOutput(address, selection, *view) ? std::nullopt : safe_word(address);
  };
  constexpr uint32_t kLiveOwner = (uint32_t(-32137) << 16)+30280;
  constexpr uint32_t kPrimaryThread = (uint32_t(-32035) << 16)-26664;
  constexpr uint32_t kSpecialScene = (uint32_t(-32035) << 16)-26232;
  constexpr uint32_t kScale = (uint32_t(-32247) << 16)-3992;
  constexpr uint32_t kAngleScale = (uint32_t(-32250) << 16)+11844;
  constexpr uint32_t kRayMin = (uint32_t(-32247) << 16)-5560;
  constexpr uint32_t kRayMax = (uint32_t(-32250) << 16)+8116;
  const auto owner = control(kLiveOwner), primary = control(kPrimaryThread), scene = control(kSpecialScene);
  const auto scale = control(kScale), angle = control(kAngleScale), low = control(kRayMin), high = control(kRayMax);
  if (!control(kView) || !owner || !primary || !scene || !scale || !angle || !low || !high ||
      control(kOne) != 0x3f800000u || control(kZero) != 0u) return {};
  bool special_scene = false;
  if (*scene) {
    if (*scene > UINT32_MAX-1035) return {};
    const auto mode = control(*scene+1032);
    if (!mode) return {};
    special_scene = *mode == 1;
  }
  const NativeLightSelectionSource source{kManager, selection, *owner, *view,
      rex::system::XThread::GetCurrentThreadId() == *primary, special_scene,
      {std::bit_cast<float>(*scale),std::bit_cast<float>(*angle),std::bit_cast<float>(*low),std::bit_cast<float>(*high)}};
  return PrepareNativeLightSelection(source, safe_word);
}
bool Select(PPCContext &ctx, uint8_t *base) {
  if (ctx.r3.u32 != kManager || ctx.r1.u32 < 704 || (ctx.r1.u32 & 15)) return false;
  const auto selection = ctx.r4.u32;
  const auto view = Word(kView);
  const auto plan = PrepareSelection(selection, ctx.r1.u32);
  if (!plan || !view) return false;
  if (REXCVAR_GET(bd_native_materials_verify)) {
    const auto before_view = Word(kView), before_dirty = Word(uint64_t(selection)+4);
    __imp__sub_8218A8C8(ctx, base);
    ++selection_stats.checked;
    if (const auto mismatch = FindNativeLightSelectionMismatch(*plan,Word)) {
      ++selection_stats.wrong;
      BD_ERROR("[native-light-selection-mismatch] selection {:08X} view {} address {:08X} actual {:08X} expected {:08X}",
          selection, *view, mismatch->write.address, mismatch->actual.value_or(0), mismatch->write.after);
      // Bounded failure-only observations, not an atomic snapshot or permission
      // to ignore a late invalidation. Preserve the refusal before any stores.
      BD_ERROR("[native-light-selection-state] frame {} rebuilt {} candidates {} pre-original view {} dirty {:08X}; post-original view {}",
          FrameStatFrameCount(),plan->rebuilt,plan->candidates,before_view.value_or(~0u),before_dirty.value_or(0),Word(kView).value_or(~0u));
      for (const auto &write : plan->writes)
        BD_ERROR("[native-light-selection-state] address {:08X} before {:08X} expected {:08X} observed {:08X}",
            write.address,write.before,write.after,Word(write.address).value_or(0));
      throw std::runtime_error("Native light selection differs from original");
    }
  }
  for (const auto &write : plan->writes)
    if (write.before != write.after) bd::mem::store<uint32_t>(write.address, write.after);
  ++selection_stats.updates; selection_stats.rebuilt += plan->rebuilt; selection_stats.candidates += plan->candidates;
  return true;
}
bool Publish(PPCContext &ctx, uint8_t *base) {
  const auto selection = ctx.r4.u32;
  if (ctx.r3.u32 != kPublisher || ctx.r1.u32 < 176 || (ctx.r1.u32 & 15)) return false;
  const auto view = Word(kView), count = Word(kManager+46816), numerator = Word(kStrength);
  if (!view || !count || !numerator || Word(kOne) != 0x3f800000u || Word(kZero) != 0u) return false;
  const auto scratch = uint64_t(ctx.r1.u32)-176;
  const auto safe_word = [&](uint64_t address) -> std::optional<uint32_t> {
    if (address < ctx.r1.u32 && address+4 > scratch) return {};
    return Word(address);
  };
  const auto publication = PrepareSelectedLights(kPublisher, selection, *view, kManager+24016,
      *count, std::bit_cast<float>(*numerator), current, safe_word,
      [](double angle) { return std::cos(angle); });
  if (!publication) return false;
  for (size_t n = 0; n < publication->count; ++n) {
    const auto address = publication->writes[n].address;
    if (address == kView || address == kManager+46816 || address == kStrength ||
        address == kOne || address == kZero) return false;
  }
  if (REXCVAR_GET(bd_native_materials_verify)) {
    __imp__sub_8218B0F0(ctx, base);
    ++stats.checked;
    for (size_t n = 0; n < publication->count; ++n) {
      const auto &write = publication->writes[n];
      const auto actual = Word(write.address);
      bool same = actual && *actual == write.word;
      if (!same && actual && write.cosine) {
        const auto a = std::bit_cast<float>(*actual), b = std::bit_cast<float>(write.word);
        same = std::isfinite(a) && std::isfinite(b) && std::abs(a-b) <= 2e-6f;
      }
      if (!same) {
        ++stats.wrong;
        BD_ERROR("[native-selected-light-mismatch] address {:08X} actual {:08X} expected {:08X} cosine {}",
            write.address, actual.value_or(0), write.word, write.cosine);
        throw std::runtime_error("Native selected-light publication differs from original");
      }
    }
  }
  // Compatibility writes consume the computed publication, never reread a
  // shader-register payload to manufacture native light ownership.
  for (size_t n = 0; n < publication->count; ++n) {
    const auto &write = publication->writes[n];
    bd::mem::store<uint32_t>(write.address, write.word);
  }
  current = publication->state;
  current_selection = selection;
  current_frame = FrameStatFrameCount();
  ObserveCompatibilityLightSelection(selection,*view);
  if (current.known == 7 && PublishNativeMaterialLights(selection, current.lights)) ++stats.snapshots;
  else InvalidateNativeMaterialLights();
  ++stats.publications; stats.changes += publication->changed;
  return true;
}
} // namespace

void PublishNativeSceneLights(uint32_t manager) {
  auto &scene = Scene();
  std::optional<NativeSceneLightSet> lights;
  std::vector<NativeNodeLightBinding> bindings;
  std::vector<NativeLightSourceBinding> sources;
  size_t unavailable = 0;
  try {
    // bdMainGameStep waited for DrawEnd, completed scene preparation and the
    // transfer callbacks; bdFrameSubmitAndDebugHUD has not signalled DrawStart.
    // The original snapshot and interpolation changed-list restoration are done.
    constexpr uint32_t kPrimary = (uint32_t(-32035)<<16)-26664;
    constexpr uint32_t kSpecial = (uint32_t(-32035)<<16)-26232;
    const auto primary = Word(kPrimary), special = Word(kSpecial), strength = Word(kStrength);
    const auto scale = Word((uint32_t(-32247)<<16)-3992), angle = Word((uint32_t(-32250)<<16)+11844);
    const auto low = Word((uint32_t(-32247)<<16)-5560), high = Word((uint32_t(-32250)<<16)+8116);
    const auto mode = special && *special ? Word(uint64_t(*special)+1032) : std::optional(0u);
    if (REXCVAR_GET(bd_native_lighting) && manager == kManager && primary && special && mode &&
        strength && scale && angle && low && high && rex::system::XThread::GetCurrentThread() &&
        rex::system::XThread::GetCurrentThreadId() == *primary) {
      lights = ReadNativeSceneLightSet(manager, *mode == 1,
          {std::bit_cast<float>(*scale),std::bit_cast<float>(*angle),std::bit_cast<float>(*low),std::bit_cast<float>(*high)},
          std::bit_cast<float>(*strength), Word);
      if (lights && !CollectNativeInstanceLightInputs(bindings, sources, unavailable)) lights.reset();
    }
  } catch (const std::exception &error) {
    lights.reset();
    BD_WARN("[native-scene-lights] handoff failed: {}", error.what());
  }
  std::lock_guard lock(scene.mutex);
  const auto frame = FrameStatFrameCount();
  const bool published = lights && scene.current.Publish(frame, std::move(*lights), std::move(bindings));
  if (!published) { scene.current.Reset(); scene.sources.clear(); }
  else {
    std::sort(sources.begin(),sources.end(),[](const auto &a, const auto &b) { return a.selection < b.selection; });
    sources.erase(std::unique(sources.begin(),sources.end(),
        [](const auto &a, const auto &b) { return a.selection == b.selection; }),sources.end());
    scene.sources = std::move(sources);
  }
  ++(published ? scene.published : scene.refused);
  scene.unavailable += unavailable;
  if (!REXCVAR_GET(bd_native_rigid_scene)) scene.current.InvalidateInherited();
}
uint64_t NativeSceneLightUpdate(uint32_t frame) {
  auto &scene = Scene();
  std::lock_guard lock(scene.mutex);
  return scene.current.Update(frame);
}
std::optional<NativeSceneLightRecipe> CaptureNativeSceneLights(uint64_t instance,
    uint64_t model_generation, uint32_t node, const NativeLightingInputs &pass) {
  if (!REXCVAR_GET(bd_native_lighting)) return {};
  auto &scene = Scene();
  std::lock_guard lock(scene.mutex);
  const auto frame = FrameStatFrameCount();
  const auto result = scene.current.Capture(frame, pass.light_update, instance, model_generation, node, pass.light_view);
  if (!result) ++scene.missing;
  if (frame-scene.reported >= 300 || (!result && scene.missing <= 3)) {
    BD_INFO("[native-scene-lights] frame {} update {} pass update {} light view {}; {} publications {} refused; "
            "{} bindings {} unavailable imports; {} native reads {} missing; instance {} generation {} node {}; no legacy selection/cache reads",
        frame, scene.current.Update(frame), pass.light_update, pass.light_view, scene.published, scene.refused,
        scene.current.Bindings(), scene.unavailable, scene.reads, scene.missing, instance, model_generation, node);
    scene.reported = frame;
    BD_INFO("[native-light-order] frame {} inherited preparations {} native commits {} owned callback observations {} unowned observations {}; no guessed defaults",
        frame,scene.inherited,scene.commits,scene.observed,scene.unowned);
  }
  return result;
}
std::optional<NativeSceneLightTicket> ResolveNativeSceneLights(const NativeSceneLightRecipe &recipe) {
  if (!REXCVAR_GET(bd_native_lighting)) return {};
  auto &scene = Scene();
  std::lock_guard lock(scene.mutex);
  const auto result = scene.current.Resolve(FrameStatFrameCount(), recipe);
  ++(result ? scene.reads : scene.missing);
  scene.inherited += result && result->inherited;
  return result;
}
bool CommitNativeSceneLights(const NativeSceneLightTicket &ticket, uint32_t stack) {
  if (!REXCVAR_GET(bd_native_lighting) || stack < 256 || (stack & 15)) return false;
  auto &scene = Scene();
  std::lock_guard lock(scene.mutex);
  const auto frame = FrameStatFrameCount();
  if (!scene.current.CanCommit(frame,ticket)) return false;
  const auto safe_word = [&](uint64_t address) -> std::optional<uint32_t> {
    return address < stack && address+4 > uint64_t(stack)-256 ? std::nullopt : Word(address);
  };
  const auto mirror = PrepareNativeLightMirror(kPublisher,ticket.lights,safe_word);
  const auto first = safe_word(kPublisher+16), second = safe_word(kPublisher+28);
  if (!mirror || !first || !second || !*first || !*second ||
      !CanFlushHostParameterDescriptor(*first,stack) || !CanFlushHostParameterDescriptor(*second,stack)) return false;
  for (size_t n = 0; n < mirror->count; ++n) {
    const auto address = mirror->writes[n].address;
    if ((uint64_t(address) < uint64_t(*first)+16 && uint64_t(address)+4 > *first) ||
        (uint64_t(address) < uint64_t(*second)+16 && uint64_t(address)+4 > *second) ||
        address == kPublisher+16 || address == kPublisher+28 || address == kView ||
        address == kManager+46816 || address == kStrength || address == kOne || address == kZero ||
        address == (uint32_t(-32133)<<16)-31532) return false;
  }
  // All CPU plans/participation are known before this call. Flush outgoing
  // compatibility values now, before backend locking or the next legacy draw.
  // A later backend refusal is terminal, never a partially replaced fallback.
  struct ExportScope {
    ExportScope() { exporting_lights = true; }
    ~ExportScope() { exporting_lights = false; }
  } exporting;
  for (size_t n = 0; n < mirror->count; ++n)
    bd::mem::store<uint32_t>(mirror->writes[n].address,mirror->writes[n].word);
  if (!FlushHostParameterDescriptor(*first,stack) || !FlushHostParameterDescriptor(*second,stack))
    throw std::runtime_error("Native light mirror changed after preflight");
  current = {}; current_selection = 0; current_frame = ~0u;
  InvalidateNativeMaterialLights();
  if (!scene.current.Commit(frame,ticket)) throw std::runtime_error("Native light order changed during commit");
  ++scene.commits;
  return true;
}
void ObserveNativeSceneLightParameters(bool vertex, uint32_t first, uint32_t count, const void *words) {
  if (exporting_lights || vertex) return;
  if (first <= 256 && count <= 256-first && (!count || first >= 32 || uint64_t(first)+count <= 20)) return;
  auto &scene = Scene();
  std::lock_guard lock(scene.mutex);
  ObserveNativeLightParameterWrite(scene.current,vertex,first,count,static_cast<const uint32_t *>(words));
}
void InvalidateNativeSceneLightInheritance() {
  auto &scene = Scene();
  std::lock_guard lock(scene.mutex);
  scene.current.InvalidateInherited();
}
std::optional<NativeSelectedLights> FindNativeSelectedLights(uint32_t selection) {
  if (!REXCVAR_GET(bd_native_lighting) || current.known != 7 || current_selection != selection ||
      current_frame != FrameStatFrameCount()) { ++stats.unavailable; return {}; }
  ++stats.snapshots;
  return current.lights;
}
void NativeSelectedLightsReport() { Report(); }
void CheckNativeSelectedLights(const NativeSelectedLights &lights, const uint8_t *pixel_constants) {
  ++stats.draw_checks;
  bool same = true;
  for (size_t slot = 0; slot < lights.size(); ++slot) {
    std::array<float, 16> actual;
    std::memcpy(actual.data(), pixel_constants+(20+slot*4)*16, sizeof(actual));
    const auto &light = lights[slot];
    const int kind = actual[3] < .5f ? LitDisabled : actual[3] < 1.5f ? LitDirectional :
                     actual[3] < 2.5f ? LitSpot : LitPoint;
    same &= std::isfinite(actual[3]) && kind == light.kind;
    const auto xyz = [&](size_t at, LitVector expected) {
      return actual[at] == expected.x && actual[at+1] == expected.y && actual[at+2] == expected.z;
    };
    if (light.kind == LitDisabled) continue;
    same &= xyz(8, light.colour);
    if (light.kind == LitDirectional || light.kind == LitSpot) same &= xyz(4, light.direction);
    if (light.kind == LitPoint || light.kind == LitSpot)
      same &= xyz(0, light.position) && actual[11] == light.inverse_range;
    if (light.kind == LitSpot)
      same &= actual[7] == light.cone_strength && std::isfinite(actual[12]) &&
              std::abs(actual[12]-light.cone_cosine) <= 2e-6f;
  }
  if (!same) {
    ++stats.draw_wrong;
    BD_ERROR("[native-selected-light-mismatch] object snapshot differs at normal-lit draw");
    throw std::runtime_error("Native selected lights differ at draw consumption");
  }
}
void PublishNativeSelectedLights(PPCContext &ctx, uint8_t *base) {
  if (!REXCVAR_GET(bd_native_lighting) || !Publish(ctx, base)) {
    InvalidateNativeSceneLightInheritance();
    current = {}; current_selection = 0; current_frame = ~0u;
    InvalidateNativeMaterialLights();
    ++stats.compatibility;
    __imp__sub_8218B0F0(ctx, base);
  }
}
void UpdateNativeLightSelection(PPCContext &ctx, uint8_t *base) {
  if (!REXCVAR_GET(bd_native_lighting) || !Select(ctx, base)) {
    ++selection_stats.compatibility;
    __imp__sub_8218A8C8(ctx, base);
  }
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(sub_8218B0F0) {
  bd::gpu::scene::PublishNativeSelectedLights(ctx, base);
}
REX_HOOK_RAW(sub_8218A8C8) {
  bd::gpu::scene::UpdateNativeLightSelection(ctx, base);
}
REX_HOOK_RAW(sub_8218ADA0) {
  bd::gpu::scene::InvalidateNativeSceneLightInheritance();
  __imp__sub_8218ADA0(ctx, base);
}
