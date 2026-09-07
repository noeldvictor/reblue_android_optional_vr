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
#include "gpu/scene/native_sampler_bridge.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/device.h"
#include "gpu/draw_queue.h"
#include "gpu/frame_stats.h"
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

REXCVAR_DEFINE_BOOL(bd_native_rigid_shadow, false, kCvarGroup,
    "Fail-closed native opaque rigid caster families, including multi-primitive nodes; no template warm-up.");
REXCVAR_DEFINE_BOOL(bd_native_rigid_scene, false, kCvarGroup,
    "Fail-closed native mono opaque rigid scene families with zero-to-three texture layers; no node interpreter/template warm-up.");
namespace bd::gpu::scene {
struct NativeRigidDrawStore {
  struct Batch {
    std::unique_ptr<plume::RenderDescriptorSet> constants;
    std::unique_ptr<plume::RenderDescriptorSet> images, samplers;
  };
  struct Program { NativeVertexInputHandle input; NativeRigidPrograms shaders; };
  // Immutable program variants remain bounded; per-draw descriptors/geometry
  // survive exactly their recording slot's fence. Upload pages have their own
  // shared bounded arena and are never read back as a CPU data source.
  std::vector<Program> programs;
  std::array<std::vector<std::shared_ptr<const NativeRigidBatchItem>>, kNumFrames> records;
  std::array<std::vector<Batch>, kNumFrames> batches;
  std::array<NativeRigidInstanceGPU, kNativeRigidBatchLimit> scratch;
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
  if (item->regression) NoteNativeRigidSubmitted(item->model_generation,item->instance,item->view);
  draw.native_rigid = std::move(item);
}
}
bool NativeRigidShadowEnabled() { return REXCVAR_GET(bd_native_rigid_shadow); }
bool NativeRigidSceneEnabled() { return REXCVAR_GET(bd_native_rigid_scene); }
bool SubmitNativeRigidShadow(const NativeInstancePose &pose, uint32_t node,
                             const std::optional<PrimitivePolicyInputs> &inputs) {
  if (!NativeRigidShadowEnabled()) return false;
  const auto *model = FindNativeInstanceNode(pose, node);
  if (!model) return false;
  const auto admission = PrepareNativeRigidCasterAdmission(*model, inputs);
  if (admission.route == NativeRigidCasterRoute::Legacy) return false;
  Require(admission.route == NativeRigidCasterRoute::Native, "caster family or object pass policy unavailable");
  const auto camera = FindNativePassCamera(1);
  Require(camera.has_value(), "fresh shadow pass camera unavailable");
  Require(node < pose.transforms.size(), "native caster transform unavailable");
  const auto plans = PrepareNativeRigidShadow(*model, pose.transforms[node], *inputs, *camera);
  Require(plans.has_value(), "whole-node caster resources or matrices unavailable");
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
      Require(shaders.shadow && shaders.scene, "native shader creation failed");
      store.programs.push_back({geometry->rigid_vertex_input, std::move(shaders)});
      program = store.programs.end() - 1;
    }
    PipelineState pipeline_state;
    pipeline_state.native_program = program->shaders.shadow.get();
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
    draw.bindings.layout = program->shaders.shadow->Layout();
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
    BD_INFO("[native-caster-family] frame {} nodes {} multi-primitive nodes {} submitted {} emitted {} fence-retired {}; excludes selected regression; whole-node opaque rigid admission",
        frame, store.family_nodes, store.family_multi_nodes, store.family_primitives, store.family_emitted, store.family_retired);
    store.reported_frame = frame;
    store.reported_generation = pose.model_generation;
  }
  return true;
}
bool SubmitNativeRigidScene(const NativeInstancePose &pose, uint32_t node,
                            const std::optional<PrimitivePolicyInputs> &inputs) {
  if (!NativeRigidSceneEnabled()) return false;
  const auto *model = FindNativeInstanceNode(pose, node);
  if (!model) return false;
  const auto admission = PrepareNativeRigidSceneAdmission(*model, inputs);
  if (admission.route == NativeRigidCasterRoute::Legacy) return false;
  const auto require = [](bool valid, const char *reason) {
    if (valid) return;
    BD_ERROR("[native-rigid-scene] admitted node refused: {}; interpreter/capture/replay remain disabled", reason);
    throw std::runtime_error(reason);
  };
  require(admission.route == NativeRigidCasterRoute::Native, "scene family or object pass policy unavailable");
  const char *refusal = "native scene object preparation failed";
  const auto plans = PrepareNativeRigidSceneForObject(pose, node, refusal);
  require(plans.has_value() && !plans->empty(), refusal);
  const auto camera = FindNativePassCamera(3);
  for (const auto &plan : *plans)
    require(camera && std::memcmp(&plan.pass.world_to_clip[0],
        camera->world_to_clip.data(), sizeof(RenderMatrix)) == 0, "scene camera changed before submission");
  auto &s = state();
  std::lock_guard lock(s.mutex);
  auto *commands = ActiveNativeSceneCommands(s.render_target ? s.render_target->texture : nullptr,
      s.depth_stencil ? s.depth_stencil->texture : nullptr);
  require(s.ready && s.device && s.command_list_open && commands, "native scene command scope unavailable");
  const auto *shape = commands->ColorShape();
  const auto current_camera = commands->Camera(FrameStatFrameCount(), 3);
  require(shape && shape->layers == 1 && current_camera &&
      current_camera->world_to_clip == camera->world_to_clip, "mono native scene scope/camera required");
  if (!s.native_rigid_draws) s.native_rigid_draws = std::make_shared<NativeRigidDrawStore>();
  auto &store = *s.native_rigid_draws;
  auto &records = store.records[Video::CurrentFrameSlot()];
  require(records.size() <= 4096 && plans->size() <= 4096-records.size(), "native draw retention capacity reached");
  struct Pending { QueuedDraw draw; std::shared_ptr<NativeRigidBatchItem> item; };
  std::vector<Pending> pending;
  pending.reserve(plans->size());
  // Preflight the complete node, including every pipeline and sampled owner,
  // before any sibling is submitted. The existing queue/fence retains all layers.
  for (const auto &plan : *plans) {
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
    draw.render_view = 3; draw.zwrite = true; draw.reorderable = true;
    require(draw.bindings.Valid(), "invalid native scene descriptor contract");
    item->geometry = geometry; item->input = {plan.object,plan.pass};
    item->model_generation = pose.model_generation; item->instance = pose.instance;
    item->regression = geometry->id == 0x258694267A8DBAEEull;
    item->albedo = plan.albedo; item->shadow = plan.shadow;
    pending.push_back({std::move(draw),std::move(item)});
  }
  store.scene_suppressed += plans->size()-pending.size();
  if (!pending.empty() && !s.draw_framebuffer_bound) {
    DrawQueueFlush(s.command_list);
    BindNativeSceneCommands(s, *commands); ApplyNativeSceneClear(s, *commands); s.draw_framebuffer_bound = true;
  }
  bool family_multi = pending.size() > 1 && !SelectedNativeRigidShadow(*model);
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
  if (SelectedNativeRigidShadow(*model) &&
      (store.scene_reported_generation != pose.model_generation || frame-store.scene_reported_frame >= 300)) {
    BD_INFO("[native-rigid-scene] frame {} submitted {} emitted {} suppressed {} fence-retired {}; node {} instance {} generation {}; owned packet and native program, no node interpreter/template/replay",
        frame, store.scene_submitted, store.scene_emitted, store.scene_suppressed, store.scene_retired,
        node, pose.instance, pose.model_generation);
    BD_INFO("[native-rigid-batch] frame {} scene instances {} indirect calls {} shadow instances {} indirect calls {} merged instances {}; native storage records, no translated gather",
        frame, store.scene_emitted, store.scene_batches, store.emitted, store.shadow_batches, store.merged_instances);
    BD_INFO("[native-scene-family] frame {} multi-primitive nodes {} submitted {} layered {} emitted {} fence-retired {}; excludes selected regression",
        frame, store.scene_family_multi_nodes, store.scene_family_primitives, store.scene_family_layered,
        store.scene_family_emitted, store.scene_family_retired);
    store.scene_reported_frame = frame;
    store.scene_reported_generation = pose.model_generation;
  }
  return true;
}
void PrepareNativeRigidBatchDraw(std::span<const NativeRigidBatchItem *const> items, QueuedDraw &draw) {
  auto &s = state();
  BatchRequire(s.native_rigid_draws && s.ready && s.device && s.command_list_open, "native batch device unavailable");
  auto &store = *s.native_rigid_draws;
  const auto slot = Video::CurrentFrameSlot();
  auto &batches = store.batches[slot];
  BatchRequire(items.size() <= kNativeRigidBatchLimit && batches.size() < 4096, "native batch capacity");
  const auto packed = std::span(store.scratch).first(items.size());
  BatchRequire(PackNativeRigidBatch(items,packed,FrameStatFrameCount(),slot), "stale or incompatible native batch");
  const auto &first = *items[0];
  BatchRequire(draw.pipeline == first.pipeline && draw.bindings.layout == first.layout && draw.framebuffer == first.framebuffer &&
      draw.indexed && draw.has_index_buffer && !draw.translated_instance_records, "native queue contract changed");
  uint64_t alignment = 16;
#if !defined(REBLUE_D3D12)
  alignment = static_cast<plume::VulkanDevice &>(*s.device).physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
#endif
  BatchRequire(alignment <= 65536, "storage alignment unsupported");
  const auto placement = PlanNativeRigidStorage(uint32_t(items.size()),uint32_t(alignment));
  BatchRequire(placement.has_value(), "native storage placement refused");
  const auto upload = AllocateHostUpload(placement->reserve,uint32_t(alignment));
  BatchRequire(upload.memory != nullptr, "bounded native instance upload refused");
  const auto offset = placement->Offset(upload.ref.offset);
  const auto prefix = uint32_t(offset-upload.ref.offset);
  BatchRequire(prefix <= upload.size && placement->bytes <= upload.size-prefix, "native storage slice outside upload");
  std::memcpy(upload.memory+prefix,packed.data(),placement->bytes);
  NativeRigidDescriptorSchema schema;
  NativeRigidDrawStore::Batch batch;
  batch.constants = schema.sets[0].create(s.device.get());
  BatchRequire(bool(batch.constants), "native storage descriptor unavailable");
  const plume::RenderBufferStructuredView view(sizeof(NativeRigidInstanceGPU),offset/sizeof(NativeRigidInstanceGPU));
  batch.constants->setBuffer(0,upload.ref.ref,placement->bytes,&view);
  draw.bindings = {}; draw.bindings.layout = first.layout;
  draw.bindings.set_count = 1; draw.bindings.sets[0] = batch.constants.get();
  if (first.view == 3) {
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
  const auto indirect = UploadHostData(&command,sizeof(command),4);
  BatchRequire(indirect.memory && draw.bindings.Valid(), "native indirect upload or bindings refused");
  draw.native_indirect = indirect.ref;
  batches.push_back(std::move(batch));
}
void NoteNativeRigidEmission(const GraphicsBindings &bindings, uint32_t render_view, uint32_t instances, uint64_t generation) {
  auto &s = state();
  if (!s.native_rigid_draws) return;
  auto &store = *s.native_rigid_draws;
  for (const auto &program : store.programs) if (bindings.layout == program.shaders.scene->Layout()) {
    if (generation) NoteNativeRigidEmitted(generation,render_view,instances);
    else if (render_view == 1) store.family_emitted += instances;
    else if (render_view == 3) store.scene_family_emitted += instances;
    if (render_view == 3 && bindings.set_count == 3) { store.scene_emitted += instances; ++store.scene_batches; }
    else if (render_view == 1 && bindings.set_count == 1) { store.emitted += instances; ++store.shadow_batches; }
    if (instances > 1) store.merged_instances += instances;
    return;
  }
}
void DrainNativeRigidDrawsLocked(VideoState &s, uint32_t slot) {
  if (!s.native_rigid_draws || slot >= kNumFrames) return;
  auto &store = *s.native_rigid_draws;
  for (const auto &record : store.records[slot]) {
    if (record->regression) NoteNativeRigidFenceRetired(record->model_generation,record->view);
    else if (record->view == 1) ++store.family_retired;
    else if (record->view == 3) ++store.scene_family_retired;
    if (record->view == 3) ++store.scene_retired;
    else ++store.retired;
  }
  store.records[slot].clear();
  store.batches[slot].clear();
}
} // namespace bd::gpu::scene
