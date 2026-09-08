/**
 * @brief Current-depth compute -> real native indexed indirect draws and pixels.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/native_depth_visibility.h"
#include "gpu/native_occlusion_program.h"
#include "gpu/scene/native_scene_commands.h"
#include "src/gpu/shaders/hlsl/native_visibility_mask_ps.hlsl.spirv.h"
#include <plume_vulkan.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace plume;
using namespace bd::gpu;
using namespace bd::gpu::scene;
namespace {
void Need(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
std::shared_ptr<NativeTargetImage> Image(RenderDevice &device, uint32_t width, uint32_t height, uint32_t samples, bool depth) {
  auto result = std::make_shared<NativeTargetImage>();
  result->identity = depth ? 21 : 20;
  result->shape = {width,height,1,depth ? RenderFormat::D32_FLOAT_S8_UINT : RenderFormat::R16G16B16A16_FLOAT,samples};
  auto desc = depth ? RenderTextureDesc::DepthTarget(width,height,result->shape.format,RenderMultisampling(samples))
      : RenderTextureDesc::ColorTarget(width,height,result->shape.format,RenderMultisampling(samples));
  result->image = device.createTexture(desc);
  Need(result->image && static_cast<VulkanTexture *>(result->image.get())->vk,"Visibility image allocation");
  RenderTextureViewDesc view;
  view.format = result->shape.format; view.dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  view.mipLevels = view.arraySize = 1;
  result->view = result->image->createTextureView(view); result->descriptor = 0;
  Need(result->view && static_cast<VulkanTextureView *>(result->view.get())->vk,"Visibility sampled depth view");
  return result;
}
void Run(RenderDevice &device, const NativeDepthVisibilityProgramHandle &visibility, uint32_t samples, bool rotated, uint32_t mode) {
  const uint32_t width = rotated ? 33 : 32, height = rotated ? 25 : 24;
  auto queue = device.createCommandQueue(RenderCommandListType::DIRECT);
  auto cmd = queue->createCommandList(); auto fence = device.createCommandFence();
  auto color = Image(device,width,height,samples,false), depth = Image(device,width,height,samples,true);
  auto resolved_color = samples > 1 ? Image(device,width,height,1,false) : nullptr;
  auto resolved_depth = samples > 1 ? Image(device,width,height,1,true) : nullptr;
  const RenderTexture *source = color->image.get(), *resolved = resolved_color ? resolved_color->image.get() : nullptr;
  RenderFramebufferDesc framebuffer_desc;
  framebuffer_desc.colorAttachments = &source; framebuffer_desc.colorAttachmentsCount = 1;
  framebuffer_desc.depthAttachment = depth->image.get();
  if (samples > 1) {
    framebuffer_desc.colorResolveAttachments = &resolved;
    framebuffer_desc.depthResolveAttachment = resolved_depth->image.get(); framebuffer_desc.depthResolveMode = RenderResolveMode::MIN;
  }
  auto framebuffer = device.createFramebuffer(framebuffer_desc); Need(bool(framebuffer),"Visibility framebuffer");
  const std::array<SampledImage,2> resolves = samples > 1 ? std::array{resolved_color->Sampled(),resolved_depth->Sampled()}
      : std::array<SampledImage,2>{};
  const float initial_depth = mode == 1 || mode == 3 ? 1.f : .5f;
  auto scene = NativeSceneCommands::Create({color,depth},framebuffer.get(),resolves,{{2,.25f,.5f,1},initial_depth,0});
  Need(bool(scene),"Visibility scene command owner");
  const RenderMatrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
  const RenderMatrix camera = rotated ? RenderMatrix{0,0,1,0,0,1,0,0,-1,0,0,0,300,-200,-100,1}
      : RenderMatrix{1,0,0,0,0,1,0,0,0,0,1,0,-100,-200,-300,1};
  const uint32_t frame = 10+mode;
  scene->PublishCamera({identity,camera,identity},true,true,false,frame,3);
  const auto view = scene->OcclusionView(frame); Need(bool(view),"Visibility current owned view");
  auto work = NativeDepthVisibilityWork::Create(device,visibility,depth,*view,8);
  Need(bool(work),"Visibility work allocation");
  auto bad_view = *view; ++bad_view.scope.depth;
  Need(!NativeDepthVisibilityWork::Create(device,visibility,depth,bad_view,8),"Depth identity cannot be substituted");
  Need(!NativeDepthVisibilityWork::Create(device,visibility,depth,*view,0) &&
       !NativeDepthVisibilityWork::Create(device,visibility,depth,*view,4097),"Visibility bounded command allocation");
  auto proxy = CreateNativeOcclusionProgram(device);
  auto mask = device.createShader(g_native_visibility_mask_ps_spirv,sizeof(g_native_visibility_mask_ps_spirv),"main",RenderShaderFormat::SPIRV);
  Need(proxy && mask,"Visibility fixture raster shaders");
  RenderGraphicsPipelineDesc desc;
  desc.pipelineLayout = proxy.layout.get(); desc.vertexShader = proxy.vertex.get(); desc.pixelShader = proxy.pixel.get();
  desc.renderTargetCount = 1; desc.renderTargetFormat[0] = color->shape.format;
  desc.renderTargetBlend[0] = RenderBlendDesc::Copy(); desc.depthTargetFormat = depth->shape.format;
  desc.multisampling.sampleCount = samples; desc.cullMode = RenderCullMode::NONE;
  desc.depthEnabled = desc.depthWriteEnabled = false;
  auto paint = device.createGraphicsPipeline(desc);
  desc.pixelShader = mask.get(); desc.renderTargetBlend[0].renderTargetWriteMask = 0;
  desc.depthEnabled = desc.depthWriteEnabled = true; desc.depthFunction = RenderComparisonFunction::ALWAYS;
  auto mask_pipeline = device.createGraphicsPipeline(desc);
  Need(paint && mask_pipeline,"Visibility indirect raster pipelines");
  std::array<uint16_t,37> indices; for (uint32_t n=0;n<indices.size();++n) indices[n] = uint16_t(n);
  auto index = device.createBuffer(RenderBufferDesc::UploadBuffer(sizeof(indices),RenderBufferFlag::INDEX));
  Need(bool(index),"Visibility index buffer");
  std::memcpy(index->map(),indices.data(),sizeof(indices)); index->unmap();
  const RenderIndexBufferView index_view(index->at(0),sizeof(indices),RenderFormat::R16_UINT);
  const auto box = [&](float x,float y,float z,float extent=.06f) {
    const std::array<float,3> center = rotated ? std::array{100+z,200+y,300-x} : std::array{100+x,200+y,300+z};
    NativeBounds result;
    for (uint32_t n=0;n<3;++n) { result.min[n] = center[n]-extent; result.max[n] = center[n]+extent; }
    return result;
  };
  const std::array<std::optional<NativeBounds>,8> boxes{box(0,0,.75f),box(-.6f,0,.25f),box(0,0,.5f),
      std::nullopt,box(0,0,.000001f),box(0,0,1.01f),box(3,0,.75f),box(.98f,-.98f,.75f,.005f)};
  cmd->begin(); scene->Bind(*cmd); Need(scene->ApplyClear(*cmd),"Visibility native clear");
  cmd->setViewports(RenderViewport(0,0,float(width),float(height))); cmd->setScissors(RenderRect(0,0,width,height));
  if (mode == 2 || mode == 4) {
    const uint32_t x = mode == 2 ? width/2 : width-1, y = mode == 2 ? height/2 : height-1;
    const RenderRect hole(x,y,x+1,y+1); cmd->clearDepthStencil(true,false,1,0,&hole,1);
  } else if (mode == 3) {
    cmd->setGraphicsPipelineLayout(proxy.layout.get()); cmd->setPipeline(mask_pipeline.get());
    NativeOcclusionPacket coverage; coverage.world_to_clip = identity; coverage.center = {0,0,.5f,0}; coverage.extent = {2,2,0,0};
    cmd->setGraphicsPushConstants(0,&coverage,0,sizeof(coverage)); cmd->drawInstanced(36,1,0,0);
  }
  const NativeRigidIndexedCommand draw{36,2,1,-1,0};
  Need(!work->RecordCommand(*cmd,*view,boxes[0],draw),"Cannot cull before current depth");
  Need(work->RecordDepth(*cmd) && !work->RecordDepth(*cmd),"One immutable current-depth snapshot per owner");
  Need(depth->layout == RenderTextureLayout::DEPTH_WRITE,"Depth layout restored");
  bad_view = *view; ++bad_view.frame;
  Need(!work->RecordCommand(*cmd,bad_view,boxes[0],draw),"Old frame cannot consume pyramid");
  bad_view = *view; bad_view.camera.world_to_clip[12] += 1;
  Need(!work->RecordCommand(*cmd,bad_view,boxes[0],draw),"Changed camera needs its own current depth");
  auto wrong_cmd = queue->createCommandList();
  Need(!work->RecordCommand(*wrong_cmd,*view,boxes[0],draw),"Another command recording cannot consume pyramid");
  std::array<RenderBufferReference,8> generated;
  for (uint32_t n=0;n<boxes.size();++n) {
    auto command = draw; command.instance_count = n+2;
    const auto output = work->RecordCommand(*cmd,*view,boxes[n],command);
    Need(bool(output),"GPU native indirect command generation"); generated[n] = *output;
  }
  Need(!work->RecordCommand(*cmd,*view,boxes[0],draw),"Visibility command capacity is enforced");
  // Execute the GENERATED commands before any CPU readback. Depth testing is
  // disabled here so the hidden draw's absence cannot be mistaken for early-Z.
  scene->Bind(*cmd); cmd->setGraphicsPipelineLayout(proxy.layout.get()); cmd->setPipeline(paint.get());
  cmd->setViewports(RenderViewport(0,0,float(width),float(height))); cmd->setScissors(RenderRect(0,0,width,height));
  cmd->setIndexBuffer(&index_view);
  for (uint32_t n=0;n<2;++n) {
    const auto packet = PrepareNativeOcclusion(view->camera,*boxes[n]); Need(bool(packet),"Visibility painted bounds");
    cmd->setGraphicsPushConstants(0,&*packet,0,sizeof(*packet));
    cmd->drawIndexedIndirect(generated[n].ref,generated[n].offset,1,sizeof(NativeRigidIndexedCommand));
  }
  cmd->setFramebuffer(nullptr);
  const auto color_read = scene->ColorReadImage();
  const uint32_t command_bytes = 8*NativeDepthPyramid::kCommandStride, pyramid_bytes = work->Plan().bytes;
  const uint32_t color_offset = command_bytes+pyramid_bytes, color_bytes = width*height*8;
  auto readback = device.createBuffer(RenderBufferDesc::ReadbackBuffer(color_offset+color_bytes)); Need(bool(readback),"Visibility readback");
  cmd->barriers(RenderBarrierStage::COPY,RenderBufferBarrier(work->Commands(),RenderBufferAccess::READ));
  cmd->barriers(RenderBarrierStage::COPY,RenderBufferBarrier(work->Pyramid(),RenderBufferAccess::READ));
  cmd->copyBufferRegion(readback->at(0),work->Commands()->at(0),command_bytes);
  cmd->copyBufferRegion(readback->at(command_bytes),work->Pyramid()->at(0),pyramid_bytes);
  cmd->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(color_read.texture,RenderTextureLayout::COPY_SOURCE));
  *color_read.layout = RenderTextureLayout::COPY_SOURCE;
  cmd->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(readback.get(),color_read.format,width,height,1,width,color_offset),
      RenderTextureCopyLocation::Subresource(color_read.texture,0,0));
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER}; host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(static_cast<VulkanCommandList *>(cmd.get())->vk,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
      0,1,&host,0,nullptr,0,nullptr);
  cmd->end(); queue->executeCommandLists(cmd.get(),fence.get());
  auto *native_fence = static_cast<VulkanCommandFence *>(fence.get());
  Need(vkWaitForFences(native_fence->device->vk,1,&native_fence->vk,VK_TRUE,5'000'000'000ULL) == VK_SUCCESS,"Visibility fence timeout");
  auto *buffer = static_cast<VulkanBuffer *>(readback.get());
  Need(vmaInvalidateAllocation(buffer->device->allocator,buffer->allocation,0,VK_WHOLE_SIZE) == VK_SUCCESS,"Visibility readback invalidation");
  const auto *bytes = static_cast<const uint8_t *>(readback->map()); Need(bytes != nullptr,"Visibility readback map");
  const bool hidden = mode == 0 || mode == 4 || (mode == 3 && samples == 1);
  for (uint32_t n=0;n<boxes.size();++n) {
    NativeRigidIndexedCommand actual; std::memcpy(&actual,bytes+n*NativeDepthPyramid::kCommandStride,sizeof(actual));
    auto expected = draw; expected.instance_count = n+2;
    if ((n == 0 && hidden) || (n == 7 && (mode == 0 || mode == 2 || (mode == 3 && samples == 1)))) expected.instance_count = 0;
    if (std::memcmp(&actual,&expected,sizeof(actual))) {
      std::cerr << "mode=" << mode << " command=" << n << " instances=" << actual.instance_count << " expected=" << expected.instance_count << '\n';
      throw std::runtime_error("GPU visibility command differs");
    }
  }
  // Check EVERY pyramid cell against an independent CPU max reduction. The
  // sample-mask case must keep uncovered MSAA samples at1, not a MIN resolve.
  std::vector<float> previous(width*height,mode == 1 || (mode == 3 && samples > 1) ? 1.f : .5f);
  if (mode == 2) previous[(height/2)*width+width/2] = 1;
  if (mode == 4) previous[(height-1)*width+width-1] = 1;
  uint32_t prior_width = width, prior_height = height;
  for (uint32_t level=0;level<work->Plan().count;++level) {
    const auto &shape = work->Plan().levels[level]; std::vector<float> next(shape.width*shape.height);
    for (uint32_t y=0;y<shape.height;++y) for (uint32_t x=0;x<shape.width;++x) {
      float expected = 0;
      for (uint32_t oy=0;oy<2;++oy) for (uint32_t ox=0;ox<2;++ox)
        expected = (std::max)(expected,previous[(std::min)(2*y+oy,prior_height-1)*prior_width+(std::min)(2*x+ox,prior_width-1)]);
      float actual; std::memcpy(&actual,bytes+command_bytes+(shape.offset+y*shape.width+x)*4,4);
      Need(actual == expected,"Complete conservative pyramid differs"); next[y*shape.width+x] = expected;
    }
    previous = std::move(next); prior_width = shape.width; prior_height = shape.height;
  }
  const auto pixel = [&](uint32_t x,uint32_t y) { std::array<uint16_t,4> p; std::memcpy(p.data(),bytes+color_offset+(y*width+x)*8,8); return p; };
  const std::array<uint16_t,4> clear{0x4000,0x3400,0x3800,0x3c00}, black{};
  Need(pixel(width/2,height/2) == (hidden ? clear : black),"Actual GPU-generated hidden command pixel");
  Need(pixel(uint32_t(.2f*width),height/2) == black,"Actual GPU-generated visible command pixel");
  Need(pixel(width-1,0) == clear,"Visibility indirect draw changed unrelated colour");
  readback->unmap();
  std::cout << "PASS native current-depth visibility samples=" << samples << " rotated/odd=" << rotated << " mode=" << mode
      << "; full max pyramid, GPU indirect command fields and executed hidden/visible pixels; no temporal history\n";
}
}
void CheckNativeDepthVisibility(RenderDevice &device) {
  auto program = CreateNativeDepthVisibilityProgram(device); Need(bool(program),"Current-depth compute programs");
  Need(!NativeDepthPyramid::Plan({0,1,1,RenderFormat::D32_FLOAT_S8_UINT,1}),"Zero shape refused");
  Need(!NativeDepthPyramid::Plan({1,1,2,RenderFormat::D32_FLOAT_S8_UINT,1}),"No mono cull applied to both eyes");
  Need(!NativeDepthPyramid::Plan({8192,8192,1,RenderFormat::D32_FLOAT_S8_UINT,1}),"Pyramid byte limit before allocation");
  const auto &limits = static_cast<VulkanDevice &>(device).physicalDeviceProperties.limits;
  for (uint32_t samples : {1u,2u,4u,8u}) {
    if (!(limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts & limits.framebufferStencilSampleCounts & samples)) {
      std::cout << "UNTESTED visibility sample count " << samples << '\n'; continue;
    }
    for (bool rotated : {false,true}) for (uint32_t mode=0;mode<5;++mode) Run(device,program,samples,rotated,mode);
  }
}
