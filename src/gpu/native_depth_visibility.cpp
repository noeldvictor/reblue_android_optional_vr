/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/native_depth_visibility.h"
#if defined(REBLUE_D3D12)
#include "src/gpu/shaders/hlsl/native_visibility_depth_cs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_visibility_depth_ms_cs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_visibility_reduce_cs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_visibility_cull_cs.hlsl.dxil.h"
#define VIS_BLOB(name) g_##name##_dxil, sizeof(g_##name##_dxil)
#else
#include "src/gpu/shaders/hlsl/native_visibility_depth_cs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_visibility_depth_ms_cs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_visibility_reduce_cs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_visibility_cull_cs.hlsl.spirv.h"
#define VIS_BLOB(name) g_##name##_spirv, sizeof(g_##name##_spirv)
#endif
namespace bd::gpu {
using namespace plume;
namespace {
struct Schemas {
  RenderDescriptorSetBuilder depth, cull;
  Schemas() {
    depth.begin(); depth.addTexture(0); depth.addReadWriteStructuredBuffer(1); depth.end();
    cull.begin(); cull.addStructuredBuffer(0); cull.addReadWriteByteAddressBuffer(1); cull.end();
  }
};
struct DepthPush { uint32_t width, height, offset, next_width, next_height, next_offset, samples, reserved; };
struct CullPush {
  NativeOcclusionPacket bounds;
  std::array<uint32_t,4> draw, output;
};
static_assert(sizeof(DepthPush) == 32 && sizeof(CullPush) == 128);
}
struct NativeDepthVisibilityProgram {
  std::unique_ptr<RenderPipelineLayout> depth_layout, cull_layout;
  std::array<std::unique_ptr<RenderShader>,4> shaders;
  std::array<std::unique_ptr<RenderPipeline>,4> pipelines;
};
NativeDepthVisibilityProgramHandle CreateNativeDepthVisibilityProgram(RenderDevice &device) {
  auto result = std::make_shared<NativeDepthVisibilityProgram>();
  Schemas schema;
  const auto layout = [&](const RenderDescriptorSetBuilder &set, uint32_t bytes) {
    RenderPipelineLayoutBuilder builder; builder.begin(false,true);
    builder.addDescriptorSet(set); builder.addPushConstant(0,0,bytes,RenderShaderStageFlag::COMPUTE);
    builder.end(); return builder.create(&device);
  };
  result->depth_layout = layout(schema.depth,sizeof(DepthPush));
  result->cull_layout = layout(schema.cull,sizeof(CullPush));
  if (!result->depth_layout || !result->cull_layout) return {};
#if defined(REBLUE_D3D12)
  constexpr auto format = RenderShaderFormat::DXIL;
#else
  constexpr auto format = RenderShaderFormat::SPIRV;
#endif
  result->shaders[0] = device.createShader(VIS_BLOB(native_visibility_depth_cs),"main",format);
  result->shaders[1] = device.createShader(VIS_BLOB(native_visibility_depth_ms_cs),"main",format);
  result->shaders[2] = device.createShader(VIS_BLOB(native_visibility_reduce_cs),"main",format);
  result->shaders[3] = device.createShader(VIS_BLOB(native_visibility_cull_cs),"main",format);
  for (uint32_t i=0;i<4;++i) {
    if (!result->shaders[i]) return {};
    result->pipelines[i] = device.createComputePipeline(RenderComputePipelineDesc(
        i == 3 ? result->cull_layout.get() : result->depth_layout.get(), result->shaders[i].get(),
        i == 3 ? 1 : 8,i == 3 ? 1 : 8,1));
    if (!result->pipelines[i]) return {};
  }
  return result;
}
std::unique_ptr<NativeDepthVisibilityWork> NativeDepthVisibilityWork::Create(RenderDevice &device,
    NativeDepthVisibilityProgramHandle program, NativeTargetImageHandle depth, const NativeOcclusionView &view, uint32_t capacity) {
  if (!capacity || capacity > NativeDepthPyramid::kCommandLimit || !program || !depth || !depth->image || !depth->view || !depth->identity || !view.frame ||
      view.scope != NativeOcclusionScope{depth->identity,depth->shape.width,depth->shape.height,depth->shape.samples}) return {};
  for (float value : view.camera.world_to_clip) if (!std::isfinite(value)) return {};
  const auto plan = NativeDepthPyramid::Plan(depth->shape);
  if (!plan) return {};
  auto result = std::make_unique<NativeDepthVisibilityWork>();
  result->program_ = std::move(program); result->depth_ = std::move(depth);
  result->view_ = view; result->plan_ = *plan; result->capacity_ = capacity;
  result->pyramid_ = device.createBuffer(RenderBufferDesc::DefaultBuffer(plan->bytes,
      RenderBufferFlag::STORAGE | RenderBufferFlag::UNORDERED_ACCESS));
  result->indirect_ = device.createBuffer(RenderBufferDesc::DefaultBuffer(
      capacity*NativeDepthPyramid::kCommandStride,
      RenderBufferFlag::STORAGE | RenderBufferFlag::UNORDERED_ACCESS | RenderBufferFlag::INDIRECT));
  Schemas schema;
  result->depth_set_ = schema.depth.create(&device); result->cull_set_ = schema.cull.create(&device);
  if (!result->pyramid_ || !result->indirect_ || !result->depth_set_ || !result->cull_set_) return {};
  const RenderBufferStructuredView floats(sizeof(float));
  result->depth_set_->setTexture(0,result->depth_->image.get(),RenderTextureLayout::SHADER_READ,result->depth_->view.get());
  result->depth_set_->setBuffer(1,result->pyramid_.get(),plan->bytes,&floats);
  result->cull_set_->setBuffer(0,result->pyramid_.get(),plan->bytes,&floats);
  result->cull_set_->setBuffer(1,result->indirect_.get(),capacity*NativeDepthPyramid::kCommandStride);
  return result;
}
bool NativeDepthVisibilityWork::RecordDepth(RenderCommandList &cmd) {
  if (recording_ || depth_->layout != RenderTextureLayout::DEPTH_WRITE) return false;
  recording_ = &cmd;
  cmd.setFramebuffer(nullptr);
  cmd.barriers(RenderBarrierStage::COMPUTE,RenderBufferBarrier(pyramid_.get(),RenderBufferAccess::WRITE),
      RenderTextureBarrier(depth_->image.get(),RenderTextureLayout::SHADER_READ));
  depth_->layout = RenderTextureLayout::SHADER_READ;
  cmd.setComputePipelineLayout(program_->depth_layout.get()); cmd.setComputeDescriptorSet(depth_set_.get(),0);
  for (uint32_t n=0;n<plan_.count;++n) {
    const auto &next = plan_.levels[n];
    const auto previous = n ? plan_.levels[n-1] : NativeDepthPyramid::Level{depth_->shape.width,depth_->shape.height,0};
    if (n) cmd.barriers(RenderBarrierStage::COMPUTE,RenderBufferBarrier(pyramid_.get(),RenderBufferAccess::READ|RenderBufferAccess::WRITE));
    cmd.setPipeline(program_->pipelines[n ? 2 : depth_->shape.samples > 1 ? 1 : 0].get());
    const DepthPush push{previous.width,previous.height,previous.offset,next.width,next.height,next.offset,depth_->shape.samples,0};
    cmd.setComputePushConstants(0,&push,0,sizeof(push));
    cmd.dispatch((next.width+7)/8,(next.height+7)/8,1);
  }
  cmd.barriers(RenderBarrierStage::COMPUTE,RenderBufferBarrier(pyramid_.get(),RenderBufferAccess::READ));
  cmd.barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(depth_->image.get(),RenderTextureLayout::DEPTH_WRITE));
  depth_->layout = RenderTextureLayout::DEPTH_WRITE;
  return true;
}
std::optional<RenderBufferReference> NativeDepthVisibilityWork::RecordCommand(RenderCommandList &cmd,
    const NativeOcclusionView &view, const std::optional<scene::NativeBounds> &bounds,
    const scene::NativeRigidIndexedCommand &draw) {
  if (recording_ != &cmd || count_ >= capacity_ || !draw.index_count || !draw.instance_count ||
      view.frame != view_.frame || view.scope != view_.scope || view.camera.world_to_clip != view_.camera.world_to_clip) return {};
  CullPush push{};
  const auto prepared = bounds ? PrepareNativeOcclusion(view.camera,*bounds) : std::nullopt;
  if (prepared) { push.bounds = *prepared; push.bounds.extent[3] = 1; }
  const uint32_t offset = count_*NativeDepthPyramid::kCommandStride;
  push.draw = {draw.index_count,draw.instance_count,draw.first_index,std::bit_cast<uint32_t>(draw.base_vertex)};
  push.output = {draw.first_instance,offset,view.scope.width,view.scope.height};
  cmd.barriers(RenderBarrierStage::COMPUTE,RenderBufferBarrier(indirect_.get(),RenderBufferAccess::WRITE));
  cmd.setComputePipelineLayout(program_->cull_layout.get()); cmd.setComputeDescriptorSet(cull_set_.get(),0);
  cmd.setPipeline(program_->pipelines[3].get()); cmd.setComputePushConstants(0,&push,0,sizeof(push)); cmd.dispatch(1,1,1);
  // GRAPHICS includes DRAW_INDIRECT in Plume. No host readback/wait participates
  // in visibility; the generated command is consumed later in this recording.
  cmd.barriers(RenderBarrierStage::GRAPHICS,RenderBufferBarrier(indirect_.get(),RenderBufferAccess::READ));
  ++count_;
  return indirect_->at(offset);
}
} // namespace bd::gpu
