/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_rigid_draw.h"
#include "gpu/scene/native_rigid_shadow.h"
#include "gpu/scene/native_rigid_scene.h"
#include "gpu/scene/native_rigid_batch.h"
#include "gpu/scene/native_rigid_lifecycle_bridge.h"
#include "gpu/scene/native_rigid_program.h"
#include "gpu/scene/native_water_program.h"
#include "gpu/scene/native_sampler_bridge.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/native_lighting_bridge.h"
#include "gpu/device.h"
#include "gpu/draw_queue.h"
#include "gpu/frame_stats.h"
#include "gpu/native_depth_visibility.h"
#include "gpu/host_upload.h"
#include "gpu/sampler_cache.h"
#include "gpu/pipeline/pipeline_cache.h"
#include "core/logging.h"
#include "core/settings.h"
#include <rex/cvar.h>
#include <cstring>
#include <stdexcept>
#if !defined(REBLUE_D3D12)
#include <plume_vulkan.h>
#endif

REXCVAR_DECLARE(bool, bd_occlusion_cull);
REXCVAR_DECLARE(bool, bd_occlusion_diag);
REXCVAR_DEFINE_BOOL(bd_native_rigid_shadow, false, kCvarGroup,
    "Fail-closed native opaque and phase1 cutout rigid caster families; no template warm-up.");
REXCVAR_DEFINE_BOOL(bd_native_rigid_scene, false, kCvarGroup,
    "Fail-closed native mono opaque/cutout rigid scene families with zero-to-three texture layers; no node interpreter/template warm-up.");
REXCVAR_DEFINE_BOOL(bd_native_rigid_deferred, false, kCvarGroup,
    "Connect ordinary rigid sorted packets to the mixed native consumer; pending live qualification.");
namespace bd::gpu::scene {
struct NativeRigidDrawStore {
  struct Batch {
    std::unique_ptr<plume::RenderDescriptorSet> constants;
    std::unique_ptr<plume::RenderDescriptorSet> images, samplers;
    std::vector<const NativeRigidBatchItem *> items;
    uint32_t visibility_work = ~0u, visibility_command = 0;
  };
  struct Visibility {
    std::unique_ptr<NativeDepthVisibilityWork> work;
    NativeTargetImageHandle depth;
    NativeOcclusionView view;
    const plume::RenderCommandList *recording = nullptr;
    bool sealed = false;
    std::span<const NativeDepthVisibilityReceipt> receipts;
  };
  struct Program { NativeVertexInputHandle input; NativeRigidPrograms shaders; NativePipelineHandle water; };
  // Immutable program variants remain bounded; per-draw descriptors/geometry
  // survive exactly their recording slot's fence. Upload pages have their own
  // shared bounded arena and are never read back as a CPU data source.
  std::vector<Program> programs;
  std::array<std::vector<std::shared_ptr<const NativeRigidBatchItem>>, kNumFrames> records;
  std::array<std::vector<Batch>, kNumFrames> batches;
  NativeDepthVisibilityProgramHandle visibility_program;
  std::shared_ptr<NativeDepthVisibilityBudget> visibility_budget = std::make_shared<NativeDepthVisibilityBudget>();
  std::array<std::vector<Visibility>, kNumFrames> visibility;
  std::array<NativeRigidInstanceGPU, kNativeRigidBatchLimit> scratch;
  std::array<NativeWaterInstanceGPU, kNativeRigidBatchLimit> water_scratch;
  uint64_t water_submitted = 0, water_emitted = 0, water_culled = 0, water_retired = 0;
  uint32_t water_reported_frame = 0;
  uint64_t submitted = 0, suppressed = 0, retired = 0;
  uint32_t reported_frame = 0;
  uint64_t emitted = 0, scene_submitted = 0, scene_emitted = 0, scene_retired = 0, scene_suppressed = 0;
  uint32_t scene_reported_frame = 0;
  uint64_t scene_batches = 0, shadow_batches = 0, merged_instances = 0;
  uint64_t reported_generation = 0, scene_reported_generation = 0;
  uint64_t family_nodes = 0, family_multi_nodes = 0, family_primitives = 0;
  uint64_t family_emitted = 0, family_retired = 0;
  uint64_t scene_family_multi_nodes = 0, scene_family_primitives = 0, scene_family_layered = 0;
  uint64_t scene_family_emitted = 0, scene_family_retired = 0;
  uint64_t scene_culled = 0, scene_retired_culled = 0, scene_resource_retired = 0;
  uint64_t visibility_commands = 0, visibility_drawn = 0, visibility_collected = 0, visibility_snapshots = 0;
  uint64_t visibility_visible = 0, visibility_culled = 0;
  struct Cutouts {
    uint64_t submitted = 0, emitted = 0, retired = 0, textured_emitted = 0, textured_retired = 0;
  } scene_cutouts, shadow_cutouts;
};
namespace {
void Require(bool valid, const char *reason) {
  if (valid) return;
  BD_ERROR("[native-rigid-shadow] admitted node refused: {}; interpreter/capture/replay remain disabled", reason);
  throw std::runtime_error(reason);
}
void BatchRequire(bool valid, const char *reason) {
  if (valid) return;
  BD_ERROR("[native-rigid-batch] refused: {}; no translated-record fallback", reason);
  throw std::runtime_error(reason);
}
void StageNativeItem(QueuedDraw &draw, std::shared_ptr<NativeRigidBatchItem> item) {
  item->pipeline = draw.pipeline; item->layout = draw.bindings.layout; item->framebuffer = draw.framebuffer;
  item->viewport = draw.viewport; item->scissor = draw.scissor; item->view = draw.render_view;
  item->frame = FrameStatFrameCount(); item->slot = Video::CurrentFrameSlot();
  BatchRequire(item->Ready(item->frame,item->slot), "incomplete native instance");
  state().native_rigid_draws->records[item->slot].push_back(item);
  if (item->Cutout()) {
    auto &store = *state().native_rigid_draws;
    ++(item->view == 3 ? store.scene_cutouts : store.shadow_cutouts).submitted;
  }
  if (item->regression) NoteNativeRigidSubmitted(item->model_generation,item->instance,item->view);
  draw.native_rigid = std::move(item);
}
}
bool NativeRigidShadowEnabled() { return REXCVAR_GET(bd_native_rigid_shadow); }
bool NativeRigidSceneEnabled() { return REXCVAR_GET(bd_native_rigid_scene); }
bool NativeRigidDeferredEnabled() { return NativeRigidSceneEnabled() && REXCVAR_GET(bd_native_rigid_deferred); }
bool SubmitNativeRigidShadow(const NativeInstancePose &pose, uint32_t node,
                             const std::optional<PrimitivePolicyInputs> &inputs) {
  if (!NativeRigidShadowEnabled()) return false;
  const auto *model = FindNativeInstanceNode(pose, node);
  if (!model) return false;
  const auto admission = PrepareNativeRigidShadowAdmission(*model, inputs);
  if (admission.route == NativeRigidCasterRoute::Legacy) return false;
  Require(admission.route == NativeRigidCasterRoute::Native, "caster family or object pass policy unavailable");
  const auto camera = FindNativePassCamera(1);
  Require(camera.has_value(), "fresh shadow pass camera unavailable");
  Require(node < pose.transforms.size(), "native caster transform unavailable");
  const bool cutouts = std::any_of(admission.policies.begin(), admission.policies.end(),
      [](const auto &policy) { return policy.alpha_test; });
  const char *refusal = "whole-node caster resources or matrices unavailable";
  const auto plans = cutouts ? PrepareNativeRigidShadowForObject(pose, node, *camera, refusal)
      : PrepareNativeRigidShadow(*model, pose.transforms[node], *inputs, *camera);
  if (!plans) BD_ERROR("[native-shadow-packet] instance {} generation {} node {} phase {} cutouts {} ranges {}; {}",
      pose.instance, pose.model_generation, node, inputs->phase, cutouts, model->ranges.size(), refusal);
  Require(plans.has_value(), refusal);
  auto &s = state();
  std::lock_guard lock(s.mutex);
  auto *commands = ActiveNativeShadowCommands(s.render_target ? s.render_target->texture : nullptr,
      s.depth_stencil ? s.depth_stencil->texture : nullptr);
  Require(s.ready && s.device && s.command_list_open && commands, "native shadow command scope unavailable");
  const auto current_camera = commands->Camera(FrameStatFrameCount(), 1);
  Require(current_camera && current_camera->world_to_clip == camera->world_to_clip, "shadow scope changed before submission");
  if (!s.native_rigid_draws) s.native_rigid_draws = std::make_shared<NativeRigidDrawStore>();
  auto &store = *s.native_rigid_draws;
  auto &records = store.records[Video::CurrentFrameSlot()];
  Require(records.size() <= 4096 && plans->size() <= 4096-records.size(), "native draw retention capacity reached");
  struct Pending { QueuedDraw draw; std::shared_ptr<NativeRigidBatchItem> item; };
  std::vector<Pending> pending;
  pending.reserve(plans->size());
  // All CPU inputs, pipelines and bindings must pass before the first sibling
  // reaches the shared queue. Failure cannot leave a partially replaced node.
  for (const auto &plan : *plans) {
    if (!plan.draw) continue;
    const auto &geometry = plan.geometry;
    auto program = std::find_if(store.programs.begin(), store.programs.end(), [&](const auto &p) {
      return p.input == geometry->rigid_vertex_input;
    });
    if (program == store.programs.end()) {
      Require(store.programs.size() < 8, "native program capacity reached");
      auto shaders = CreateNativeRigidPrograms(*s.device, geometry->rigid_vertex_input);
      Require(shaders.shadow && shaders.scene && shaders.shadow_cutout, "native shader creation failed");
      store.programs.push_back({geometry->rigid_vertex_input, std::move(shaders)});
      program = store.programs.end() - 1;
    }
    PipelineState pipeline_state;
    const auto &shader = plan.albedo ? program->shaders.shadow_cutout : program->shaders.shadow;
    pipeline_state.native_program = shader.get();
    pipeline_state.vertexStrides[0] = uint8_t(geometry->strides[0]);
    pipeline_state.renderTargetFormat = plume::RenderFormat::UNKNOWN;
    pipeline_state.depthStencilFormat = plume::RenderFormat::D32_FLOAT_S8_UINT;
    pipeline_state.colorWriteEnable = 0;
    pipeline_state.cullMode = plan.cull == PrimitiveCull::Back ? plume::RenderCullMode::BACK :
        plan.cull == PrimitiveCull::Front ? plume::RenderCullMode::FRONT : plume::RenderCullMode::NONE;
    SanitizePipelineState(pipeline_state);
    auto *pipeline = GetOrCreatePipeline(pipeline_state);
    Require(pipeline != nullptr, "native shadow pipeline unavailable");
    QueuedDraw draw;
    draw.pipeline = pipeline;
    draw.bindings.layout = shader->Layout();
    draw.vertex_views[0] = geometry->streams[0];
    draw.input_slots[0] = plume::RenderInputSlot(0, geometry->strides[0]);
    draw.vertex_count = 1; draw.index_view = geometry->index;
    draw.has_index_buffer = draw.indexed = true;
    draw.count = geometry->count; draw.start_index = geometry->start_index; draw.base_vertex = geometry->base_vertex;
    draw.framebuffer = commands->Framebuffer();
    const auto width = draw.framebuffer->getWidth(), height = draw.framebuffer->getHeight();
    draw.viewport = plume::RenderViewport(0, 0, float(width), float(height));
    draw.scissor = plume::RenderRect(0, 0, width, height); draw.has_viewport = true;
    draw.render_view = 1; draw.zwrite = true; draw.reorderable = true;
    Require(draw.bindings.Valid(), "invalid native descriptor contract");
    auto item = std::make_shared<NativeRigidBatchItem>();
    item->geometry = geometry; item->input = {plan.object,plan.pass};
    item->albedo[0] = plan.albedo;
    if (plan.albedo) {
      Require(plan.sampler.has_value(), "native cutout sampler unavailable");
      item->albedo_samplers[0] = ResolveSamplerLocked(MaterialSamplerDesc(*plan.sampler));
      Require(item->albedo_samplers[0] != nullptr, "native cutout sampler creation failed");
    }
    item->model_generation = pose.model_generation; item->instance = pose.instance;
    item->regression = geometry->id == 0x258694267A8DBAEEull;
    pending.push_back({std::move(draw), std::move(item)});
  }
  store.suppressed += plans->size()-pending.size();
  if (!pending.empty() && !s.draw_framebuffer_bound) {
    DrawQueueFlush(s.command_list);
    BindNativeSceneCommands(s, *commands); ApplyNativeSceneClear(s, *commands);
    s.draw_framebuffer_bound = true;
  }
  for (auto &entry : pending) {
    store.family_primitives += !entry.item->regression;
    StageNativeItem(entry.draw, std::move(entry.item));
    DrawQueuePush(entry.draw);
  }
  if (!DrawQueueEnabled()) DrawQueueFlush(s.command_list);
  store.submitted += pending.size();
  if (!SelectedNativeRigidShadow(*model)) {
    ++store.family_nodes;
    store.family_multi_nodes += plans->size() > 1;
  }
  const auto frame = FrameStatFrameCount();
  if (SelectedNativeRigidShadow(*model) &&
      (store.reported_generation != pose.model_generation || frame - store.reported_frame >= 300)) {
    BD_INFO("[native-rigid-shadow] frame {} submitted {} suppressed {} fence-retired {}; node {} instance {} generation {} phase {}; native program and owned matrices, no interpreter/template/replay",
        frame, store.submitted, store.suppressed, store.retired, node, pose.instance, pose.model_generation, inputs->phase);
    BD_INFO("[native-caster-family] frame {} nodes {} multi-primitive nodes {} submitted {} emitted {} fence-retired {}; excludes selected regression; whole-node rigid admission",
        frame, store.family_nodes, store.family_multi_nodes, store.family_primitives, store.family_emitted, store.family_retired);
    store.reported_frame = frame;
    store.reported_generation = pose.model_generation;
  }
  return true;
}
bool SubmitNativeRigidScene(const NativeInstancePose &pose, uint32_t node,
                            const std::optional<PrimitivePolicyInputs> &inputs, uint32_t stack) {
  if (!NativeRigidSceneEnabled()) return false;
  const auto *model = FindNativeInstanceNode(pose, node);
  if (!model) return false;
  const auto admission = PrepareNativeRigidSceneAdmission(*model, inputs, NativeRigidDeferredEnabled());
  if (admission.route == NativeRigidCasterRoute::Legacy) return false;
  Require(admission.route == NativeRigidCasterRoute::Native, "scene family or object pass policy unavailable");
  const char *refusal = "native scene object preparation failed";
  auto plans = PrepareNativeRigidSceneForObject(pose, node, refusal);
  Require(plans.has_value() && !plans->empty(), refusal);
  NativeRigidSceneSubmission submission{pose.instance, pose.model_generation, node,
      FrameStatFrameCount(), SelectedNativeRigidShadow(*model), std::move(*plans)};
  Require(StageNativeRigidSceneDeferredForObject(submission), "native deferred staging refused");
  if (submission.plans.empty()) return true;
  return SubmitNativeRigidScenePackets(std::move(submission), stack);
}
bool SubmitNativeRigidScenePackets(NativeRigidSceneSubmission submission, uint32_t stack) {
  const auto require = [](bool valid, const char *reason) {
    if (valid) return;
    BD_ERROR("[native-rigid-scene] admitted packet refused: {}; interpreter/capture/replay remain disabled", reason);
    throw std::runtime_error(reason);
  };
  auto &plans = submission.plans;
  require(NativeRigidSceneEnabled() && submission.instance && submission.model_generation &&
      submission.frame == FrameStatFrameCount() && !plans.empty() && plans.size() <= 4096,
      "stale or incomplete owned scene submission");
  // No active object scope, pose, source model, shader-register import or camera
  // lookup survives this boundary. Resolve Keep at consumption, not at capture.
  require(plans.front().light_recipe.has_value(), "owned light action unavailable");
  const auto ticket = ResolveNativeSceneLights(*plans.front().light_recipe);
  require(ticket && FinalizeNativeRigidSceneLights(plans, *ticket), "ordered whole-node light finalization unavailable");
  const bool draws = std::any_of(plans.begin(), plans.end(), [](const auto &plan) { return plan.draw; });
  require(!draws || CommitNativeSceneLights(*ticket, stack), "outgoing compatibility light publication unavailable");
  auto &s = state();
  std::lock_guard lock(s.mutex);
  auto *commands = ActiveNativeSceneCommands(s.render_target ? s.render_target->texture : nullptr,
      s.depth_stencil ? s.depth_stencil->texture : nullptr);
  require(s.ready && s.device && s.command_list_open && commands, "native scene command scope unavailable");
  const auto *shape = commands->ColorShape();
  const auto current_camera = commands->Camera(FrameStatFrameCount(), 3);
  require(shape && shape->layers == 1 && current_camera, "mono native scene scope/camera required");
  for (const auto &plan : plans) {
    require(plan.geometry && plan.vertex_input && plan.light_ticket,
        "incomplete owned scene geometry or finalized lighting");
    for (const auto &eye : plan.pass.world_to_clip)
      require(std::memcmp(&eye, current_camera->world_to_clip.data(), sizeof(RenderMatrix)) == 0,
          "scene camera changed before submission");
  }
  if (!s.native_rigid_draws) s.native_rigid_draws = std::make_shared<NativeRigidDrawStore>();
  auto &store = *s.native_rigid_draws;
  auto &records = store.records[Video::CurrentFrameSlot()];
  require(records.size() <= 4096 && plans.size() <= 4096-records.size(), "native draw retention capacity reached");
  struct Pending {
    QueuedDraw draw;
    std::shared_ptr<NativeRigidBatchItem> item;
  };
  std::vector<Pending> pending;
  pending.reserve(plans.size());
  // Preflight the complete node, including every pipeline and sampled owner,
  // before any sibling is submitted. The existing queue/fence retains all layers.
  for (const auto &plan : plans) {
    if (!plan.draw) continue;
    const auto &geometry = plan.geometry;
    auto program = std::find_if(store.programs.begin(), store.programs.end(), [&](const auto &p) {
      return p.input == plan.vertex_input;
    });
    if (program == store.programs.end()) {
      require(store.programs.size() < 8, "native program capacity reached");
      auto shaders = CreateNativeRigidPrograms(*s.device, plan.vertex_input);
      require(shaders.scene && shaders.shadow, "native shader creation failed");
      store.programs.push_back({plan.vertex_input, std::move(shaders)});
      program = store.programs.end()-1;
    }
    PipelineState pipeline_state;
    pipeline_state.native_program = program->shaders.scene.get();
    pipeline_state.vertexStrides[0] = uint8_t(geometry->strides[0]);
    pipeline_state.renderTargetFormat = shape->format;
    pipeline_state.depthStencilFormat = plume::RenderFormat::D32_FLOAT_S8_UINT;
    pipeline_state.sampleCount = static_cast<plume::RenderSampleCounts>(shape->samples);
    pipeline_state.zWriteEnable = plan.depth_write;
    bool blend_dirty = false;
    ApplyBlendState(plan.blend, pipeline_state, blend_dirty);
    pipeline_state.enableAlphaToCoverage = plan.alpha_to_coverage && shape->samples > 1;
    pipeline_state.cullMode = plan.cull == PrimitiveCull::Back ? plume::RenderCullMode::BACK :
        plan.cull == PrimitiveCull::Front ? plume::RenderCullMode::FRONT : plume::RenderCullMode::NONE;
    SanitizePipelineState(pipeline_state);
    auto *pipeline = GetOrCreatePipeline(pipeline_state);
    require(pipeline != nullptr, "native scene pipeline unavailable");
    auto item = std::make_shared<NativeRigidBatchItem>();
    for (uint32_t layer = 0; layer < 3; ++layer) {
      // Inactive slots are bound legally but never sampled. Their placeholder
      // descriptor is not a material image and cannot make a missing layer owned.
      const auto sampler = layer < plan.object.flags.y ? MaterialSamplerDesc(plan.samplers[layer]) : plume::RenderSamplerDesc{};
      item->albedo_samplers[layer] = ResolveSamplerLocked(sampler);
      require(item->albedo_samplers[layer] != nullptr, "native layer sampler unavailable");
    }
    plume::RenderSamplerDesc sampler;
    sampler.minFilter = sampler.magFilter = plume::RenderFilter::LINEAR;
    sampler.mipmapMode = plume::RenderMipmapMode::NEAREST;
    sampler.addressU = sampler.addressV = sampler.addressW = plume::RenderTextureAddressMode::CLAMP;
    sampler.comparisonEnabled = true; sampler.comparisonFunc = plume::RenderComparisonFunction::LESS_EQUAL;
    item->shadow_sampler = ResolveSamplerLocked(sampler);
    require(item->shadow_sampler != nullptr, "native shadow sampler unavailable");
    QueuedDraw draw;
    draw.pipeline = pipeline; draw.bindings.layout = program->shaders.scene->Layout();
    draw.vertex_views[0] = geometry->streams[0]; draw.input_slots[0] = plume::RenderInputSlot(0, geometry->strides[0]);
    draw.vertex_count = 1; draw.index_view = geometry->index; draw.has_index_buffer = draw.indexed = true;
    draw.count = geometry->count; draw.start_index = geometry->start_index; draw.base_vertex = geometry->base_vertex;
    draw.framebuffer = commands->Framebuffer();
    draw.viewport = plume::RenderViewport(0, 0, float(shape->width), float(shape->height));
    draw.scissor = plume::RenderRect(0, 0, shape->width, shape->height); draw.has_viewport = true;
    draw.render_view = 3; draw.zwrite = plan.depth_write;
    // Blended cutouts keep their authored ordering and depth writes. They are
    // not opaque reorder candidates merely because their alpha also discards.
    draw.reorderable = !plan.blend.alphaBlendEnable;
    require(draw.bindings.Valid(), "invalid native scene descriptor contract");
    item->geometry = geometry; item->input = {plan.object,plan.pass};
    item->model_generation = submission.model_generation; item->instance = submission.instance;
    item->regression = geometry->id == 0x258694267A8DBAEEull;
    item->albedo = plan.albedo; item->shadow = plan.shadow;
    item->scene_depth = commands->DepthOwner();
    item->world_bounds = geometry->bounds
        ? TransformNativeBounds(*geometry->bounds,std::bit_cast<RenderMatrix>(plan.object.world)) : std::nullopt;
    pending.push_back({std::move(draw),std::move(item)});
  }
  store.scene_suppressed += plans.size()-pending.size();
  // Every authored effect and sibling preflight still runs. Visibility is now
  // decided from current depth at emission, not previous-frame query history.
  if (!pending.empty() && !s.draw_framebuffer_bound) {
    DrawQueueFlush(s.command_list);
    BindNativeSceneCommands(s, *commands); ApplyNativeSceneClear(s, *commands); s.draw_framebuffer_bound = true;
  }
  bool family_multi = pending.size() > 1 && !submission.regression_node;
  for (auto &entry : pending) {
    if (!entry.item->regression) {
      ++store.scene_family_primitives;
      store.scene_family_layered += entry.item->input.object_data.flags.y > 1;
    }
    StageNativeItem(entry.draw,std::move(entry.item));
    DrawQueuePush(entry.draw);
  }
  if (!DrawQueueEnabled()) DrawQueueFlush(s.command_list);
  store.scene_submitted += pending.size();
  store.scene_family_multi_nodes += family_multi;
  const auto frame = FrameStatFrameCount();
  if (submission.regression_node &&
      (store.scene_reported_generation != submission.model_generation || frame-store.scene_reported_frame >= 300)) {
    BD_INFO("[native-rigid-scene] frame {} submitted {} emitted {} suppressed {} fence-retired {}; node {} instance {} generation {}; visibility pending {} culled {} retired-culled {} resources-retired {}; owned packet and native program, no node interpreter/template/replay",
        frame, store.scene_submitted, store.scene_emitted, store.scene_suppressed, store.scene_retired,
        submission.node, submission.instance, submission.model_generation, store.scene_submitted-store.scene_emitted-store.scene_culled,
        store.scene_culled,store.scene_retired_culled,store.scene_resource_retired);
    BD_INFO("[native-rigid-batch] frame {} scene instances {} visible indirect calls {} shadow instances {} visible indirect calls {} merged instances {}; native storage records, no translated gather",
        frame, store.scene_emitted, store.scene_batches, store.emitted, store.shadow_batches, store.merged_instances);
    if (REXCVAR_GET(bd_occlusion_diag)) {
      const auto used = store.visibility_budget->Used();
      BD_INFO("[native-depth-vis] frame {} snapshots {} generated {} draw-recorded {} fence-collected {} visible-instances {} culled-instances {}; buffers {} owners {}; current depth, no temporal history",
          frame,store.visibility_snapshots,store.visibility_commands,store.visibility_drawn,store.visibility_collected,
          store.visibility_visible,store.visibility_culled,used.bytes,used.owners);
    }
    BD_INFO("[native-scene-family] frame {} multi-primitive nodes {} submitted {} layered {} emitted {} fence-retired {}; excludes selected regression",
        frame, store.scene_family_multi_nodes, store.scene_family_primitives, store.scene_family_layered,
        store.scene_family_emitted, store.scene_family_retired);
    const auto &scene = store.scene_cutouts;
    const auto &shadow = store.shadow_cutouts;
    BD_INFO("[native-cutout-family] frame {} scene submitted {} emitted {} fence-retired {} textured-emitted {} textured-retired {}; shadow submitted {} emitted {} fence-retired {} textured-emitted {} textured-retired {}; cutout instances only",
        frame, scene.submitted, scene.emitted, scene.retired, scene.textured_emitted, scene.textured_retired,
        shadow.submitted, shadow.emitted, shadow.retired, shadow.textured_emitted, shadow.textured_retired);
    store.scene_reported_frame = frame;
    store.scene_reported_generation = submission.model_generation;
  }
  return true;
}
bool SubmitNativeWaterScenePackets(NativeWaterSceneSubmission submission) {
  BatchRequire(NativeRigidSceneEnabled() && submission.instance && submission.model_generation &&
      submission.frame == FrameStatFrameCount() && !submission.plans.empty() && submission.plans.size() <= 4096,
      "stale or incomplete owned water submission");
  auto &s = state();
  std::lock_guard lock(s.mutex);
  auto *commands = ActiveNativeSceneCommands(s.render_target ? s.render_target->texture : nullptr,
      s.depth_stencil ? s.depth_stencil->texture : nullptr);
  BatchRequire(s.ready && s.device && s.command_list_open && commands, "native water scene scope unavailable");
  const auto *shape = commands->ColorShape();
  const auto camera = commands->Camera(submission.frame,3);
  BatchRequire(shape && shape->layers == 1 && camera, "live stereo water camera producer remains unconnected");
  if (!s.native_rigid_draws) s.native_rigid_draws = std::make_shared<NativeRigidDrawStore>();
  auto &store = *s.native_rigid_draws;
  auto &records = store.records[Video::CurrentFrameSlot()];
  BatchRequire(records.size() <= 4096 && submission.plans.size() <= 4096-records.size(), "water draw retention capacity");
  struct Pending { QueuedDraw draw; std::shared_ptr<NativeRigidBatchItem> item; };
  std::vector<Pending> pending;
  pending.reserve(submission.plans.size());
  for (const auto &plan : submission.plans) {
    const auto &geometry = plan.geometry;
    BatchRequire(geometry && geometry->id && geometry->canonical_vertices && geometry->water_vertex_input &&
        geometry->stream_mask == 1 && geometry->strides[0] && geometry->strides[0] <= 255 &&
        geometry->streams[0].buffer.ref && geometry->index.buffer.ref && geometry->count,
        "load-owned water geometry or tangent unavailable");
    const auto bounds = NativeWaterWorldBounds(*geometry,plan.input);
    BatchRequire(bounds.has_value() && plan.images.Ready(plan.input.image_layers), "water wave bounds or image ownership unavailable");
    for (uint32_t role = 0; role < 6; ++role)
      BatchRequire(plan.samplers[role].comparisonEnabled == (role == 5), "water comparison sampler role mismatch");
    for (const auto &eye : plan.input.pass_data.world_to_clip)
      BatchRequire(std::memcmp(&eye,camera->world_to_clip.data(),sizeof(RenderMatrix)) == 0,
          "water camera changed before ordered submission");
    for (uint32_t role = 0; role < 6; ++role)
      BatchRequire(!commands->WritesImage(plan.images.Image(role)),
          "water sampled image aliases an active attachment or resolve");
    auto program = std::find_if(store.programs.begin(),store.programs.end(),[&](const auto &entry) {
      return entry.water && entry.input == geometry->water_vertex_input;
    });
    if (program == store.programs.end()) {
      BatchRequire(store.programs.size() < 8, "native program capacity reached");
      auto shader = CreateNativeWaterProgram(*s.device,geometry->water_vertex_input);
      BatchRequire(bool(shader), "native water shader creation failed");
      store.programs.push_back({geometry->water_vertex_input,{},std::move(shader)});
      program = store.programs.end()-1;
    }
    PipelineState pipeline_state;
    pipeline_state.native_program = program->water.get();
    pipeline_state.vertexStrides[0] = uint8_t(geometry->strides[0]);
    pipeline_state.renderTargetFormat = shape->format;
    pipeline_state.depthStencilFormat = plume::RenderFormat::D32_FLOAT_S8_UINT;
    pipeline_state.sampleCount = static_cast<plume::RenderSampleCounts>(shape->samples);
    pipeline_state.zWriteEnable = plan.depth_write;
    pipeline_state.cullMode = plan.cull;
    bool blend_dirty = false;
    ApplyBlendState(plan.blend,pipeline_state,blend_dirty);
    SanitizePipelineState(pipeline_state);
    auto *pipeline = GetOrCreatePipeline(pipeline_state);
    BatchRequire(pipeline != nullptr, "native water pipeline unavailable");
    auto water = std::make_shared<NativeWaterBatchData>();
    water->input = plan.input; water->images = plan.images;
    for (uint32_t role = 0; role < 6; ++role) {
      water->samplers[role] = ResolveSamplerLocked(plan.samplers[role]);
      BatchRequire(water->samplers[role] != nullptr, "native water sampler unavailable");
    }
    auto item = std::make_shared<NativeRigidBatchItem>();
    item->water = std::move(water); item->geometry = geometry; item->world_bounds = bounds;
    item->model_generation = submission.model_generation; item->instance = submission.instance;
    item->scene_depth = commands->DepthOwner();
    QueuedDraw draw;
    draw.pipeline = pipeline; draw.bindings.layout = program->water->Layout();
    draw.vertex_views[0] = geometry->streams[0]; draw.input_slots[0] = plume::RenderInputSlot(0,geometry->strides[0]);
    draw.vertex_count = 1; draw.index_view = geometry->index; draw.has_index_buffer = draw.indexed = true;
    draw.count = geometry->count; draw.start_index = geometry->start_index; draw.base_vertex = geometry->base_vertex;
    draw.framebuffer = commands->Framebuffer();
    draw.viewport = plume::RenderViewport(0,0,float(shape->width),float(shape->height));
    draw.scissor = plume::RenderRect(0,0,shape->width,shape->height); draw.has_viewport = true;
    draw.render_view = 3; draw.zwrite = plan.depth_write;
    // Water remains an ordering barrier, including ostensibly opaque variants:
    // neighbouring material scopes can publish snapshots or late authored data.
    draw.reorderable = false;
    BatchRequire(draw.bindings.Valid(), "native water descriptor layout unavailable");
    pending.push_back({std::move(draw),std::move(item)});
  }
  records.reserve(records.size()+pending.size());
  if (!s.draw_framebuffer_bound) {
    DrawQueueFlush(s.command_list);
    BindNativeSceneCommands(s,*commands); ApplyNativeSceneClear(s,*commands); s.draw_framebuffer_bound = true;
  }
  for (auto &entry : pending) {
    StageNativeItem(entry.draw,std::move(entry.item));
    DrawQueuePush(entry.draw);
  }
  store.water_submitted += pending.size();
  if (!DrawQueueEnabled()) DrawQueueFlush(s.command_list);
  if (store.water_submitted == pending.size() || submission.frame-store.water_reported_frame >= 300) {
    BD_INFO("[native-water-scene] frame {} submitted {} emitted {} culled {} fence-retired {}; instance {} generation {}; owned water packets in shared native queue",
        submission.frame,store.water_submitted,store.water_emitted,store.water_culled,store.water_retired,
        submission.instance,submission.model_generation);
    store.water_reported_frame = submission.frame;
  }
  return true;
}
void PrepareNativeRigidBatchDraw(std::span<const NativeRigidBatchItem *const> items, QueuedDraw &draw, bool refresh_depth) {
  auto &s = state();
  BatchRequire(s.native_rigid_draws && s.ready && s.device && s.command_list_open, "native batch device unavailable");
  auto &store = *s.native_rigid_draws;
  const auto slot = Video::CurrentFrameSlot();
  auto &batches = store.batches[slot];
  BatchRequire(items.size() <= kNativeRigidBatchLimit && batches.size() < 4096, "native batch capacity");
  BatchRequire(!items.empty() && items[0], "empty native batch");
  const auto &first = *items[0];
  const void *packed;
  const auto stride = uint32_t(first.water ? sizeof(NativeWaterInstanceGPU) : sizeof(NativeRigidInstanceGPU));
  if (first.water) {
    const auto output = std::span(store.water_scratch).first(items.size());
    BatchRequire(PackNativeWaterBatch(items,output,FrameStatFrameCount(),slot), "stale or incompatible water batch");
    packed = output.data();
  } else {
    const auto output = std::span(store.scratch).first(items.size());
    BatchRequire(PackNativeRigidBatch(items,output,FrameStatFrameCount(),slot), "stale or incompatible native batch");
    packed = output.data();
  }
  BatchRequire(draw.pipeline == first.pipeline && draw.bindings.layout == first.layout && draw.framebuffer == first.framebuffer &&
      draw.indexed && draw.has_index_buffer && !draw.translated_instance_records, "native queue contract changed");
  uint64_t alignment = 16;
#if !defined(REBLUE_D3D12)
  alignment = static_cast<plume::VulkanDevice &>(*s.device).physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
#endif
  BatchRequire(alignment <= 65536, "storage alignment unsupported");
  const auto placement = PlanNativeRigidStorage(uint32_t(items.size()),uint32_t(alignment),stride);
  BatchRequire(placement.has_value(), "native storage placement refused");
  const auto upload = AllocateHostUpload(placement->reserve,uint32_t(alignment));
  BatchRequire(upload.memory != nullptr, "bounded native instance upload refused");
  const auto offset = placement->Offset(upload.ref.offset);
  const auto prefix = uint32_t(offset-upload.ref.offset);
  BatchRequire(prefix <= upload.size && placement->bytes <= upload.size-prefix, "native storage slice outside upload");
  std::memcpy(upload.memory+prefix,packed,placement->bytes);
  NativeRigidDescriptorSchema schema;
  NativeRigidDrawStore::Batch batch;
  batch.items.assign(items.begin(),items.end());
  batch.constants = schema.sets[0].create(s.device.get());
  BatchRequire(bool(batch.constants), "native storage descriptor unavailable");
  const plume::RenderBufferStructuredView view(stride,offset/stride);
  batch.constants->setBuffer(0,upload.ref.ref,placement->bytes,&view);
  draw.bindings = {}; draw.bindings.layout = first.layout;
  draw.bindings.set_count = 1; draw.bindings.sets[0] = batch.constants.get();
  if (first.water) {
    NativeWaterDescriptorSchema water_schema;
    batch.images = water_schema.sets[1].create(s.device.get());
    batch.samplers = water_schema.sets[2].create(s.device.get());
    BatchRequire(batch.images && batch.samplers && BindNativeWaterImages(*first.water,*batch.images,*batch.samplers),
        "water image owners or descriptor contract unavailable");
    draw.bindings.set_count = 3; draw.bindings.sets[1] = batch.images.get(); draw.bindings.sets[2] = batch.samplers.get();
  } else if (first.view == 1 && first.albedo[0]) {
    batch.images = schema.sets[1].create(s.device.get()); batch.samplers = schema.sets[2].create(s.device.get());
    BatchRequire(batch.images && batch.samplers, "native cutout descriptors unavailable");
    // Only slot0 is sampled. Populate inactive slots from that same retained
    // material, never the active depth attachment (no read/write feedback).
    for (uint32_t layer = 0; layer < 4; ++layer) {
      batch.images->setTexture(layer,first.albedo[0]->image.get(),plume::RenderTextureLayout::SHADER_READ,first.albedo[0]->view.get());
      batch.samplers->setSampler(layer,first.albedo_samplers[0]);
    }
    draw.bindings.set_count = 3; draw.bindings.sets[1] = batch.images.get(); draw.bindings.sets[2] = batch.samplers.get();
  } else if (first.view == 3) {
    batch.images = schema.sets[1].create(s.device.get()); batch.samplers = schema.sets[2].create(s.device.get());
    BatchRequire(batch.images && batch.samplers, "native image descriptors unavailable");
    for (uint32_t layer = 0; layer < 3; ++layer) {
      const auto &image = first.albedo[layer];
      // The required shadow owner supplies valid descriptors for inactive slots.
      // Ready() forbids using this to fill an active layer's missing material.
      batch.images->setTexture(layer,image ? image->image.get() : first.shadow->image.get(),
          plume::RenderTextureLayout::SHADER_READ,image ? image->view.get() : first.shadow->view.get());
      batch.samplers->setSampler(layer,first.albedo_samplers[layer]);
    }
    batch.images->setTexture(3,first.shadow->image.get(),plume::RenderTextureLayout::SHADER_READ,first.shadow->view.get());
    batch.samplers->setSampler(3,first.shadow_sampler);
    draw.bindings.set_count = 3; draw.bindings.sets[1] = batch.images.get(); draw.bindings.sets[2] = batch.samplers.get();
  }
  const NativeRigidIndexedCommand command{draw.count,uint32_t(items.size()),draw.start_index,draw.base_vertex,0};
  if (first.view == 3 && REXCVAR_GET(bd_occlusion_cull)) {
    const auto &depth = first.scene_depth;
    BatchRequire(depth && depth->shape.layers == 1 && depth->layout == plume::RenderTextureLayout::DEPTH_WRITE,
        "current native depth is unavailable for visibility");
    NativeOcclusionView view;
    view.frame = first.frame;
    view.scope = {depth->identity,depth->shape.width,depth->shape.height,depth->shape.samples};
    view.camera.world_to_clip = std::bit_cast<RenderMatrix>(first.Pass().world_to_clip[0]);
    if (!store.visibility_program) store.visibility_program = CreateNativeDepthVisibilityProgram(*s.device);
    auto &works = store.visibility[slot];
    uint32_t selected = 0;
    while (selected < works.size() && (works[selected].depth != depth || works[selected].recording != s.command_list ||
        works[selected].view.frame != first.frame || works[selected].sealed)) ++selected;
    if (selected == works.size()) {
      BatchRequire(works.size() < 16, "visibility recording owner capacity");
      auto work = NativeDepthVisibilityWork::Create(*s.device,store.visibility_program,depth,view,store.visibility_budget);
      BatchRequire(work && work->RecordDepth(*s.command_list), "bounded current-depth visibility allocation or snapshot refused");
      works.push_back({std::move(work),depth,view,s.command_list});
      ++store.visibility_snapshots;
    } else if (refresh_depth || works[selected].view.camera.world_to_clip != view.camera.world_to_clip) {
      BatchRequire(works[selected].work->RefreshDepth(*s.command_list,view), "fresh native depth snapshot refused");
      works[selected].view = view; ++store.visibility_snapshots;
    }
    auto &work = *works[selected].work;
    batch.visibility_work = selected; batch.visibility_command = work.CommandCount();
    const auto indirect = work.RecordCommand(*s.command_list,view,NativeRigidBatchBounds(items),command);
    BatchRequire(indirect.has_value(), "current-depth native command refused");
    draw.native_indirect = *indirect; draw.native_visibility = &work;
    draw.native_visibility_command = batch.visibility_command; ++store.visibility_commands;
  } else {
    const auto indirect = UploadHostData(&command,sizeof(command),4);
    BatchRequire(indirect.memory != nullptr, "native indirect upload refused");
    draw.native_indirect = indirect.ref;
  }
  BatchRequire(draw.bindings.Valid(), "native batch bindings refused");
  batches.push_back(std::move(batch));
}
namespace {
void ResolveNativeRigidEmission(NativeRigidDrawStore &store, std::span<const NativeRigidBatchItem *const> items, bool visible) {
  BatchRequire(!items.empty(), "empty native emission receipt");
  const auto instances = uint32_t(items.size()), render_view = items[0]->view;
  for (const auto *item : items) {
    BatchRequire(item && item->output.Resolve(visible), "native draw resolved before recording or more than once");
    if (visible && item->Cutout()) {
      auto &cutouts = render_view == 3 ? store.scene_cutouts : store.shadow_cutouts;
      ++cutouts.emitted;
      cutouts.textured_emitted += (item->input.object_data.flags.x & RigidAlbedo) != 0;
    }
  }
  if (items[0]->water) {
    if (visible) store.water_emitted += instances; else store.water_culled += instances;
    return;
  }
  if (!visible) { store.scene_culled += instances; return; }
  if (items[0]->regression) NoteNativeRigidEmitted(items[0]->model_generation,render_view,instances);
  else if (render_view == 1) store.family_emitted += instances;
  else if (render_view == 3) store.scene_family_emitted += instances;
  if (render_view == 3) { store.scene_emitted += instances; ++store.scene_batches; }
  else { store.emitted += instances; ++store.shadow_batches; }
  if (instances > 1) store.merged_instances += instances;
}
}
void NoteNativeRigidEmission(const QueuedDraw &draw, std::span<const NativeRigidBatchItem *const> items) {
  if (!draw.native_rigid) return;
  auto &s = state();
  BatchRequire(s.native_rigid_draws && !items.empty(), "native emission lost its instance records");
  auto &store = *s.native_rigid_draws;
  for (const auto *item : items) BatchRequire(item && item->output.Record(), "native draw command recorded twice");
  if (draw.native_visibility) ++store.visibility_drawn;
  else ResolveNativeRigidEmission(store,items,true);
}
void SealNativeRigidVisibilityLocked(VideoState &s) {
  if (!s.native_rigid_draws) return;
  for (auto &entry : s.native_rigid_draws->visibility[Video::CurrentFrameSlot()]) {
    BatchRequire(!entry.sealed && entry.recording == s.command_list && entry.work->Seal(*s.command_list),
        "native visibility receipt copy missing before submission");
    entry.sealed = true;
  }
}
void DrainNativeRigidDrawsLocked(VideoState &s, uint32_t slot) {
  if (!s.native_rigid_draws || slot >= kNumFrames) return;
  auto &store = *s.native_rigid_draws;
  // DrainSlot is reached only after this slot's real submission fence. Collect
  // while batch items, image owners and all work buffers still exist.
  for (auto &entry : store.visibility[slot]) {
    BatchRequire(entry.sealed, "native visibility work retired without submission sealing");
    const auto receipts = entry.work->CollectAfterFence();
    BatchRequire(receipts.has_value(), "native visibility GPU receipt integrity failure");
    entry.receipts = *receipts;
  }
  for (const auto &batch : store.batches[slot]) if (batch.visibility_work != ~0u) {
    const auto &entry = store.visibility[slot].at(batch.visibility_work);
    BatchRequire(batch.visibility_command < entry.receipts.size(), "missing native visibility command receipt");
    const auto &receipt = entry.receipts[batch.visibility_command];
    BatchRequire(receipt.draw_recorded && receipt.requested_instances == batch.items.size(), "GPU command generation was not a real native draw");
    const auto emitted = receipt.EmittedInstances();
    BatchRequire(!emitted || emitted == batch.items.size(), "partial native batch visibility is not supported");
    ResolveNativeRigidEmission(store,batch.items,emitted != 0);
    ++store.visibility_collected;
    store.visibility_visible += emitted;
    store.visibility_culled += receipt.requested_instances-emitted;
  }
  for (const auto &record : store.records[slot]) {
    BatchRequire(record->output.Retire(), "native record retired with pending or duplicate output classification");
    const bool visible = record->output.Visible();
    if (record->water) { ++store.water_retired; continue; }
    if (visible && record->Cutout()) {
      auto &cutouts = record->view == 3 ? store.scene_cutouts : store.shadow_cutouts;
      ++cutouts.retired;
      cutouts.textured_retired += (record->input.object_data.flags.x & RigidAlbedo) != 0;
    }
    if (record->regression) NoteNativeRigidFenceRetired(record->model_generation,record->view);
    else if (visible && record->view == 1) ++store.family_retired;
    else if (visible && record->view == 3) ++store.scene_family_retired;
    if (record->view == 3) {
      ++store.scene_resource_retired;
      if (visible) ++store.scene_retired; else ++store.scene_retired_culled;
    }
    else ++store.retired;
  }
  store.records[slot].clear();
  store.batches[slot].clear();
  store.visibility[slot].clear();
}
} // namespace bd::gpu::scene
