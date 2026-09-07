/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_rigid_draw.h"
#include "gpu/scene/native_rigid_shadow.h"
#include "gpu/scene/native_rigid_program.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/device.h"
#include "gpu/draw_queue.h"
#include "gpu/frame_stats.h"
#include "gpu/host_upload.h"
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
    "Fail-closed direct native shadow acceptance for the selected rigid field asset; no template warm-up.");
namespace bd::gpu::scene {
struct NativeRigidDrawStore {
  struct Record {
    std::shared_ptr<const NativeGeometry> geometry;
    std::unique_ptr<plume::RenderDescriptorSet> constants;
  };
  struct Program { NativeVertexInputHandle input; NativeRigidPrograms shaders; };
  // Immutable program variants remain bounded; per-draw descriptors/geometry
  // survive exactly their recording slot's fence. Upload pages have their own
  // shared bounded arena and are never read back as a CPU data source.
  std::vector<Program> programs;
  std::array<std::vector<Record>, kNumFrames> records;
  uint64_t submitted = 0, suppressed = 0, retired = 0;
  uint32_t reported_frame = 0;
};
namespace {
void Require(bool valid, const char *reason) {
  if (valid) return;
  BD_ERROR("[native-rigid-shadow] selected node refused: {}; interpreter/capture/replay remain disabled", reason);
  throw std::runtime_error(reason);
}
}
bool NativeRigidShadowEnabled() { return REXCVAR_GET(bd_native_rigid_shadow); }
bool SubmitNativeRigidShadow(const NativeInstancePose &pose, uint32_t node,
                             const std::optional<PrimitivePolicyInputs> &inputs) {
  if (!NativeRigidShadowEnabled()) return false;
  const auto *model = FindNativeInstanceNode(pose, node);
  if (!model || !SelectedNativeRigidShadow(*model)) return false;
  Require(inputs.has_value(), "object pass policy unavailable");
  const auto camera = FindNativePassCamera(1);
  Require(camera.has_value(), "fresh shadow pass camera unavailable");
  const auto plan = PrepareNativeRigidShadow(*model, pose.transforms[node], *inputs, *camera);
  Require(plan.has_value(), "whole-node caster contract unsupported");
  auto &s = state();
  std::lock_guard lock(s.mutex);
  auto *commands = ActiveNativeShadowCommands(s.render_target ? s.render_target->texture : nullptr,
      s.depth_stencil ? s.depth_stencil->texture : nullptr);
  Require(s.ready && s.device && s.command_list_open && commands, "native shadow command scope unavailable");
  const auto current_camera = commands->Camera(FrameStatFrameCount(), 1);
  Require(current_camera && current_camera->world_to_clip == camera->world_to_clip, "shadow scope changed before submission");
  if (!s.native_rigid_draws) s.native_rigid_draws = std::make_shared<NativeRigidDrawStore>();
  auto &store = *s.native_rigid_draws;
  if (!plan->draw) { ++store.suppressed; return true; }
  const auto &geometry = plan->geometry;
  auto &records = store.records[Video::CurrentFrameSlot()];
  Require(records.size() < 4096, "native draw retention capacity reached");
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
  pipeline_state.cullMode = plan->cull == PrimitiveCull::Back ? plume::RenderCullMode::BACK :
      plan->cull == PrimitiveCull::Front ? plume::RenderCullMode::FRONT : plume::RenderCullMode::NONE;
  SanitizePipelineState(pipeline_state);
  auto *pipeline = GetOrCreatePipeline(pipeline_state);
  Require(pipeline != nullptr, "native shadow pipeline unavailable");
  uint64_t alignment = 256;
#if !defined(REBLUE_D3D12)
  alignment = static_cast<plume::VulkanDevice &>(*s.device).physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
#endif
  Require(alignment && alignment <= 65536 && !(alignment & (alignment - 1)), "uniform alignment unsupported");
  const auto object_upload = UploadHostData(&plan->object, sizeof(plan->object), uint32_t(alignment));
  const auto pass_upload = UploadHostData(&plan->pass, sizeof(plan->pass), uint32_t(alignment));
  Require(object_upload.memory && pass_upload.memory, "bounded uniform upload refused");
  NativeRigidDescriptorSchema schema;
  auto constants = schema.sets[0].create(s.device.get());
  Require(bool(constants), "native constant descriptors unavailable");
  constants->setBuffer(0, object_upload.ref.ref, sizeof(plan->object));
  constants->setBuffer(1, pass_upload.ref.ref, sizeof(plan->pass));
  QueuedDraw draw;
  draw.pipeline = pipeline;
  draw.bindings.layout = program->shaders.shadow->Layout();
  draw.bindings.set_count = 1; draw.bindings.sets[0] = constants.get();
  draw.bindings.dynamic_counts[0] = 2;
  draw.bindings.offsets[0] = uint32_t(object_upload.ref.offset);
  draw.bindings.offsets[1] = uint32_t(pass_upload.ref.offset);
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
  // No translated-instance ABI opt-in. Native instance batching is separate
  // follow-up work, not an excuse to pack source register records here.
  Require(draw.bindings.Valid(), "invalid native descriptor contract");
  records.push_back({geometry, std::move(constants)});
  if (!s.draw_framebuffer_bound) {
    DrawQueueFlush(s.command_list);
    BindNativeSceneCommands(s, *commands); ApplyNativeSceneClear(s, *commands);
    s.draw_framebuffer_bound = true;
  }
  DrawQueuePush(draw);
  if (!DrawQueueEnabled()) DrawQueueFlush(s.command_list);
  ++store.submitted;
  const auto frame = FrameStatFrameCount();
  if (store.submitted == 1 || frame - store.reported_frame >= 300) {
    BD_INFO("[native-rigid-shadow] frame {} submitted {} suppressed {} fence-retired {}; node {} instance {} generation {} phase {}; native program and owned matrices, no interpreter/template/replay",
        frame, store.submitted, store.suppressed, store.retired, node, pose.instance, pose.model_generation, inputs->phase);
    store.reported_frame = frame;
  }
  return true;
}
void DrainNativeRigidDrawsLocked(VideoState &s, uint32_t slot) {
  if (!s.native_rigid_draws || slot >= kNumFrames) return;
  auto &store = *s.native_rigid_draws;
  store.retired += store.records[slot].size(); store.records[slot].clear();
}
} // namespace bd::gpu::scene
