/**
 * @brief Opt-in selected-asset reload acceptance through the game's task dispatcher.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_rigid_lifecycle_bridge.h"
#include "gpu/scene/native_rigid_lifecycle.h"
#include "gpu/scene/native_rigid_shadow.h"
#include "gpu/scene/native_instance.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include "core/task_layout.h"
#include "engine/game.h"
#include "engine/state_layout.h"
#include <mutex>
#include <chrono>
#include <stdexcept>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/xthread.h>

REXCVAR_DEFINE_BOOL(bd_native_rigid_reload, false, kCvarGroup,
    "One diagnostic return-to-title/reload of the selected native rigid asset. Requires hard-off and autoplay; captures remain separately controlled.");
REXCVAR_DECLARE(bool, bd_native_rigid_hard_off);
REXCVAR_DECLARE(bool, bd_xr_autoplay);
REX_EXTERN(__imp__SequenceControl_vf02);
REX_IMPORT(__imp__SequenceHolder_FindSequenceByName, RigidFindSequenceByName, uint32_t(uint32_t,uint32_t));

namespace bd::gpu::scene {
namespace {
void Require(bool value, const char *reason) {
  if (value) return;
  BD_ERROR("[native-rigid-reload] refused: {}", reason);
  throw std::runtime_error(reason);
}
struct Evidence {
  std::mutex mutex;
  NativeRigidLifecycle lifecycle;
  uint64_t latest = 0;
  bool ready = false;
  std::chrono::steady_clock::time_point observed{};
  NativeRigidReloadInput input{};
};
Evidence &State() { static Evidence evidence; return evidence; }
// Called with the diagnostic mutex held. No GPU/model locks or guest calls.
void Report(const char *event, const NativeRigidEpoch &epoch) {
  const auto &shadow = epoch.views[0]; const auto &scene = epoch.views[1];
  BD_INFO("[native-rigid-lifecycle] {} generation {} instance {} source-retired {} scene {}/{}/{} shadow {}/{}/{}; submitted/emitted/fence-retired",
      event, epoch.generation, epoch.first_instance, epoch.source_retired ? 1 : 0,
      scene.submitted,scene.emitted,scene.retired,shadow.submitted,shadow.emitted,shadow.retired);
}
void Note(NativeRigidLifecycle::Event event, uint64_t generation, uint64_t instance, uint32_t view, uint32_t count) {
  if (!NativeRigidLifecycleEnabled()) return;
  auto &state = State(); std::lock_guard lock(state.mutex);
  Require(state.lifecycle.Note(event,generation,instance,view,count), "generation-specific native event out of order or outside budget");
}
}
bool NativeRigidLifecycleEnabled() { return REXCVAR_GET(bd_native_rigid_reload); }
uint64_t NativeRigidRetiringGeneration(const std::shared_ptr<const NativeModelRenderData> &model) {
  if (!NativeRigidLifecycleEnabled() || !model) return 0;
  // Loader-local classification only. No first-draw discovery or new alias map.
  for (uint32_t node = 0; node < NativeInstanceRegistry::kMaxTransforms; ++node)
    if (const auto *program = model->FindNode(node); program && SelectedNativeRigidShadow(*program)) return model->Generation();
  return 0;
}
void NoteNativeRigidModelLoaded(const std::shared_ptr<const NativeModelRenderData> &model) {
  const auto generation = NativeRigidRetiringGeneration(model);
  if (!generation) return;
  auto &state = State(); std::lock_guard lock(state.mutex);
  Require(state.lifecycle.Loaded(generation), "duplicate selected generation or eight-epoch diagnostic capacity exhausted");
  state.latest = generation;
  Report("loaded",*state.lifecycle.Find(generation));
}
void NoteNativeRigidSourceRetired(uint64_t generation) {
  if (!generation) return;
  auto &state = State(); std::lock_guard lock(state.mutex);
  Require(state.lifecycle.SourceRetired(generation), "selected source retired without its load generation");
  Report("source-retired",*state.lifecycle.Find(generation));
}
void NoteNativeRigidSubmitted(uint64_t generation, uint64_t instance, uint32_t view) {
  Note(NativeRigidLifecycle::Event::Submitted,generation,instance,view,1);
}
void NoteNativeRigidEmitted(uint64_t generation, uint32_t view, uint32_t count) {
  Note(NativeRigidLifecycle::Event::Emitted,generation,0,view,count);
}
void NoteNativeRigidFenceRetired(uint64_t generation, uint32_t view) {
  Note(NativeRigidLifecycle::Event::FenceRetired,generation,0,view,1);
}
void ObserveNativeRigidReloadField(bool walking, uint64_t stage) {
  if (!NativeRigidLifecycleEnabled()) return;
  auto &state = State(); std::lock_guard lock(state.mutex);
  state.ready = !state.input.paused && walking && stage == ((uint64_t(2) << 32) | 4101);
  state.observed = std::chrono::steady_clock::now();
}
NativeRigidReloadInput GetNativeRigidReloadInput() {
  if (!NativeRigidLifecycleEnabled()) return {};
  auto &state = State(); std::lock_guard lock(state.mutex);
  return state.input;
}

// This tick runs on the original sequence task's thread, after its update.
// The input/GPU/async loader callbacks never call game functions. Copy evidence
// before touching game state: no diagnostic lock -> model/video lock inversion.
void TickNativeRigidReload(uint32_t sequence) {
  if (!NativeRigidLifecycleEnabled()) return;
  Require(REXCVAR_GET(bd_native_rigid_hard_off) && REXCVAR_GET(bd_xr_autoplay), "reload requires hard-off and autoplay from startup");
  if (!sequence || sequence != mem::try_load<uint32_t>(engine::addr::kSequenceControl)) return;
  enum class Phase { ColdField, Title, ReloadedField, Complete };
  static Phase phase = Phase::ColdField;
  static NativeRigidOutputWindow window;
  static uint64_t old_generation = 0, old_instance = 0, old_task_uid = 0;
  static TaskRef dispatcher;
  if (phase == Phase::Complete) return;
  auto &state = State();
  NativeRigidEpoch current, old;
  bool ready;
  {
    std::lock_guard lock(state.mutex);
    if (const auto *epoch = state.lifecycle.Find(state.latest)) current = *epoch;
    if (const auto *epoch = state.lifecycle.Find(old_generation)) old = *epoch;
    ready = state.ready && std::chrono::steady_clock::now() - state.observed < std::chrono::milliseconds(250);
  }
  const auto child = mem::try_field<uint32_t>(sequence,104);
  const auto game_task = mem::try_load<uint32_t>(engine::addr::kGameTask);
  if (phase == Phase::ColdField && window.Step(ready,current)) {
    Require(LiveTask(sequence) && TaskUID(sequence) && LiveTask(game_task) && TaskUID(game_task) && child == game_task &&
        mem::try_field<uint32_t>(game_task,72) == sequence && mem::try_field<uint64_t>(game_task,80) == TaskUID(sequence) &&
        mem::try_field<uint32_t>(sequence,112) == 0 && mem::try_field<uint32_t>(sequence,116) == 0 &&
        mem::try_field<uint32_t>(game_task,140) == 0, "return-to-title task contract changed");
    // Exact registered Title name used by bdGameTaskUpdate at 0x820C4930.
    // +112 is a sequence ID, not a task pointer. Resolve, never assume the ID.
    const auto title_id = RigidFindSequenceByName(sequence,0x82065008);
    Require(title_id != 0, "registered Title sequence unavailable");
    dispatcher = TaskRef(sequence); old_task_uid = TaskUID(game_task);
    old_generation = current.generation; old_instance = current.first_instance;
    {
      std::lock_guard lock(state.mutex);
      state.input.paused = true; state.ready = false;
      Report("cold-qualified",current);
      BD_INFO("[native-rigid-reload] window generation {} scene {}->{} shadow {}->{}",current.generation,
          window.Baseline()[1],current.views[1].emitted,window.Baseline()[0],current.views[0].emitted);
    }
    // Same deferred shutdown request as the game, never a direct model retire
    // or a forced fence drain. The task dispatcher owns destruction/notification.
    mem::store<uint32_t>(game_task+140,1);
    mem::store<uint32_t>(sequence+112,title_id);
    KillTask(game_task);
    BD_INFO("[native-rigid-reload] title requested generation {} instance {} task-uid {} sequence-id {}",old_generation,old_instance,old_task_uid,title_id);
    phase = Phase::Title;
  } else if (phase == Phase::Title) {
    Require(dispatcher.Is(sequence), "sequence dispatcher identity changed during reload");
    if (!old.Closed() || game_task || mem::try_load<uint32_t>(engine::addr::kFieldSceneCtl) ||
        !LiveTask(child) || !TaskUID(child) || TaskUID(child) == old_task_uid ||
        engine::Game::Get().Mode() != engine::EngineMode::TitleOrMenu) return;
    std::lock_guard lock(state.mutex);
    Report("closed-at-title",old);
    ++state.input.serial; state.input.paused = false; state.ready = false;
    window = {}; phase = Phase::ReloadedField;
    BD_INFO("[native-rigid-reload] title reached; old generation {} fully fence-retired; autoplay epoch {}",old_generation,state.input.serial);
  } else if (phase == Phase::ReloadedField) {
    Require(dispatcher.Is(sequence), "sequence dispatcher identity changed after title");
    if (!ready) { window.Step(false,current); return; }
    Require(old.Closed() && current.generation && current.generation != old_generation &&
        current.first_instance && current.first_instance != old_instance && LiveTask(game_task) &&
        TaskUID(game_task) && TaskUID(game_task) != old_task_uid && child == game_task,
        "field returned without fresh selected model, instance and game-task identities");
    if (!window.Step(true,current)) return;
    std::lock_guard lock(state.mutex);
    Report("reload-qualified",current);
    BD_INFO("[native-rigid-reload] window generation {} scene {}->{} shadow {}->{}",current.generation,
        window.Baseline()[1],current.views[1].emitted,window.Baseline()[0],current.views[0].emitted);
    BD_INFO("[native-rigid-reload] complete old-generation {} new-generation {} old-instance {} new-instance {}; 900 fresh scene and shadow emissions in each interactive field epoch",old_generation,current.generation,old_instance,current.first_instance);
    phase = Phase::Complete;
  }
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(SequenceControl_vf02) {
  const auto sequence = ctx.r3.u32;
  __imp__SequenceControl_vf02(ctx,base);
  bd::gpu::scene::TickNativeRigidReload(sequence);
}
