/**
 * @brief Real production rigid shaders, tiny stereo colour/depth readback; no files.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_rigid_program.h"
#include "gpu/scene/native_rigid_inputs.h"
#include "gpu/draw_bindings.h"
#include <plume_vulkan.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using namespace plume;
using namespace bd::gpu;
using namespace bd::gpu::scene;
constexpr uint32_t size = 8, colour_bytes = size*size*16, depth_bytes = size*size*4;
void Need(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
RenderMatrix Identity() { return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; }
std::unique_ptr<RenderTextureView> SampledArrayView(RenderTexture &image, RenderFormat format) {
  // Match NativeTextureGpu and NativeTargetImage, not the default plain-2D
  // view previously used by this fixture. The sun and ordinary albedo are mono.
  RenderTextureViewDesc view;
  view.format = format;
  view.dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  view.mipLevels = view.arraySize = 1;
  auto result = image.createTextureView(view);
  Need(result && static_cast<VulkanTextureView *>(result.get())->vk, "Rigid sampled array view");
  return result;
}
std::unique_ptr<RenderBuffer> Upload(RenderDevice &device, const void *data, uint32_t bytes, RenderBufferFlags flags) {
  auto result = device.createBuffer(RenderBufferDesc::UploadBuffer(bytes, flags));
  Need(bool(result), "Rigid buffer allocation");
  auto *mapped = result->map(); Need(mapped != nullptr, "Rigid buffer map");
  std::memcpy(mapped, data, bytes); result->unmap();
  auto *native = static_cast<VulkanBuffer *>(result.get());
  Need(vmaFlushAllocation(native->device->allocator, native->allocation, 0, VK_WHOLE_SIZE) == VK_SUCCESS, "Rigid upload flush");
  return result;
}
void Run(RenderDevice &device, uint32_t mode) {
  // Canonical asset semantics, deliberately unrelated to translated locations.
  NativeMeshData mesh;
  mesh.streams.push_back({0,64,{}});
  mesh.attributes = {{MeshSemantic::Position,0,0}, {MeshSemantic::Normal,0,16},
                     {MeshSemantic::TexCoord,0,32}, {MeshSemantic::Color,0,48}};
  NativeVertexInputLibrary inputs;
  auto input = NativeRigidVertexInput(mesh, inputs);
  Need(bool(input), "Native rigid input");
  auto programs = CreateNativeRigidPrograms(device, input);
  Need(programs.scene && programs.shadow, "Production rigid shader programs");
  mesh = {}; input.reset(); // consuming owned inputs after source data retires

  auto world = Identity(); world[0] = 1.2f; world[5] = 1.25f; world[12] = .1f; world[13] = -.1f;
  const uint32_t flags = RigidAlbedo | RigidVertexColour | RigidDiffuse | RigidSpecular |
      (mode == 1 ? RigidReceiveShadow : 0) | (mode >= 2 ? RigidFogEnabled : 0);
  const auto object = BuildRigidObject(world, {.5f,.75f,1,.8f}, {.1f,.2f,.15f,8}, {.5f,.5f,.5f,.5f}, flags);
  world[14] = -.25f;
  const auto caster = BuildRigidObject(world, {1,1,1,1}, {0,0,0,8}, {1,1,0,0}, 0);
  Need(object && caster, "Native rigid object inputs");
  NativeRigidPassInputs pass;
  pass.world_to_clip = {Identity(), Identity()};
  // Half-pixel separation keeps both eyes' samples strictly inside the shadow
  // frustum. Exactly-on-edge interpolation is not a stable occlusion oracle.
  pass.world_to_clip[1][12] = .0625f; pass.world_to_clip[1][10] = .5f;
  pass.world_to_shadow = Identity();
  pass.cameras = {{{0,0,2,0}, {4,1,2,0}}};
  pass.ambient = {.2f,.15f,.1f,0}; pass.colour_grade = {.01f,.02f,.03f,.9f};
  pass.shadow_colour_strength = {.15f,.2f,.1f,.5f};
  pass.shadow_filter = {.001f,.001f,0,0};
  pass.lights[0] = {LitVec(0,0,3), LitVec(0,0,-1), LitVec(.6f,.5f,.4f), .1f, .3f, 1,
                    mode == 2 ? LitPoint : mode == 3 ? LitSpot : LitDirectional};
  pass.lights[1] = {LitVec(2,0,3), LitVec(0,0,-1), LitVec(.1f,.05f,.2f), .05f,0,0,LitPoint};
  pass.fog[0] = {LitVec(0,0,0), LitVec(0,0,1), LitVec(.2f,.4f,.6f), 0,8,.3f,mode < 2,mode == 2,LitFogBlend};
  pass.fog[1] = {LitVec(0,0,0), LitVec(1,0,0), LitVec(.1f,.2f,.1f), -2,2,.2f,mode < 2,false,LitFogAdd};
  const auto packed_pass = BuildRigidPass(pass); Need(bool(packed_pass), "Native rigid pass inputs");
  const auto alignment = static_cast<VulkanDevice &>(device).physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
  const uint32_t stride = uint32_t((sizeof(NativeRigidPassGPU) + alignment - 1) / alignment * alignment);
  Need(stride && stride <= 65536, "Rigid fixture alignment bound");
  std::vector<uint8_t> uniform_bytes(stride * 4);
  std::memcpy(uniform_bytes.data()+stride, &*object, sizeof(*object));
  std::memcpy(uniform_bytes.data()+2*stride, &*caster, sizeof(*caster));
  std::memcpy(uniform_bytes.data()+3*stride, &*packed_pass, sizeof(*packed_pass));
  auto uniforms = Upload(device, uniform_bytes.data(), uint32_t(uniform_bytes.size()), RenderBufferFlag::CONSTANT);
  struct Vertex { RigidFloat4 position, normal, uv, colour; };
  std::array<Vertex, 3> vertices{};
  const float xy[3][2]{{-1,-1},{3,-1},{-1,3}};
  for (uint32_t i = 0; i < 3; ++i) vertices[i] = {{xy[i][0],xy[i][1],.5f,1},
      {.3f,.4f,1,0},{xy[i][0],xy[i][1],0,0},{.8f,.6f,.4f,.5f}};
  auto vb = Upload(device, vertices.data(), sizeof(vertices), RenderBufferFlag::VERTEX);
  const uint16_t indices[]{0,1,2};
  auto ib = Upload(device, indices, sizeof(indices), RenderBufferFlag::INDEX);
  auto colour_desc = RenderTextureDesc::ColorTarget(size,size,RenderFormat::R32G32B32A32_FLOAT);
  colour_desc.arraySize = 2;
  auto colour = device.createTexture(colour_desc);
  auto depth_desc = RenderTextureDesc::DepthTarget(size,size,RenderFormat::D32_FLOAT);
  depth_desc.arraySize = 2;
  auto depth = device.createTexture(depth_desc);
  depth_desc.arraySize = 1;
  depth_desc.format = RenderFormat::D32_FLOAT_S8_UINT;
  auto shadow = device.createTexture(depth_desc);
  colour_desc.arraySize = 1;
  auto albedo = device.createTexture(colour_desc);
  Need(colour && depth && shadow && albedo, "Rigid images");
  auto albedo_view = SampledArrayView(*albedo, colour_desc.format);
  auto shadow_view = SampledArrayView(*shadow, depth_desc.format);
  const RenderTexture *colour_ptr = colour.get(), *albedo_ptr = albedo.get();
  RenderFramebufferDesc fb;
  fb.colorAttachments = &colour_ptr; fb.colorAttachmentsCount = 1; fb.depthAttachment = depth.get(); fb.viewMask = 3;
  auto scene_fb = device.createFramebuffer(fb);
  fb = {}; fb.depthAttachment = shadow.get(); auto shadow_fb = device.createFramebuffer(fb);
  fb = {}; fb.colorAttachments = &albedo_ptr; fb.colorAttachmentsCount = 1;
  auto albedo_fb = device.createFramebuffer(fb);
  Need(scene_fb && shadow_fb && albedo_fb, "Rigid framebuffers");
  NativeRigidDescriptorSchema schema;
  std::array<std::unique_ptr<RenderDescriptorSet>, 3> sets;
  for (uint32_t i = 0; i < 3; ++i) { sets[i] = schema.sets[i].create(&device); Need(bool(sets[i]), "Rigid descriptors"); }
  sets[0]->setBuffer(0, uniforms.get(), sizeof(NativeRigidObjectGPU));
  sets[0]->setBuffer(1, uniforms.get(), sizeof(NativeRigidPassGPU));
  sets[1]->setTexture(0, albedo.get(), RenderTextureLayout::SHADER_READ, albedo_view.get());
  sets[1]->setTexture(1, shadow.get(), RenderTextureLayout::SHADER_READ, shadow_view.get());
  RenderSamplerDesc sampler_desc;
  sampler_desc.minFilter = sampler_desc.magFilter = RenderFilter::NEAREST;
  sampler_desc.mipmapMode = RenderMipmapMode::NEAREST;
  sampler_desc.addressU = sampler_desc.addressV = sampler_desc.addressW = RenderTextureAddressMode::CLAMP;
  auto sampler = device.createSampler(sampler_desc);
  sampler_desc.comparisonEnabled = true; sampler_desc.comparisonFunc = RenderComparisonFunction::LESS_EQUAL;
  auto compare = device.createSampler(sampler_desc);
  Need(sampler && compare, "Rigid samplers");
  sets[2]->setSampler(0,sampler.get()); sets[2]->setSampler(1,compare.get());
  const RenderInputSlot slot(0, sizeof(Vertex));
  RenderGraphicsPipelineDesc pipeline_desc;
  ApplyNativePipelineProgram(*programs.scene, pipeline_desc);
  pipeline_desc.inputSlots = &slot; pipeline_desc.inputSlotsCount = 1;
  pipeline_desc.renderTargetCount = 1; pipeline_desc.renderTargetFormat[0] = colour_desc.format;
  pipeline_desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
  pipeline_desc.depthTargetFormat = RenderFormat::D32_FLOAT;
  pipeline_desc.depthEnabled = pipeline_desc.depthWriteEnabled = true;
  pipeline_desc.depthFunction = RenderComparisonFunction::LESS; pipeline_desc.cullMode = RenderCullMode::NONE;
  pipeline_desc.viewMask = 3;
  auto scene_pipeline = device.createGraphicsPipeline(pipeline_desc);
  ApplyNativePipelineProgram(*programs.shadow, pipeline_desc);
  pipeline_desc.viewMask = 0; pipeline_desc.renderTargetCount = 0;
  pipeline_desc.depthTargetFormat = depth_desc.format;
  auto shadow_pipeline = device.createGraphicsPipeline(pipeline_desc);
  Need(scene_pipeline && static_cast<VulkanGraphicsPipeline *>(scene_pipeline.get())->vk &&
       shadow_pipeline && static_cast<VulkanGraphicsPipeline *>(shadow_pipeline.get())->vk, "Rigid native pipelines");
  auto queue = device.createCommandQueue(RenderCommandListType::DIRECT);
  auto commands = queue->createCommandList(); auto fence = device.createCommandFence();
  commands->begin();
  commands->setViewports(RenderViewport(0,0,size,size)); commands->setScissors(RenderRect(0,0,size,size));
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(albedo.get(),RenderTextureLayout::COLOR_WRITE));
  commands->setFramebuffer(albedo_fb.get());
  const RenderRect left(0,0,size/2,size), right(size/2,0,size,size);
  commands->clearColor(0,RenderColor(.5f,.25f,.75f,1),&left,1);
  commands->clearColor(0,RenderColor(.25f,.75f,.5f,.5f),&right,1);
  commands->setFramebuffer(nullptr);
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(albedo.get(),RenderTextureLayout::SHADER_READ));
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(shadow.get(),RenderTextureLayout::DEPTH_WRITE));
  commands->setFramebuffer(shadow_fb.get()); commands->clearDepthStencil(true,true,1,0);
  GraphicsBindings bindings;
  // Production casters bind only their two matrix blocks. The depth-only VS
  // does not require otherwise declared image/sampler sets to be populated.
  bindings.layout = programs.shadow->Layout(); bindings.set_count = 1;
  bindings.sets[0] = sets[0].get();
  bindings.dynamic_counts[0] = 2; bindings.offsets = {2*stride,3*stride};
  GraphicsBindingState binding_state;
  Need(ApplyGraphicsBindings(*commands,bindings,binding_state), "Rigid shadow bind");
  commands->setPipeline(shadow_pipeline.get());
  const RenderVertexBufferView vertex_view({vb.get(),0},sizeof(vertices));
  const RenderIndexBufferView index_view({ib.get(),0},sizeof(indices),RenderFormat::R16_UINT);
  commands->setVertexBuffers(0,&vertex_view,1,&slot); commands->setIndexBuffer(&index_view);
  commands->drawIndexedInstanced(3,1,0,0,0);
  commands->setFramebuffer(nullptr);
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(shadow.get(),RenderTextureLayout::SHADER_READ));
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(colour.get(),RenderTextureLayout::COLOR_WRITE));
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(depth.get(),RenderTextureLayout::DEPTH_WRITE));
  commands->setFramebuffer(scene_fb.get()); commands->clearColor(0,RenderColor(0,0,0,0));
  commands->clearDepthStencil(true,false,1,0);
  bindings.layout = programs.scene->Layout(); bindings.offsets[0] = stride;
  bindings.set_count = 3;
  for (uint32_t n=1;n<3;++n) bindings.sets[n] = sets[n].get();
  Need(ApplyGraphicsBindings(*commands,bindings,binding_state), "Rigid scene bind");
  commands->setPipeline(scene_pipeline.get()); commands->drawIndexedInstanced(3,1,0,0,0);
  commands->setFramebuffer(nullptr);
  auto readback = device.createBuffer(RenderBufferDesc::ReadbackBuffer(2*colour_bytes+3*depth_bytes));
  Need(bool(readback), "Rigid readback");
  auto copy = [&](RenderTexture *image,RenderFormat format,uint32_t layers,uint32_t offset,uint32_t bytes) {
    commands->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(image,RenderTextureLayout::COPY_SOURCE));
    for (uint32_t eye=0;eye<layers;++eye) commands->copyTextureRegion(
        RenderTextureCopyLocation::PlacedFootprint(readback.get(),format,size,size,1,size,offset+eye*bytes),
        RenderTextureCopyLocation::Subresource(image,0,eye));
  };
  copy(colour.get(),colour_desc.format,2,0,colour_bytes);
  copy(depth.get(),RenderFormat::D32_FLOAT,2,2*colour_bytes,depth_bytes);
  // Copy only D32 from the production D32/S8 shadow. The generic copy helper
  // selects both aspects for this format, which is not a valid buffer copy.
  commands->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(shadow.get(),RenderTextureLayout::COPY_SOURCE));
  VkBufferImageCopy shadow_copy{};
  shadow_copy.bufferOffset = 2*colour_bytes+2*depth_bytes;
  shadow_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  shadow_copy.imageSubresource.layerCount = 1;
  shadow_copy.imageExtent = {size,size,1};
  vkCmdCopyImageToBuffer(static_cast<VulkanCommandList *>(commands.get())->vk,
      static_cast<VulkanTexture *>(shadow.get())->vk, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      static_cast<VulkanBuffer *>(readback.get())->vk, 1, &shadow_copy);
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(static_cast<VulkanCommandList *>(commands.get())->vk,VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
  commands->end(); queue->executeCommandLists(commands.get(),fence.get());
  auto *native_fence=static_cast<VulkanCommandFence *>(fence.get());
  Need(vkWaitForFences(native_fence->device->vk,1,&native_fence->vk,VK_TRUE,5'000'000'000ULL)==VK_SUCCESS,"Rigid fence timeout");
  queue->waitForCommandFence(fence.get());
  auto *native_buffer=static_cast<VulkanBuffer *>(readback.get());
  Need(vmaInvalidateAllocation(native_buffer->device->allocator,native_buffer->allocation,0,VK_WHOLE_SIZE)==VK_SUCCESS,"Rigid invalidate");
  const auto *pixels=static_cast<const float *>(readback->map()); Need(pixels!=nullptr,"Rigid readback map");
  float maximum_error=0;
  for (uint32_t eye=0;eye<2;++eye) for (uint32_t y=0;y<size;++y) for (uint32_t x=0;x<size;++x) {
    const LitVector position=LitVec(2*(x+.5f)/size-1-(eye?.0625f:0),1-2*(y+.5f)/size,.5f);
    const LitVector normal=LitNormalize(LitVec(.3f/1.2f,.4f/1.25f,1));
    const auto camera=LitVec(pass.cameras[eye].x,pass.cameras[eye].y,pass.cameras[eye].z);
    const auto view=LitNormalize(LitSubtract(camera,position));
    const bool second=(position.x-.1f)/1.2f>=0;
    const auto texture=second?LitVec(.25f,.75f,.5f):LitVec(.5f,.25f,.75f);
    LitSurface surface{LitMultiply(texture,LitVec(.4f,.45f,.4f)),LitVec(.1f,.2f,.15f),
      LitVec(.2f,.15f,.1f),LitVec(.15f,.2f,.1f),.5f,mode==1?0.f:1.f,true,true};
    LitResponse response[3];
    for (uint32_t n=0;n<3;++n) response[n]=EvaluateLitLight(pass.lights[n],position,normal,view,8);
    auto expected=ComposeLitSurface(surface,pass.lights[0],pass.lights[1],pass.lights[2],response[0],response[1],response[2]);
    if(mode>=2) for(const auto &fog:pass.fog) expected=ApplyLitFog(expected,position,camera,fog);
    expected=LitScale(LitAdd(expected,LitVec(.01f,.02f,.03f)),.9f);
    const float rgba[]{expected.x,expected.y,expected.z,second?.2f:.4f};
    const auto pixel=eye*size*size+y*size+x;
    for(uint32_t c=0;c<4;++c) {
      const float actual=pixels[pixel*4+c], error=std::abs(actual-rgba[c]);
      if(!std::isfinite(actual)||error>.003f) {
        std::cerr<<"Rigid mode="<<mode<<" eye="<<eye<<" x="<<x<<" y="<<y<<" c="<<c<<" actual="<<actual<<" expected="<<rgba[c]<<'\n';
        throw std::runtime_error("Rigid colour mismatch");
      }
      maximum_error=(std::max)(maximum_error,error);
    }
    Need(std::abs(pixels[2*colour_bytes/4+pixel]-(eye?.25f:.5f))<1e-5f,"Rigid per-eye depth mismatch");
    Need(std::abs(pixels[(2*colour_bytes+2*depth_bytes)/4+y*size+x]-.25f)<1e-5f,"Rigid caster depth mismatch");
  }
  readback->unmap();
  std::cout<<"PASS production native rigid shaders: mode="<<mode<<" eyes=2 pixels=128 max_error="<<maximum_error
           <<"; sampled views=2D_ARRAY layer=0 shadow=D32_S8; native caster/receiver; raw bytes=0\n";
}
}
void CheckNativeRigid(plume::RenderDevice &device) { for(uint32_t mode=0;mode<4;++mode) Run(device,mode); }
