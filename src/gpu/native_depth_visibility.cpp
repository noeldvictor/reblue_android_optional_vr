/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/native_depth_visibility.h"
#include <cstring>
#if defined(REBLUE_D3D12)
#include "src/gpu/shaders/hlsl/native_visibility_depth_cs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_visibility_depth_ms_cs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_visibility_reduce_cs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_visibility_cull_cs.hlsl.dxil.h"
#define VIS_BLOB(name) g_##name##_dxil, sizeof(g_##name##_dxil)
#else
#include <plume_vulkan.h>
#include "src/gpu/shaders/hlsl/native_visibility_depth_cs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_visibility_depth_ms_cs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_visibility_reduce_cs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_visibility_cull_cs.hlsl.spirv.h"
#define VIS_BLOB(name) g_##name##_spirv, sizeof(g_##name##_spirv)
#endif
namespace bd::gpu {
using namespace plume;
static_assert(!std::is_copy_constructible_v<NativeDepthVisibilityWork> &&
              !std::is_move_constructible_v<NativeDepthVisibilityWork>);
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
    NativeDepthVisibilityProgramHandle program, NativeTargetImageHandle depth, const NativeOcclusionView &view,
    std::shared_ptr<NativeDepthVisibilityBudget> budget, uint32_t capacity) {
  if (!budget || !capacity || capacity > NativeDepthPyramid::kCommandLimit || !program || !depth || !depth->image || !depth->view || !depth->identity || !view.frame ||
      view.scope != NativeOcclusionScope{depth->identity,depth->shape.width,depth->shape.height,depth->shape.samples}) return {};
  for (float value : view.camera.world_to_clip) if (!std::isfinite(value)) return {};
  const auto plan = NativeDepthPyramid::Plan(depth->shape);
  if (!plan) return {};
  auto result = std::make_unique<NativeDepthVisibilityWork>();
  const uint64_t buffer_bytes = plan->bytes+2*capacity*NativeDepthPyramid::kCommandStride;
  if (!budget->Acquire(buffer_bytes)) return {};
  result->reservation_.budget = std::move(budget); result->reservation_.bytes = buffer_bytes;
  result->program_ = std::move(program); result->depth_ = std::move(depth);
  result->view_ = view; result->plan_ = *plan; result->capacity_ = capacity;
  result->expected_.reserve(capacity); result->receipts_.reserve(capacity);
  result->pyramid_ = device.createBuffer(RenderBufferDesc::DefaultBuffer(plan->bytes,
      RenderBufferFlag::STORAGE | RenderBufferFlag::UNORDERED_ACCESS));
  result->indirect_ = device.createBuffer(RenderBufferDesc::DefaultBuffer(
      capacity*NativeDepthPyramid::kCommandStride,
      RenderBufferFlag::STORAGE | RenderBufferFlag::UNORDERED_ACCESS | RenderBufferFlag::INDIRECT));
  result->readback_ = device.createBuffer(RenderBufferDesc::ReadbackBuffer(capacity*NativeDepthPyramid::kCommandStride));
  Schemas schema;
  result->depth_set_ = schema.depth.create(&device); result->cull_set_ = schema.cull.create(&device);
  if (!result->pyramid_ || !result->indirect_ || !result->readback_ || !result->depth_set_ || !result->cull_set_) return {};
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
  return BuildDepth(cmd);
}
bool NativeDepthVisibilityWork::RefreshDepth(RenderCommandList &cmd, const NativeOcclusionView &view) {
  if (recording_ != &cmd || sealed_ || snapshots_ >= NativeDepthPyramid::kCommandLimit || view.frame != view_.frame || view.scope != view_.scope ||
      depth_->layout != RenderTextureLayout::DEPTH_WRITE) return false;
  for (float value : view.camera.world_to_clip) if (!std::isfinite(value)) return false;
  view_ = view;
  return BuildDepth(cmd);
}
bool NativeDepthVisibilityWork::BuildDepth(RenderCommandList &cmd) {
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
  ++snapshots_;
  return true;
}
std::optional<RenderBufferReference> NativeDepthVisibilityWork::RecordCommand(RenderCommandList &cmd,
    const NativeOcclusionView &view, const std::optional<scene::NativeBounds> &bounds,
    const scene::NativeRigidIndexedCommand &draw) {
  if (recording_ != &cmd || sealed_ || count_ >= capacity_ || !draw.index_count || !draw.instance_count ||
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
  expected_.push_back(draw); receipts_.push_back({draw.instance_count,0,false});
  ++count_;
  return indirect_->at(offset);
}
bool NativeDepthVisibilityWork::DrawCommand(RenderCommandList &cmd, uint32_t index) {
  if (recording_ != &cmd || sealed_ || index >= count_ || receipts_[index].draw_recorded) return false;
  cmd.drawIndexedIndirect(indirect_.get(),index*NativeDepthPyramid::kCommandStride,1,sizeof(scene::NativeRigidIndexedCommand));
  receipts_[index].draw_recorded = true;
  return true;
}
bool NativeDepthVisibilityWork::Seal(RenderCommandList &cmd) {
  if (recording_ != &cmd || sealed_ || !count_) return false;
  cmd.setFramebuffer(nullptr);
  const std::array barriers{RenderBufferBarrier(indirect_.get(),RenderBufferAccess::READ),
      RenderBufferBarrier(readback_.get(),RenderBufferAccess::WRITE)};
  cmd.barriers(RenderBarrierStage::COPY,barriers.data(),uint32_t(barriers.size()));
  cmd.copyBufferRegion(readback_->at(0),indirect_->at(0),count_*NativeDepthPyramid::kCommandStride);
#if !defined(REBLUE_D3D12)
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(static_cast<VulkanCommandList &>(cmd).vk,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
      0,1,&host,0,nullptr,0,nullptr);
#endif
  sealed_ = true;
  return true;
}
std::optional<std::span<const NativeDepthVisibilityReceipt>> NativeDepthVisibilityWork::CollectAfterFence() {
  if (!sealed_ || collected_) return {};
#if !defined(REBLUE_D3D12)
  auto &buffer = static_cast<VulkanBuffer &>(*readback_);
  if (vmaInvalidateAllocation(buffer.device->allocator,buffer.allocation,0,VK_WHOLE_SIZE) != VK_SUCCESS) return {};
#endif
  const auto *bytes = static_cast<const uint8_t *>(readback_->map());
  if (!bytes) return {};
  bool valid = true;
  for (uint32_t n=0;n<count_;++n) {
    scene::NativeRigidIndexedCommand actual;
    std::memcpy(&actual,bytes+n*NativeDepthPyramid::kCommandStride,sizeof(actual));
    const auto instances = actual.instance_count;
    actual.instance_count = expected_[n].instance_count;
    valid &= (instances == 0 || instances == expected_[n].instance_count) &&
        std::memcmp(&actual,&expected_[n],sizeof(actual)) == 0;
    receipts_[n].generated_instances = instances;
  }
  readback_->unmap();
  if (!valid) return {};
  collected_ = true;
  return std::span<const NativeDepthVisibilityReceipt>(receipts_);
}
} // namespace bd::gpu
