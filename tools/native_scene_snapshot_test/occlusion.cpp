/**
 * @brief Real native query results and attachment preservation, no game captures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/native_occlusion_program.h"
#include "gpu/scene/native_scene_commands.h"
#include "gpu/scene/native_mesh_data.h"
#include <plume_vulkan.h>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace plume;
using namespace bd::gpu;
using namespace bd::gpu::scene;
namespace {
constexpr uint32_t size = 8, color_bytes = size*size*8, depth_bytes = size*size*4;
void Need(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
std::shared_ptr<NativeTargetImage> Image(RenderDevice &device, uint32_t samples, bool depth) {
  auto result = std::make_shared<NativeTargetImage>();
  result->identity = depth ? 2 : 1;
  result->shape = {size,size,1,depth ? RenderFormat::D32_FLOAT_S8_UINT : RenderFormat::R16G16B16A16_FLOAT,samples};
  auto desc = depth ? RenderTextureDesc::DepthTarget(size,size,result->shape.format,RenderMultisampling(samples))
      : RenderTextureDesc::ColorTarget(size,size,result->shape.format,RenderMultisampling(samples));
  result->image = device.createTexture(desc); result->descriptor = 0;
  Need(result->image && static_cast<VulkanTexture *>(result->image.get())->vk, "Occlusion image allocation");
  return result;
}
void Run(RenderDevice &device, uint32_t samples, bool rotated) {
  auto queue = device.createCommandQueue(RenderCommandListType::DIRECT);
  auto cmd = queue->createCommandList(); auto fence = device.createCommandFence();
  auto pool = device.createOcclusionQueryPool(3);
  auto program = CreateNativeOcclusionProgram(device);
  Need(pool && bool(program), "Production occlusion program/query pool");
  RenderDescriptorSetBuilder set_builder; set_builder.begin();
  for (uint32_t n=0;n<3;++n) set_builder.addConstantBufferDynamic(n);
  set_builder.end();
  auto resume_set = set_builder.create(&device);
  RenderPipelineLayoutBuilder layout_builder; layout_builder.begin(false,true);
  layout_builder.addDescriptorSet(set_builder); layout_builder.end();
  auto resume_layout = layout_builder.create(&device);
  const uint32_t alignment = uint32_t(static_cast<VulkanDevice &>(device).physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
  auto uniform = device.createBuffer(RenderBufferDesc::UploadBuffer(4*alignment+16,RenderBufferFlag::CONSTANT));
  Need(resume_set && resume_layout && uniform,"Occlusion resume descriptor allocation");
  for (uint32_t n=0;n<3;++n) resume_set->setBuffer(n,uniform.get(),16);
  GraphicsBindings resume; resume.layout = resume_layout.get(); resume.sets[0] = resume_set.get();
  resume.set_count = 1; resume.dynamic_counts[0] = 3;
  resume.offsets = {alignment,2*alignment,3*alignment};
  auto color = Image(device,samples,false), depth = Image(device,samples,true);
  auto resolved_color = samples > 1 ? Image(device,1,false) : nullptr;
  auto resolved_depth = samples > 1 ? Image(device,1,true) : nullptr;
  const RenderTexture *source = color->image.get();
  const RenderTexture *output = resolved_color ? resolved_color->image.get() : nullptr;
  RenderFramebufferDesc desc;
  desc.colorAttachments = &source; desc.colorAttachmentsCount = 1; desc.depthAttachment = depth->image.get();
  if (samples > 1) {
    desc.colorResolveAttachments = &output; desc.depthResolveAttachment = resolved_depth->image.get();
    desc.depthResolveMode = RenderResolveMode::MIN;
  }
  auto framebuffer = device.createFramebuffer(desc);
  Need(bool(framebuffer), "Occlusion native framebuffer");
  auto pipeline = CreateNativeOcclusionPipeline(device,program,color->shape);
  Need(bool(pipeline), "Production occlusion pipeline");
  auto stereo = color->shape; stereo.layers = 2;
  Need(!CreateNativeOcclusionPipeline(device,program,stereo), "Mono queries cannot authorize stereo culling");
  const std::array<SampledImage,2> resolved = samples > 1
      ? std::array{resolved_color->Sampled(),resolved_depth->Sampled()} : std::array<SampledImage,2>{};
  auto scene = NativeSceneCommands::Create({color,depth},framebuffer.get(),resolved,{{2,.25f,.5f,1},.5f,0});
  Need(bool(scene), "Occlusion native scene commands");
  const RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  // Both translated and rotated cameras, deliberately far from the world origin.
  const RenderMatrix camera = rotated
      ? RenderMatrix{0,0,1,0, 0,1,0,0, -1,0,0,0, 300,-200,-100,1}
      : RenderMatrix{1,0,0,0, 0,1,0,0, 0,0,1,0, -100,-200,-300,1};
  scene->PublishCamera({identity,camera,identity},true,true,false,10,3);
  const auto view = scene->OcclusionView(10);
  Need(view && view->scope.depth == 2 && view->scope.samples == samples, "Owned occlusion view");
  NativeOcclusionTracker tracker;
  tracker.Begin(10);
  NativeMeshData mesh;
  mesh.attributes = {{MeshSemantic::Position,0,0}}; mesh.layout = NativeMeshLayoutId(mesh.attributes);
  mesh.indices = {0,1,2,0,2,3,4,5,6,4,6,7};
  mesh.streams.push_back({0,16,std::vector<uint8_t>(8*16)});
  for (unsigned corner=0;corner<8;++corner) {
    // Wide/shallow native geometry: isotropic radius must not replace its axes.
    const std::array<float,4> p{(corner&1) ? .15f : -.15f,(corner&2) ? .15f : -.15f,(corner&4) ? .02f : -.02f,1};
    for (unsigned axis=0;axis<4;++axis) {
      const auto bits = std::bit_cast<uint32_t>(p[axis]);
      for (unsigned byte=0;byte<4;++byte) mesh.streams[0].bytes[corner*16+axis*4+byte] = uint8_t(bits >> (8*byte));
    }
  }
  const auto local_bounds = BuildNativeMeshBounds(mesh);
  Need(bool(local_bounds),"Production indexed native bounds");
  std::array<NativeBounds,3> bounds;
  std::array<NativeOcclusionObservation,3> queries;
  for (uint32_t i=0;i<queries.size();++i) {
    const float z = i == 0 ? .25f : i == 1 ? .75f : .5f;
    const RenderMatrix world = rotated
        ? RenderMatrix{0,0,-1,0, 0,1,0,0, 1,0,0,0, 100+z,200,300,1}
        : RenderMatrix{1,0,0,0, 0,1,0,0, 0,0,1,0, 100,200,300+z,1};
    const auto transformed = TransformNativeBounds(*local_bounds,world);
    Need(bool(transformed),"Production transformed native bounds"); bounds[i] = *transformed;
    Need(tracker.Request({12,34,5,i},view,bounds[i]) == NativeOcclusionDecision::NoHistory, "First native request stays visible");
  }
  tracker.Queries(*view,[&](const auto &query) { queries.at(query.identity.primitive) = query; });
  cmd->begin(); cmd->resetQueryPool(pool.get(),0,3);
  scene->Bind(*cmd); Need(scene->ApplyClear(*cmd), "Occlusion first clear");
  cmd->setViewports(RenderViewport(0,0,size,size)); cmd->setScissors(RenderRect(0,0,size,size));
  Need(WithNativeOcclusionBindings(*cmd,program.layout.get(),resume,[&] {
    cmd->setPipeline(pipeline.get());
    for (uint32_t i=0;i<queries.size();++i) {
      cmd->setGraphicsPushConstants(0,&queries[i].packet,0,sizeof(queries[i].packet));
      cmd->beginQuery(pool.get(),i); cmd->drawInstanced(36,1,0,0); cmd->endQuery(pool.get(),i);
    }
  }),"Occlusion binding scope");
  Need(static_cast<VulkanCommandList *>(cmd.get())->activeGraphicsPipelineLayout == resume_layout.get(),
      "Query leaked its push-only layout into the next descriptor consumer");
  cmd->setGraphicsDescriptorSetDynamic(resume_set.get(),0,resume.offsets.data(),3);
  cmd->setFramebuffer(nullptr); // complete ordinary native MSAA resolves first
  const auto color_read = scene->ColorReadImage();
  const auto depth_read = resolved_depth ? resolved_depth->Sampled() : depth->Sampled();
  auto readback = device.createBuffer(RenderBufferDesc::ReadbackBuffer(color_bytes+depth_bytes));
  Need(bool(readback), "Occlusion readback");
  for (const auto &image : {color_read,depth_read}) {
    cmd->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(image.texture,RenderTextureLayout::COPY_SOURCE));
    *image.layout = RenderTextureLayout::COPY_SOURCE;
    if (image.texture == color_read.texture)
      cmd->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(readback.get(),image.format,size,size,1,size,0),
          RenderTextureCopyLocation::Subresource(image.texture,0,0));
    else {
      VkBufferImageCopy copy{}; copy.bufferOffset = color_bytes;
      copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      copy.imageSubresource.layerCount = 1; copy.imageExtent = {size,size,1};
      vkCmdCopyImageToBuffer(static_cast<VulkanCommandList *>(cmd.get())->vk,
          static_cast<VulkanTexture *>(image.texture)->vk,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          static_cast<VulkanBuffer *>(readback.get())->vk,1,&copy);
    }
  }
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(static_cast<VulkanCommandList *>(cmd.get())->vk,VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
  cmd->end(); queue->executeCommandLists(cmd.get(),fence.get());
  auto *native_fence = static_cast<VulkanCommandFence *>(fence.get());
  Need(vkWaitForFences(native_fence->device->vk,1,&native_fence->vk,VK_TRUE,5'000'000'000ULL) == VK_SUCCESS,
      "Occlusion fence failed/timed out");
  auto *native_pool = static_cast<VulkanQueryPool *>(pool.get());
  std::array<uint64_t,3> results{};
  Need(vkGetQueryPoolResults(native_pool->device->vk,native_pool->vk,0,3,sizeof(results),results.data(),
      sizeof(uint64_t),VK_QUERY_RESULT_64_BIT) == VK_SUCCESS, "Occlusion query results unavailable");
  Need(results[0] > 0 && results[1] == 0 && results[2] > 0, "Front/hidden/intersecting query classification");
  pool->queryResults(3);
  Need(std::equal(results.begin(),results.end(),pool->getResults()), "Runtime query readback matches Vulkan results");
  for (uint32_t i=0;i<queries.size();++i) tracker.Collect(queries[i],pool->getResults()[i] == 0);
  auto *native_buffer = static_cast<VulkanBuffer *>(readback.get());
  Need(vmaInvalidateAllocation(native_buffer->device->allocator,native_buffer->allocation,0,VK_WHOLE_SIZE) == VK_SUCCESS,
      "Occlusion readback visibility");
  const auto *bytes = static_cast<const uint8_t *>(readback->map()); Need(bytes != nullptr,"Occlusion readback map");
  const std::array<uint16_t,4> expected{0x4000,0x3400,0x3800,0x3c00};
  for (uint32_t pixel=0;pixel<size*size;++pixel) {
    Need(std::memcmp(bytes+pixel*8,expected.data(),8) == 0,"Occlusion query changed colour");
    float actual; std::memcpy(&actual,bytes+color_bytes+pixel*4,4);
    Need(actual == .5f,"Occlusion query changed depth");
  }
  readback->unmap();
  // A second real submission/fence with unchanged depth and camera, not two
  // fabricated frame stamps for one result. The hidden native consumer must
  // warm up first, then be culled while front/intersecting consumers remain.
  auto next_view = *view; next_view.frame = 11; tracker.Begin(11);
  for (uint32_t i=0;i<queries.size();++i)
    Need(tracker.Request({12,34,5,i},next_view,bounds[i]) ==
        (i == 1 ? NativeOcclusionDecision::Warming : NativeOcclusionDecision::Visible), "One zero cannot cull");
  tracker.Queries(next_view,[&](const auto &query) { queries.at(query.identity.primitive) = query; });
  auto next_cmd = queue->createCommandList(); auto next_fence = device.createCommandFence();
  next_cmd->begin(); next_cmd->resetQueryPool(pool.get(),0,3); scene->Bind(*next_cmd);
  next_cmd->setViewports(RenderViewport(0,0,size,size)); next_cmd->setScissors(RenderRect(0,0,size,size));
  Need(WithNativeOcclusionBindings(*next_cmd,program.layout.get(),resume,[&] {
    next_cmd->setPipeline(pipeline.get());
    for (uint32_t i=0;i<queries.size();++i) {
      next_cmd->setGraphicsPushConstants(0,&queries[i].packet,0,sizeof(queries[i].packet));
      next_cmd->beginQuery(pool.get(),i); next_cmd->drawInstanced(36,1,0,0); next_cmd->endQuery(pool.get(),i);
    }
  }),"Second query binding scope");
  next_cmd->end(); queue->executeCommandLists(next_cmd.get(),next_fence.get());
  native_fence = static_cast<VulkanCommandFence *>(next_fence.get());
  Need(vkWaitForFences(native_fence->device->vk,1,&native_fence->vk,VK_TRUE,5'000'000'000ULL) == VK_SUCCESS,
      "Second occlusion fence failed/timed out");
  pool->queryResults(3);
  for (uint32_t i=0;i<queries.size();++i) tracker.Collect(queries[i],pool->getResults()[i] == 0);
  next_view.frame = 12; tracker.Begin(12);
  for (uint32_t i=0;i<queries.size();++i)
    Need(tracker.Request({12,34,5,i},next_view,bounds[i]) ==
        (i == 1 ? NativeOcclusionDecision::Occluded : NativeOcclusionDecision::Visible), "Fenced native consumer culling decision");
  uint32_t refresh = 0; tracker.Queries(next_view,[&](const auto &) { ++refresh; });
  Need(refresh == 3,"Culled native consumer still requests a visibility refresh");
  std::cout << "PASS native occlusion samples=" << samples << " rotated=" << rotated
      << " front=" << results[0] << " hidden=" << results[1] << " intersecting=" << results[2]
      << "; all colour/depth pixels preserved; two real fences -> hidden-only culling decision\n";
}
}
void CheckNativeOcclusion(RenderDevice &device) {
  const auto &limits = static_cast<VulkanDevice &>(device).physicalDeviceProperties.limits;
  for (uint32_t samples : {1u,2u,4u,8u}) {
    if (!(limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts & limits.framebufferStencilSampleCounts & samples)) {
      std::cout << "UNTESTED occlusion sample count " << samples << '\n'; continue;
    }
    Run(device,samples,false); Run(device,samples,true);
  }
}
