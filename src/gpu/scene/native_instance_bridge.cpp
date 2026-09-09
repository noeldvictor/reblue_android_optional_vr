/**
 * @brief Publish instance poses at their producer, consume without palette reads.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_instance_bridge.h"
#include "gpu/scene/native_instance_source.h"
#include "gpu/scene/native_skeleton_source.h"
#include "gpu/scene/native_animation_bridge.h"
#include "gpu/scene/native_scene_lights_source.h"
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_rigid_route.h"
#include "gpu/scene/native_rigid_route_bridge.h"
#include "gpu/scene/native_primitive_policy_source.h"
#include "gpu/scene/native_deferred_contract.h"
#include "gpu/scene/guest_scene.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include "engine/frame_clock.h"
#include "gpu/frame_stats.h"
#include <algorithm>
#include <stdexcept>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/xthread.h>

REXCVAR_DEFINE_BOOL(bd_native_instances, true, kCvarGroup,
    "Producer-owned instance poses for native traversal and draw transforms.");
REXCVAR_DEFINE_BOOL(bd_native_skeleton, false, kCvarGroup,
    "Host evaluation of load-owned ordinary skinned skeletons. Pending desktop qualification; native_materials_verify compares the original.");
REXCVAR_DECLARE(bool, bd_native_materials_verify);
REXCVAR_DECLARE(bool, bd_native_rigid_scene);
REXCVAR_DECLARE(bool, bd_native_rigid_shadow);
REXCVAR_DECLARE(bool, bd_host_walk);
REXCVAR_DEFINE_BOOL(bd_native_rigid_hard_off, false, kCvarGroup,
    "Require load-owned rigid routing before culling and reject selected-family legacy entry in every view. Requires both native rigid paths.");
REX_EXTERN(__imp__bdVisualObjectInitBones);
REX_EXTERN(__imp__bdBoneInitSkinned);
REX_EXTERN(__imp__sub_82140DF8);
REX_EXTERN(__imp__sub_8213F5E8);

namespace bd::gpu::scene {
namespace {
static_assert(kVisualBoneContainer == instance_source::kPaletteContainer);
enum class SkeletonMissing : size_t {
  Scope, Publication, UpdateLane, SourceMatch, ChildStop, Model, Topology, Palette,
  Channels, Root, Evaluation, Count
};
constexpr std::array skeleton_missing_names{"scope", "publication", "update lane", "source match",
    "child stop", "model", "topology", "palette", "channels", "root", "evaluation"};
static_assert(skeleton_missing_names.size() == size_t(SkeletonMissing::Count));
struct Store {
  std::mutex mutex;
  NativeInstanceRegistry instances;
  std::unordered_map<uint32_t, instance_source::Binding> sources;
  uint64_t imports = 0, refused = 0, reads = 0, unavailable = 0, checked = 0, wrong = 0;
  uint64_t handoffs = 0, handoff_missing = 0;
  uint64_t skeleton_updates = 0, skeleton_published = 0, skeleton_unavailable = 0, skeleton_checks = 0, skeleton_wrong = 0;
  std::array<uint64_t, size_t(SkeletonMissing::Count)> skeleton_missing{};
  uint32_t miss_examples = 0;
  uint32_t drift_examples = 0;
  uint32_t frame = 0;
  std::optional<NativePosePhase> render_phase;
};
Store &Instances() { static Store result; return result; }
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address + bytes > uint64_t(UINT32_MAX) + 1 ||
      !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096;
       page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
std::optional<uint32_t> Word(uint64_t address) {
  if ((address & 3) || !Range(address, 4)) return {};
  return bd::mem::load<uint32_t>(uint32_t(address));
}
void Report(Store &store) {
  const auto frame = FrameStatFrameCount();
  if (frame - store.frame < 300) return;
  const auto stats = store.instances.Stats();
  BD_INFO("[native-instances] {} created {} retired {} live / {} bytes; "
          "{} poses published {} reused; {} producer imports {} refused; "
          "{} consumer reads {} unavailable; {} checks wrong {}; "
          "{} handoffs {} unavailable; original pose calculation, secondary palettes and source index remain",
      stats.created, stats.retired, stats.indexed, stats.bytes, stats.published,
      stats.reused, store.imports, store.refused + stats.refused, store.reads,
      store.unavailable, store.checked, store.wrong, store.handoffs, store.handoff_missing);
  if (stats.render_reads)
    BD_INFO("[native-render-poses] frame {} reads {} blends {} reused {} snapped {} refused {}; immutable whole-pose interpolation, no guest scratch",
        frame, stats.render_reads, stats.render_blends, stats.render_reused, stats.render_snaps, stats.render_refused);
  if (REXCVAR_GET(bd_native_skeleton))
    BD_INFO("[native-skeleton] frame {} evaluated {} update poses {} unavailable {}; checked {} wrong {}; ordinary skinned hierarchy, curve/late-writer/copy adapters remain",
        frame, store.skeleton_updates, store.skeleton_published, store.skeleton_unavailable, store.skeleton_checks, store.skeleton_wrong);
  store.frame = frame;
}
struct SkeletonEvaluationScope;
thread_local SkeletonEvaluationScope *active_skeleton_evaluation = nullptr;
struct SkeletonEvaluationScope {
  uint32_t visual;
  SkeletonEvaluationScope *previous;
  NativeModelRenderHandle model;
  std::vector<RenderMatrix> pose;
  explicit SkeletonEvaluationScope(uint32_t source_visual)
      : visual(source_visual), previous(active_skeleton_evaluation) { active_skeleton_evaluation = this; }
  ~SkeletonEvaluationScope() { active_skeleton_evaluation = previous; }
};
bool EvaluateOwnedBones(PPCContext &ctx, uint8_t *base) {
  auto *scope = active_skeleton_evaluation;
  uint32_t count = 0; size_t joints = 0;
  auto unavailable = [&](SkeletonMissing reason) {
    auto &store = Instances(); std::lock_guard lock(store.mutex);
    ++store.skeleton_unavailable;
    if (++store.skeleton_missing[size_t(reason)] <= 2)
      BD_INFO("[native-skeleton-unavailable] {} visual {:08X} graph {:08X} palette {:08X} channels {:08X} count {} joints {}; original whole call retained",
          skeleton_missing_names[size_t(reason)], scope ? scope->visual : 0,
          ctx.r4.u32,ctx.r3.u32,ctx.r5.u32,count,joints);
    return false;
  };
  if (!scope || !rex::system::XThread::GetCurrentThread()) return unavailable(SkeletonMissing::Scope);
  const auto publication = instance_source::ReadPublication(
      scope->visual, rex::system::XThread::GetCurrentThreadId(), Word);
  if (!publication) return unavailable(SkeletonMissing::Publication);
  count = publication->count;
  if (publication->lane != 0) return unavailable(SkeletonMissing::UpdateLane);
  if (publication->graph != ctx.r4.u32 || publication->palette != ctx.r3.u32)
    return unavailable(SkeletonMissing::SourceMatch);
  const auto excluded_child = Word(0x82DC99DC); // source's optional child-walk stop node
  if (!excluded_child || *excluded_child) return unavailable(SkeletonMissing::ChildStop);
  const auto model = FindLoadedNativeModel(publication->graph);
  if (!model) return unavailable(SkeletonMissing::Model);
  joints = model->Skeleton().size();
  if (!joints || joints != count) return unavailable(SkeletonMissing::Topology);
  if (!Range(publication->palette, uint64_t(count)*sizeof(RenderMatrix))) return unavailable(SkeletonMissing::Palette);
  auto channels = TakeNativeAnimationChannels(scope->visual,publication->graph,model->Generation(),ctx.r5.u32);
  if (!channels) channels=skeleton_source::ReadChannels(ctx.r5.u32, publication->count, Word);
  if (!channels) return unavailable(SkeletonMissing::Channels);
  const auto root = skeleton_source::ReadRoot(
      {ctx.r6.u64,ctx.r7.u64,ctx.r8.u64,ctx.r9.u64,ctx.r10.u64},ctx.r1.u32,Word);
  if (!root) return unavailable(SkeletonMissing::Root);
  std::vector<RenderMatrix> pose;
  if (!EvaluateNativeSkeleton(model->Skeleton(),*channels,*root,pose)) return unavailable(SkeletonMissing::Evaluation);
  if (REXCVAR_GET(bd_native_materials_verify)) {
    __imp__bdBoneInitSkinned(ctx,base);
    const auto *original = bd::mem::at<const be_f32>(publication->palette);
    bool same = true; size_t first_joint = 0, first_component = 0;
    for (size_t n=0; n<pose.size(); ++n) for (size_t c=0; c<16; ++c) {
      const float expected = original[n*16+c], actual = pose[n][c];
      const bool equal = std::isfinite(expected) && std::abs(actual-expected) <= 1e-4f*std::max(1.0f,std::abs(expected));
      if (same && !equal) { first_joint = n; first_component = c; }
      same &= equal;
    }
    auto &store = Instances(); std::lock_guard lock(store.mutex);
    ++store.skeleton_checks;
    if (!same) {
      ++store.skeleton_wrong;
      BD_ERROR("[native-skeleton-drift] visual {:08X} model {} joint {} component {} native {} original {}; evaluated pose differs, no replacement/fallback",
          scope->visual,model->Generation(),first_joint,first_component,
          pose[first_joint][first_component],float(original[first_joint*16+first_component]));
      throw std::runtime_error("Native skeleton source comparison failed");
    }
  }
  // Outgoing adapter for collision/effects and still-unconverted late writers.
  // The evaluated values also enter the native update owner, never a draw cache.
  auto *output = bd::mem::at<be_f32>(publication->palette);
  for (size_t n=0; n<pose.size(); ++n) for (size_t c=0; c<16; ++c) output[n*16+c] = pose[n][c];
  scope->model = model; scope->pose = std::move(pose);
  auto &store = Instances(); std::lock_guard lock(store.mutex);
  ++store.skeleton_updates;
  return true;
}
void PublishEvaluatedPose(const SkeletonEvaluationScope &scope) {
  if (!scope.model || scope.pose.empty()) return;
  auto &store = Instances(); std::lock_guard lock(store.mutex);
  const auto found = store.sources.find(scope.visual);
  if (found == store.sources.end() || found->second.model_generation != scope.model->Generation() ||
      !store.instances.Publish(found->second.instance,0,scope.pose))
    throw std::runtime_error("Native evaluated update pose publication refused");
  ++store.skeleton_published;
}
void Retire(uint32_t visual) {
  RetireNativeAnimationChannels(visual);
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  if (const auto it = store.sources.find(visual); it != store.sources.end()) {
    store.instances.Retire(it->second.instance);
    store.sources.erase(it);
  }
}
void Attach(uint32_t visual) {
  if (!rex::system::XThread::GetCurrentThread()) { Retire(visual); return; }
  const auto input_source = instance_source::ReadPublication(
      visual, rex::system::XThread::GetCurrentThreadId(), Word);
  if (!input_source) {
    Retire(visual);
    auto &store = Instances();
    std::lock_guard lock(store.mutex);
    ++store.refused; Report(store); return;
  }
  const auto graph = input_source->graph;
  const auto model = FindLoadedNativeModel(graph);
  const auto generation = model ? model->Generation() : 0;
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  auto it = store.sources.find(visual);
  if (it != store.sources.end() && it->second.model_generation != generation) {
    store.instances.Retire(it->second.instance);
    store.sources.erase(it); it = store.sources.end();
  }
  if (!generation) {
    ++store.refused; Report(store); return;
  }
  if (it == store.sources.end()) {
    const auto id = store.instances.Create(generation, model);
    if (!id) { Report(store); return; }
    try { it = store.sources.emplace(visual, instance_source::Binding{id, generation, {}}).first; }
    catch (...) { store.instances.Retire(id); throw; }
  }
  // InitBones is not the last writer (runtime 898 proves later edits before
  // handoff). Establish identity here, but import no provisional matrix values.
  Report(store);
}
void Handoff(uint32_t container) {
  if (container < kVisualBoneContainer) return;
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  const auto it = store.sources.find(container - kVisualBoneContainer);
  // The helper also copies unconverted secondary/other containers. Never
  // discover an instance here or infer one from equal matrix values.
  if (it == store.sources.end()) return;
  const auto transfer = instance_source::ReadTransfer(container, Word);
  // Import once at the authoritative publication boundary, after ALL pose
  // writers and the actual conditional copy. Never re-import from a draw lookup.
  thread_local std::array<RenderMatrix, NativeInstanceRegistry::kMaxTransforms> scratch;
  std::span<const RenderMatrix> completed;
  if (transfer && Range(transfer->destination, uint64_t(transfer->count) * sizeof(RenderMatrix))) {
    const auto *input = bd::mem::at<const be_f32>(transfer->destination);
    for (uint32_t matrix = 0; matrix < transfer->count; ++matrix)
      for (uint32_t element = 0; element < 16; ++element)
        scratch[matrix][element] = float(input[matrix * 16 + element]);
    completed = {scratch.data(), transfer->count};
  }
  const bool transferred = instance_source::PublishCompletedTransfer(
      store.instances, it->second, transfer, completed);
  if (transferred)
    store.instances.ObserveRenderTick(it->second.instance, bd::engine::TickCount(), bd::engine::InterpolationActive());
  ++store.imports;
  if (!transferred) ++store.refused;
  ++(transferred ? store.handoffs : store.handoff_missing);
  Report(store);
}
} // namespace

NativeVisualIdentity FindNativeVisualIdentity(uint32_t visual) {
  if (!REXCVAR_GET(bd_native_instances) || !visual) return {};
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  const auto it = store.sources.find(visual);
  return it == store.sources.end() ? NativeVisualIdentity{} :
      NativeVisualIdentity{it->second.instance, it->second.model_generation};
}
namespace {
thread_local NativeVisualInputScope *active_visual_inputs = nullptr;
}
NativeVisualInputScope::~NativeVisualInputScope() {
  if (active_visual_inputs == this) active_visual_inputs = nullptr;
}
bool NativeVisualInputScope::Begin(std::span<const NativeVisualIdentity> requested, uint32_t frame) {
  if (active_visual_inputs) return false;
  std::vector<NativeVisualInputs> inputs;
  if (!CollectNativeVisualInputs(requested, inputs) || !inputs_.Publish(frame, std::move(inputs))) return false;
  requested_.assign(requested.begin(), requested.end());
  frame_ = frame; refreshes_ = 0; active_visual_inputs = this;
  return true;
}
std::optional<NativeVisualInputs> NativeVisualInputScope::ReadAfterWriter(
    NativeVisualIdentity identity, uint32_t frame) const {
  if (active_visual_inputs != this || frame != FrameStatFrameCount() || !inputs_.Read(identity, frame)) return {};
  std::vector<NativeVisualInputs> current;
  if (!CollectNativeVisualInputs(std::span(&identity, 1), current)) return {};
  return current.front();
}
void RefreshNativeVisualInputsAfterWriter() {
  if (!active_visual_inputs) return;
  auto &scope = *active_visual_inputs;
  std::vector<NativeVisualInputs> inputs;
  if (scope.frame_ != FrameStatFrameCount() || !CollectNativeVisualInputs(scope.requested_, inputs) ||
      !scope.inputs_.Publish(scope.frame_, std::move(inputs)))
    throw std::runtime_error("Native deferred authored input refresh unavailable");
  ++scope.refreshes_;
}
bool CollectNativeVisualInputs(std::span<const NativeVisualIdentity> requested,
    std::vector<NativeVisualInputs> &out) {
  out.clear();
  if (!REXCVAR_GET(bd_native_instances) || requested.empty() || requested.size() > NativeVisualPublication::kMaxVisuals)
    return false;
  for (size_t i = 0; i < requested.size(); ++i)
    if (!requested[i] || (i && requested[i-1] >= requested[i])) return false;
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  out.reserve(requested.size());
  for (const auto &[visual, binding] : store.sources) {
    const NativeVisualIdentity identity{binding.instance, binding.model_generation};
    if (!std::binary_search(requested.begin(), requested.end(), identity)) continue;
    const auto pose = store.instances.Read(binding.instance, 1);
    const auto inputs = ReadNativeDeferredVisualInputs(identity, visual, Word);
    if (!pose || !pose->model || pose->model_generation != identity.model_generation || !inputs) {
      out.clear(); return false;
    }
    out.push_back(*inputs);
  }
  if (out.size() != requested.size()) { out.clear(); return false; }
  return true;
}

bool CollectNativeInstanceLightInputs(std::vector<NativeNodeLightBinding> &out,
    std::vector<NativeLightSourceBinding> &sources, size_t &unavailable) {
  out.clear(); sources.clear(); unavailable = 0;
  if (!REXCVAR_GET(bd_native_instances)) return false;
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  for (const auto &[visual, binding] : store.sources) {
    const auto pose = store.instances.Read(binding.instance, 1);
    if (!pose || !pose->model || pose->model_generation != binding.model_generation) {
      ++unavailable; continue;
    }
    for (uint32_t node = 0; node < pose->transforms.size(); ++node) {
      if (!FindNativeInstanceNode(*pose, node)) continue;
      const auto source = ReadNativeObjectLightSource(visual, node, Word);
      if (!source) { ++unavailable; continue; }
      if (out.size() == NativeSceneLightingPublication::kMaxBindings) { out.clear(); sources.clear(); return false; }
      if (out.size() == out.capacity())
        out.reserve(std::min(NativeSceneLightingPublication::kMaxBindings,
            std::max(size_t(64), out.size()*2)));
      out.push_back({binding.instance, binding.model_generation, node, source->inputs});
      if (source->selection) {
        if (sources.size() == sources.capacity())
          sources.reserve(std::min(NativeSceneLightingPublication::kMaxBindings,
              std::max(size_t(64), sources.size()*2)));
        sources.push_back({source->selection,binding.instance,binding.model_generation,node});
      }
    }
  }
  return true;
}

std::shared_ptr<const NativeInstancePose> FindNativeInstancePose(
    uint32_t visual, uint32_t graph, uint32_t palette) {
  if (!REXCVAR_GET(bd_native_instances) || !visual || !graph || !palette) return {};
  const auto generation = LoadedNativeModelGeneration(graph);
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  const auto it = store.sources.find(visual);
  std::shared_ptr<const NativeInstancePose> pose;
  if (it != store.sources.end())
    pose = instance_source::Find(store.instances, it->second, generation, palette);
  if (!pose && REXCVAR_GET(bd_native_materials_verify) && store.miss_examples < 4) {
    const bool found = it != store.sources.end();
    BD_INFO("[native-instance-miss] visual {:08X} graph {:08X} generation {} indexed {} "
            "stored generation {} palette {:08X} stored {:08X}/{:08X}",
        visual, graph, generation, found, found ? it->second.model_generation : 0,
        palette, found ? it->second.palettes[0] : 0, found ? it->second.palettes[1] : 0);
    ++store.miss_examples;
  }
  if (pose && REXCVAR_GET(bd_native_materials_verify)) {
    // Independent consumer-time check catches missed producer/lane updates.
    bool same = Range(palette, pose->transforms.size() * sizeof(RenderMatrix));
    if (same) {
      const auto *source = bd::mem::at<const be_f32>(palette);
      for (size_t m = 0; m < pose->transforms.size(); ++m)
        for (size_t k = 0; k < 16; ++k)
          same &= pose->transforms[m][k] == float(source[m * 16 + k]);
    }
    ++store.checked;
    if (!same) {
      if (store.drift_examples < 3) {
        BD_INFO("[native-instance-drift] at consumer visual {:08X} graph {:08X} palette {:08X}",
            visual, graph, palette);
        ++store.drift_examples;
      }
      ++store.wrong; pose.reset();
    }
  }
  ++(pose ? store.reads : store.unavailable);
  Report(store);
  return pose;
}

std::shared_ptr<const NativeInstancePose> ResolveNativeRenderPose(const NativeInstancePose &completed) {
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  const auto source = store.instances.Read(completed.instance, 1);
  if (source.get() != &completed) return {};
  const auto frame = FrameStatFrameCount();
  // Freeze one host phase across all instances and scene/shadow views of a
  // rendered frame. Authored endpoints still advance only at their handoff.
  if (!store.render_phase || store.render_phase->frame != frame)
    store.render_phase = NativePosePhase{bd::engine::TickCount(), frame,
        bd::engine::Alpha(), bd::engine::InterpolationActive()};
  return store.instances.ReadRender(source, *store.render_phase);
}

bool CopyNativeInstanceWorld(const NodeTag &tag, float out[16]) {
  if (tag.from_list || !tag.ctx_va ||
      uint64_t(tag.palette_va) + uint64_t(tag.node_index) * 64 != tag.matrix_va) return false;
  const auto pose = FindNativeInstancePose(tag.visual_va,
      bd::mem::try_load<uint32_t>(tag.ctx_va + 4), tag.palette_va);
  if (!pose || tag.node_index >= pose->transforms.size()) return false;
  std::copy_n(pose->transforms[tag.node_index].begin(), 16, out);
  return true;
}

namespace {
void RouteRequire(bool valid, const char *reason) {
  if (valid) return;
  BD_ERROR("[native-rigid-hard-off] refused: {}; no interpreter/capture/replay fallback", reason);
  throw std::runtime_error(reason);
}
bool HardOffEnabled() {
  if (!REXCVAR_GET(bd_native_rigid_hard_off)) return false;
  RouteRequire(REXCVAR_GET(bd_native_rigid_scene) && REXCVAR_GET(bd_native_rigid_shadow) &&
      REXCVAR_GET(bd_host_walk) && REXCVAR_GET(bd_native_instances),
      "hard-off requires native scene, shadow, host walk and instance producers");
  return true;
}
}

std::shared_ptr<const NativeModelRenderData> LoadNativeRigidRouteModel(uint32_t context) {
  if (!HardOffEnabled()) return {};
  const auto graph = Word(uint64_t(context) + offsetof(GuestTraverseCtx, sceneGraph));
  const auto model = graph ? FindLoadedNativeModel(*graph) : nullptr;
  RouteRequire(bool(model), "load-owned traversal model unavailable");
  return model;
}

void RequireNativeRigidWalkNode(const std::shared_ptr<const NativeModelRenderData> &model,
    const NativeInstancePose *pose, uint32_t node, uint32_t view,
    const std::optional<PrimitivePolicyInputs> &inputs) {
  if (!HardOffEnabled()) return;
  const auto decision = PrepareNativeRigidRoute(model, pose, node, view, inputs);
  RouteRequire(decision.route != NativeRigidRoute::Refused, decision.refusal);
  if (decision.route == NativeRigidRoute::Legacy) return;
  // Keep the selected-object regression window independent of family growth.
  if (!SelectedNativeRigidShadow(*model->FindNode(node))) return;
  // Fixed-size counters, no lifetime index or retained templates. This proves
  // admission ran before culling; actual GPU emissions remain a separate gate.
  thread_local uint64_t scene = 0, shadow = 0;
  thread_local uint32_t reported = 0;
  thread_local bool first = true;
  thread_local uint64_t reported_generation = 0;
  ++(decision.route == NativeRigidRoute::Scene ? scene : shadow);
  const auto frame = FrameStatFrameCount();
  if (first || reported_generation != model->Generation() || frame - reported >= 300) {
    BD_INFO("[native-rigid-hard-off] frame {} scene checks {} shadow checks {}; node {} generation {}; load-owned admission before culling, legacy entry disabled",
        frame, scene, shadow, node, model->Generation());
    first = false; reported = frame;
    reported_generation = model->Generation();
  }
}

void RequireNativeRigidLegacyNode(uint32_t context, uint32_t mesh) {
  if (!HardOffEnabled()) return;
  const auto graph = Word(uint64_t(context) + offsetof(GuestTraverseCtx, sceneGraph));
  const auto owned = graph ? FindLoadedNativeModelMaterials(*graph, mesh) : nullptr;
  RouteRequire(bool(owned), "legacy node cannot be classified from its loaded model");
  const auto view = Word(kRenderViewIdVa);
  RouteRequire(view.has_value(), "legacy render view unavailable");
  const auto visual = Word(context);
  const auto inputs = (*view == 1 || *view == 3) && visual ? ReadPrimitivePolicyInputs(context, *visual, Word) : std::nullopt;
  RouteRequire(NativeRigidLegacyAllowed(owned.get(), *view, inputs), "native family reached legacy node entry");
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(bdVisualObjectInitBones) {
  const uint32_t visual = ctx.r3.u32;
  bd::gpu::scene::SkeletonEvaluationScope evaluation(visual);
  __imp__bdVisualObjectInitBones(ctx, base);
  if (!REXCVAR_GET(bd_native_instances)) {
    bd::gpu::scene::Retire(visual); return;
  }
  try {
    bd::gpu::scene::Attach(visual);
    bd::gpu::scene::PublishEvaluatedPose(evaluation);
  }
  catch (const std::exception &error) {
    bd::gpu::scene::Retire(visual);
    BD_WARN("[native-instances] producer publication failed: {}", error.what());
  }
}

REX_HOOK_RAW(bdBoneInitSkinned) {
  if (REXCVAR_GET(bd_native_instances) && REXCVAR_GET(bd_native_skeleton)) {
    if (bd::gpu::scene::EvaluateOwnedBones(ctx,base)) return;
  }
  __imp__bdBoneInitSkinned(ctx,base);
}

REX_HOOK_RAW(sub_82140DF8) {
  // Full model-unload entry releases both palettes and the shared model, and is
  // called by the base destructor sub_8213FDD0 as well as reload paths.
  bd::gpu::scene::Retire(ctx.r3.u32);
  __imp__sub_82140DF8(ctx, base);
}

REX_HOOK_RAW(sub_8213F5E8) {
  const uint32_t container = ctx.r3.u32;
  // The derived container's actual vtable points here, not to the ungated
  // base helper sub_82144D10. Only state 3 copies and resets the dirty word.
  const bool copied = bd::gpu::scene::instance_source::TransferReady(
      bd::gpu::scene::Word(uint64_t(container) + 28));
  __imp__sub_8213F5E8(ctx, base);
  if (copied && REXCVAR_GET(bd_native_instances)) {
    try { bd::gpu::scene::Handoff(container); }
    catch (const std::exception &error) {
      if (container >= bd::gpu::scene::kVisualBoneContainer)
        bd::gpu::scene::Retire(container - bd::gpu::scene::kVisualBoneContainer);
      BD_WARN("[native-instances] render publication failed: {}", error.what());
    }
  }
}
