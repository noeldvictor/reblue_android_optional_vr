/**
 * @brief Real production rigid shaders, tiny stereo colour/depth readback; no files.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_rigid_program.h"
#include "gpu/scene/native_rigid_inputs.h"
#include "gpu/scene/native_rigid_batch.h"
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
  const bool cutout = (mode >= 12 && mode < 23) || mode == 40, untextured = mode == 10 || mode == 21;
  const bool shadow_cutout = mode >= 23, shadow_untextured = mode == 31 || mode == 36 || mode == 38;
  const bool cutout_receiver = mode >= 37;
  const bool overlap = mode >= 44;
  const float right_alpha = mode == 36 || mode == 41 ? .6f : mode == 42 ? .599f : mode == 43 ? .601f : .5f;
  // Canonical asset semantics, deliberately unrelated to translated locations.
  NativeMeshData mesh;
  mesh.streams.push_back({0,80,{}});
  mesh.attributes = {{MeshSemantic::Position,0,0}, {MeshSemantic::Normal,0,16},
                     {MeshSemantic::TexCoord,0,32}, {MeshSemantic::Color,0,48}, {MeshSemantic::TexCoord,2,64}};
  NativeVertexInputLibrary inputs;
  auto input = NativeRigidVertexInput(mesh, inputs, mode >= 6 && !untextured);
  Need(bool(input), "Native rigid input");
  auto programs = CreateNativeRigidPrograms(device, input);
  Need(programs.scene && programs.shadow && programs.shadow_cutout, "Production rigid shader programs");
  mesh = {}; input.reset(); // consuming owned inputs after source data retires

  auto world = Identity(); world[0] = 1.2f; world[5] = 1.25f; world[12] = .1f; world[13] = -.1f;
  const bool instanced = mode == 4 || mode == 11 || mode == 20 || mode == 32 || mode == 39 || overlap;
  const uint32_t flags = (untextured ? 0 : RigidAlbedo) | RigidVertexColour | RigidDiffuse | RigidSpecular |
      (mode == 1 || cutout_receiver || (instanced && !shadow_cutout) ? RigidReceiveShadow : 0) | (mode >= 2 ? RigidFogEnabled : 0);
  const uint32_t detail_layers = mode == 5 ? 1 : mode >= 6 && !untextured ? 2 : 0;
  const RigidFloat4 base_uv{.5f,.5f,mode == 7 || mode == 22 ? -2.f : .5f,.5f};
  const std::array<RigidFloat4,2> detail_uv{{{.75f,.5f,mode == 8 ? -2.f : 1.05f,.7f},
                                          {.6f,.65f,mode == 9 ? -2.f : 1.15f,.35f}}};
  const auto object = BuildRigidObject(world, {.5f,.75f,1,.8f}, {.1f,.2f,.15f,8}, base_uv, flags, detail_uv, detail_layers);
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
  pass.shadow_filter = {.001f,.001f,cutout_receiver ? .03125f : 0,0};
  pass.lights[0] = {LitVec(0,0,3), LitVec(0,0,-1), LitVec(.6f,.5f,.4f), .1f, .3f, 1,
                    mode == 2 ? LitPoint : mode == 3 ? LitSpot : LitDirectional};
  pass.lights[1] = {LitVec(2,0,3), LitVec(0,0,-1), LitVec(.1f,.05f,.2f), .05f,0,0,LitPoint};
  pass.fog[0] = {LitVec(0,0,0), LitVec(0,0,1), LitVec(.2f,.4f,.6f), 0,8,.3f,mode < 2,mode == 2,LitFogBlend};
  pass.fog[1] = {LitVec(0,0,0), LitVec(1,0,0), LitVec(.1f,.2f,.1f), -2,2,.2f,mode < 2,false,LitFogAdd};
  const auto packed_pass = BuildRigidPass(pass); Need(bool(packed_pass), "Native rigid pass inputs");
  const uint32_t instance_count = instanced ? 2 : 1;
  std::array<NativeRigidInstanceGPU,2> scene_instances, shadow_instances;
  std::array<NativeRigidPassInputs,2> references{pass,pass};
  scene_instances[0] = {*object,*packed_pass}; shadow_instances[0] = {*caster,*packed_pass};
  if (instanced) for (uint32_t n=0;n<2;++n) {
    auto transform = Identity(); transform[0] = .5f; transform[5] = n ? 1.1f : 1.f;
    transform[12] = n ? .5f : -.5f; transform[14] = n ? -.125f : 0;
    // Distinct worlds, colour/alpha, lights and fog within one GPU draw, in both
    // eyes. A shared final object's uniforms cannot satisfy the pixel oracle.
    references[n].lights[0].colour.x += .2f*n;
    references[n].fog[0].opacity += .15f*n;
    references[n].colour_grade.x += .1f*n;
    auto instance_uv = detail_uv;
    instance_uv[0].w += .27f*n; instance_uv[1].z -= .18f*n;
    scene_instances[n] = {*BuildRigidObject(transform,{.5f+.25f*n,.75f,1,.8f-.2f*n},
        {.1f,.2f,.15f,8},base_uv,flags,instance_uv,detail_layers),*BuildRigidPass(references[n])};
    transform[14] -= .25f;
    shadow_instances[n] = {*BuildRigidObject(transform,{1,1,1,1},{0,0,0,8},{1,1,0,0},0),scene_instances[n].pass_data};
  }
  if (cutout) for (uint32_t n=0;n<instance_count;++n) {
    // Exact alpha=1/.5 boundaries after vertex/object/base multiplication.
    // Detail image alpha must affect RGB only. Distinct per-instance references
    // catch accidentally batch-wide cutoffs in the real indirect draw.
    auto &data = scene_instances[n].object_data;
    data.diffuse.w = 2.f;
    Need(SetRigidCutout(data, mode == 20 && n ? 128 : mode == 22 || mode == 40 ? 64 : 255,
        mode < 20 ? mode-12 : RigidCutoutGE), "Rigid cutout pack");
  }
  if (shadow_cutout) for (uint32_t n=0;n<instance_count;++n) {
    // Poison irrelevant scene alpha/comparison inputs: none may change fixed
    // shadowmap coverage. Scene tests 0..22 still exercise all scene comparisons.
    auto &data = shadow_instances[n].object_data;
    data.diffuse.w = mode <= 30 ? float(mode-23)/8 : mode == 34 || ((mode == 32 || mode == 39) && n) ? 1.f : 2.f;
    data.flags.x = (shadow_untextured ? 0 : RigidAlbedo) | (mode == 34 ? 0 : RigidVertexColour);
    data.flags.y = shadow_untextured ? 0 : 1;
    data.uv_scale_offset = {.5f,.5f,mode == 33 ? -2.f : mode == 35 ? 1.5f : .5f+.25f*n,.5f};
    Need(SetRigidCutout(data, mode == 36 || mode == 38 ? 256 : (mode == 32 || mode == 39) && n ? 128 : 255,
        mode <= 30 ? mode-23 : RigidCutoutGE), "Rigid shadow cutout pack");
  }
  if (overlap) {
    // Overlapping near/far cutouts with complementary holes. Reverse actual
    // GPU instance order in mode45: holes must never erase an earlier caster.
    for (uint32_t n=0;n<2;++n) {
      auto &data = shadow_instances[n].object_data;
      // Both depths precede the nearer scene instance (.375), including bias.
      data.world = caster->world; data.world.rows[3].z = n ? -.1875f : -.25f;
      data.uv_scale_offset = {.5f,.5f,.5f+.5f*n,.5f};
    }
    if (mode == 45) std::swap(shadow_instances[0],shadow_instances[1]);
  }
  const auto alignment = static_cast<VulkanDevice &>(device).physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
  const auto placement = PlanNativeRigidStorage(instance_count,uint32_t(alignment));
  Need(bool(placement), "Rigid fixture storage alignment bound");
  const uint64_t scene_offset = placement->Offset(1), shadow_offset = placement->Offset(scene_offset+placement->bytes);
  std::vector<uint8_t> storage_bytes(shadow_offset+placement->bytes,0xCD); // poison prefix catches element/byte confusion
  std::memcpy(storage_bytes.data()+scene_offset,scene_instances.data(),placement->bytes);
  std::memcpy(storage_bytes.data()+shadow_offset,shadow_instances.data(),placement->bytes);
  auto storage = Upload(device,storage_bytes.data(),uint32_t(storage_bytes.size()),RenderBufferFlag::STORAGE);
  struct Vertex { RigidFloat4 position, normal, uv, colour, secondary_uv; };
  std::array<Vertex, 4> vertices{};
  const float xy[4][2]{{-1,-1},{1,-1},{1,1},{-1,1}};
  for (uint32_t i = 0; i < 4; ++i) vertices[i] = {{xy[i][0],xy[i][1],.5f,1},
      {.3f,.4f,1,0},{xy[i][0],xy[i][1],-xy[i][1],xy[i][0]},
      {.8f,.6f,.4f,.5f},{xy[i][1],-xy[i][0],0,0}};
  auto vb = Upload(device, vertices.data(), sizeof(vertices), RenderBufferFlag::VERTEX);
  const uint16_t indices[]{0,1,2,0,2,3};
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
  std::array<std::unique_ptr<RenderTexture>,2> details;
  std::array<std::unique_ptr<RenderTextureView>,2> detail_views;
  std::array<std::unique_ptr<RenderFramebuffer>,2> detail_fbs;
  for (uint32_t n=0;n<2;++n) {
    details[n] = device.createTexture(colour_desc); Need(bool(details[n]), "Rigid detail image");
    detail_views[n] = SampledArrayView(*details[n],colour_desc.format);
    const RenderTexture *pointer = details[n].get(); fb.colorAttachments = &pointer;
    detail_fbs[n] = device.createFramebuffer(fb); Need(bool(detail_fbs[n]), "Rigid detail framebuffer");
  }
  NativeRigidDescriptorSchema schema;
  std::array<std::unique_ptr<RenderDescriptorSet>, 3> sets;
  for (uint32_t i = 0; i < 3; ++i) { sets[i] = schema.sets[i].create(&device); Need(bool(sets[i]), "Rigid descriptors"); }
  auto shadow_set = schema.sets[0].create(&device); Need(bool(shadow_set), "Rigid shadow storage set");
  const RenderBufferStructuredView scene_view(sizeof(NativeRigidInstanceGPU),uint32_t(scene_offset/sizeof(NativeRigidInstanceGPU)));
  const RenderBufferStructuredView caster_view(sizeof(NativeRigidInstanceGPU),uint32_t(shadow_offset/sizeof(NativeRigidInstanceGPU)));
  sets[0]->setBuffer(0,storage.get(),placement->bytes,&scene_view);
  shadow_set->setBuffer(0,storage.get(),placement->bytes,&caster_view);
  // Match the real queue's legal inactive descriptors too: these point at the
  // owned depth image but must never supply a material sample for disabled layers.
  sets[1]->setTexture(0, untextured ? shadow.get() : albedo.get(), RenderTextureLayout::SHADER_READ,
      untextured ? shadow_view.get() : albedo_view.get());
  for (uint32_t n=0;n<2;++n)
    sets[1]->setTexture(n+1, n < detail_layers ? details[n].get() : shadow.get(), RenderTextureLayout::SHADER_READ,
        n < detail_layers ? detail_views[n].get() : shadow_view.get());
  sets[1]->setTexture(3, shadow.get(), RenderTextureLayout::SHADER_READ, shadow_view.get());
  RenderSamplerDesc sampler_desc;
  sampler_desc.minFilter = sampler_desc.magFilter = RenderFilter::NEAREST;
  sampler_desc.mipmapMode = RenderMipmapMode::NEAREST;
  sampler_desc.addressU = sampler_desc.addressV = sampler_desc.addressW = RenderTextureAddressMode::CLAMP;
  auto sampler = device.createSampler(sampler_desc);
  sampler_desc.addressU = sampler_desc.addressV = RenderTextureAddressMode::MIRROR;
  auto detail1_sampler = device.createSampler(sampler_desc);
  sampler_desc.addressU = sampler_desc.addressV = RenderTextureAddressMode::WRAP;
  auto detail2_sampler = device.createSampler(sampler_desc);
  sampler_desc.addressU = sampler_desc.addressV = RenderTextureAddressMode::CLAMP;
  if (cutout_receiver) sampler_desc.minFilter = sampler_desc.magFilter = RenderFilter::LINEAR;
  sampler_desc.comparisonEnabled = true; sampler_desc.comparisonFunc = RenderComparisonFunction::LESS_EQUAL;
  auto compare = device.createSampler(sampler_desc);
  Need(sampler && compare && detail1_sampler && detail2_sampler, "Rigid samplers");
  sets[2]->setSampler(0,sampler.get()); sets[2]->setSampler(1,detail1_sampler.get());
  sets[2]->setSampler(2,detail2_sampler.get()); sets[2]->setSampler(3,compare.get());
  std::unique_ptr<RenderDescriptorSet> caster_images, caster_samplers;
  if (shadow_cutout && !shadow_untextured) {
    caster_images = schema.sets[1].create(&device); caster_samplers = schema.sets[2].create(&device);
    Need(caster_images && caster_samplers, "Rigid caster material descriptors");
    for (uint32_t n=0;n<4;++n) {
      caster_images->setTexture(n,albedo.get(),RenderTextureLayout::SHADER_READ,albedo_view.get());
      caster_samplers->setSampler(n,detail2_sampler.get()); // phase1 wrap, not scene clamp
    }
  }
  const RenderInputSlot slot(0, sizeof(Vertex));
  RenderGraphicsPipelineDesc pipeline_desc;
  ApplyNativePipelineProgram(*programs.scene, pipeline_desc);
  pipeline_desc.inputSlots = &slot; pipeline_desc.inputSlotsCount = 1;
  pipeline_desc.renderTargetCount = 1; pipeline_desc.renderTargetFormat[0] = colour_desc.format;
  pipeline_desc.renderTargetBlend[0] = cutout ? RenderBlendDesc::AlphaBlend() : RenderBlendDesc::Copy();
  pipeline_desc.depthTargetFormat = RenderFormat::D32_FLOAT;
  pipeline_desc.depthEnabled = pipeline_desc.depthWriteEnabled = true;
  pipeline_desc.depthFunction = RenderComparisonFunction::LESS; pipeline_desc.cullMode = RenderCullMode::NONE;
  pipeline_desc.viewMask = 3;
  auto scene_pipeline = device.createGraphicsPipeline(pipeline_desc);
  const auto &shadow_program = shadow_cutout && !shadow_untextured ? programs.shadow_cutout : programs.shadow;
  ApplyNativePipelineProgram(*shadow_program, pipeline_desc);
  pipeline_desc.viewMask = 0; pipeline_desc.renderTargetCount = 0;
  pipeline_desc.depthTargetFormat = depth_desc.format;
  auto shadow_pipeline = device.createGraphicsPipeline(pipeline_desc);
  Need(scene_pipeline && static_cast<VulkanGraphicsPipeline *>(scene_pipeline.get())->vk &&
       shadow_pipeline && static_cast<VulkanGraphicsPipeline *>(shadow_pipeline.get())->vk, "Rigid native pipelines");
  const NativeRigidIndexedCommand indirect_command{6,instance_count,0,0,0};
  const std::array<NativeRigidIndexedCommand,2> indirect_commands{{{},indirect_command}};
  auto indirect = Upload(device,indirect_commands.data(),sizeof(indirect_commands),RenderBufferFlag::INDIRECT);
  auto queue = device.createCommandQueue(RenderCommandListType::DIRECT);
  auto commands = queue->createCommandList(); auto fence = device.createCommandFence();
  commands->begin();
  commands->setViewports(RenderViewport(0,0,size,size)); commands->setScissors(RenderRect(0,0,size,size));
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(albedo.get(),RenderTextureLayout::COLOR_WRITE));
  commands->setFramebuffer(albedo_fb.get());
  const RenderRect left(0,0,size/2,size), right(size/2,0,size,size);
  commands->clearColor(0,RenderColor(.5f,.25f,.75f,1),&left,1);
  commands->clearColor(0,RenderColor(.25f,.75f,.5f,right_alpha),&right,1);
  commands->setFramebuffer(nullptr);
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(albedo.get(),RenderTextureLayout::SHADER_READ));
  const std::array<std::array<RigidFloat4,4>,2> detail_colours{{
      {{{.8f,.1f,.2f,0},{.1f,.8f,.2f,.25f},{.2f,.1f,.8f,.75f},{.8f,.8f,.2f,1}}},
      {{{.9f,.1f,.2f,.6f},{.2f,.9f,.1f,.3f},{.1f,.2f,.9f,.8f},{.8f,.8f,.1f,0}}}}};
  for (uint32_t n=0;n<2;++n) {
    commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(details[n].get(),RenderTextureLayout::COLOR_WRITE));
    commands->setFramebuffer(detail_fbs[n].get());
    for (uint32_t q=0;q<4;++q) {
      const uint32_t x=(q&1)*size/2, y=(q>>1)*size/2;
      const RenderRect rect(x,y,x+size/2,y+size/2);
      const auto &c=detail_colours[n][q];
      commands->clearColor(0,RenderColor(c.x,c.y,c.z,c.w),&rect,1);
    }
    commands->setFramebuffer(nullptr);
    commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(details[n].get(),RenderTextureLayout::SHADER_READ));
  }
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(shadow.get(),RenderTextureLayout::DEPTH_WRITE));
  commands->setFramebuffer(shadow_fb.get()); commands->clearDepthStencil(true,true,1,0);
  GraphicsBindings bindings;
  // Production casters bind only their native instance storage. The depth-only VS
  // does not require otherwise declared image/sampler sets to be populated.
  bindings.layout = shadow_program->Layout(); bindings.set_count = 1;
  bindings.sets[0] = shadow_set.get();
  if (caster_images) {
    bindings.set_count = 3; bindings.sets[1] = caster_images.get(); bindings.sets[2] = caster_samplers.get();
  }
  GraphicsBindingState binding_state;
  Need(ApplyGraphicsBindings(*commands,bindings,binding_state), "Rigid shadow bind");
  commands->setPipeline(shadow_pipeline.get());
  const RenderVertexBufferView vertex_view({vb.get(),0},sizeof(vertices));
  const RenderIndexBufferView index_view({ib.get(),0},sizeof(indices),RenderFormat::R16_UINT);
  commands->setVertexBuffers(0,&vertex_view,1,&slot); commands->setIndexBuffer(&index_view);
  commands->drawIndexedIndirect(indirect.get(),sizeof(NativeRigidIndexedCommand),1,sizeof(NativeRigidIndexedCommand));
  commands->setFramebuffer(nullptr);
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(shadow.get(),RenderTextureLayout::SHADER_READ));
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(colour.get(),RenderTextureLayout::COLOR_WRITE));
  commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(depth.get(),RenderTextureLayout::DEPTH_WRITE));
  const float background[4]{.0625f,.125f,.25f,.5f};
  commands->setFramebuffer(scene_fb.get());
  commands->clearColor(0,cutout ? RenderColor(background[0],background[1],background[2],background[3]) : RenderColor(0,0,0,0));
  commands->clearDepthStencil(true,false,1,0);
  bindings.layout = programs.scene->Layout(); bindings.sets[0] = sets[0].get();
  bindings.set_count = 3;
  for (uint32_t n=1;n<3;++n) bindings.sets[n] = sets[n].get();
  Need(ApplyGraphicsBindings(*commands,bindings,binding_state), "Rigid scene bind");
  commands->setPipeline(scene_pipeline.get());
  commands->drawIndexedIndirect(indirect.get(),sizeof(NativeRigidIndexedCommand),1,sizeof(NativeRigidIndexedCommand));
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
  // Independent mono caster oracle. Use authored fixture values, not the GPU
  // readback or production cutout predicate, for both depth and receiver checks.
  const auto single_caster_depth = [&](uint32_t x, uint32_t n) {
    const auto &matrix = shadow_instances[n].object_data.world.rows;
    bool casts = true;
    if (shadow_cutout && !shadow_untextured) {
      const float source_x = (2*(x+.5f)/size-1-matrix[3].x)/matrix[0].x;
      const float offset = overlap ? .5f+.5f*(mode == 45 ? 1-n : n) :
          mode == 33 ? -2.f : mode == 35 ? 1.5f : .5f+.25f*n;
      const float u = source_x*.5f+offset;
      const float image_alpha = u-std::floor(u) >= .5f ? right_alpha : 1.f;
      casts = image_alpha >= .6f;
    }
    return casts ? .5f+matrix[3].z : 1.f;
  };
  const auto caster_depth = [&](uint32_t x) {
    return overlap ? (std::min)(single_caster_depth(x,0),single_caster_depth(x,1)) :
        single_caster_depth(x,instanced && x >= size/2 ? 1 : 0);
  };
  uint32_t lit_receivers = 0, shadowed_receivers = 0, filtered_receivers = 0;
  float maximum_error=0;
  for (uint32_t eye=0;eye<2;++eye) for (uint32_t y=0;y<size;++y) for (uint32_t x=0;x<size;++x) {
    const uint32_t n = instanced && x >= size/2 ? 1 : 0;
    const auto &reference = references[n];
    const auto &object_reference = scene_instances[n].object_data;
    const auto &matrix = object_reference.world.rows;
    const float world_z = .5f+matrix[3].z;
    const LitVector position=LitVec(2*(x+.5f)/size-1-(eye?.0625f:0),1-2*(y+.5f)/size,world_z);
    const LitVector normal=LitNormalize(LitVec(.3f/matrix[0].x,.4f/matrix[1].y,1));
    const auto camera=LitVec(reference.cameras[eye].x,reference.cameras[eye].y,reference.cameras[eye].z);
    const auto view=LitNormalize(LitSubtract(camera,position));
    const float asset_x=(position.x-matrix[3].x)/matrix[0].x, asset_y=(position.y-matrix[3].y)/matrix[1].y;
    const bool second=asset_x*.5f+base_uv.z>=.5f;
    auto texture=second?LitVec(.25f,.75f,.5f):LitVec(.5f,.25f,.75f);
    float texture_alpha=second?right_alpha:1.f;
    if (asset_x*.5f+base_uv.z < 0) { texture=LitVec(0,0,0); texture_alpha=0; }
    if (!object_reference.flags.y) { texture=LitVec(1,1,1); texture_alpha=1; }
    // Independent scalar oracle for UV-channel mapping, mirror/wrap addressing,
    // negative-U sentinels and ordered RGB overlays. Base alpha is never blended.
    for (uint32_t layer=1;layer<object_reference.flags.y;++layer) {
      const auto &uv=object_reference.detail_uv_scale_offset[layer-1];
      const float u=(layer==1?-asset_y:asset_y)*uv.x+uv.z;
      const float v=(layer==1?asset_x:-asset_x)*uv.y+uv.w;
      if (u < 0) continue;
      const auto address=[&](float value) {
        if (layer==2) return value-std::floor(value);
        const float repeat=value-2*std::floor(value/2);
        return repeat <= 1 ? repeat : 2-repeat;
      };
      const auto &detail=detail_colours[layer-1][(address(u)>=.5f?1:0)+(address(v)>=.5f?2:0)];
      texture.x += (detail.x-texture.x)*detail.w;
      texture.y += (detail.y-texture.y)*detail.w;
      texture.z += (detail.z-texture.z)*detail.w;
    }
    const auto &diffuse = object_reference.diffuse;
    float visibility = mode==1||(instanced&&!shadow_cutout)?0.f:1.f;
    if (cutout_receiver) {
      // Four independent bilinear comparison taps over analytic caster depths.
      // V is uniform in this fixture; both vertical taps have the same result.
      // Half-pixel eye displacement must affect the filtered edge, not pick an
      // eye layer from the mono shadow image or borrow scene depth as the oracle.
      visibility = 0;
      const float compare_z = position.z-.001f-(1-normal.z)*.001f;
      for (uint32_t tap=0;tap<4;++tap) {
        const float texel = (position.x*.5f+.5f+(tap&1 ? .03125f : -.03125f))*size-.5f;
        const int32_t left = int32_t(std::floor(texel));
        const float fraction = texel-left;
        const auto compare_at = [&](int32_t x) {
          return compare_z <= caster_depth(uint32_t(std::clamp(x,0,int32_t(size-1)))) ? 1.f : 0.f;
        };
        visibility += .25f*((1-fraction)*compare_at(left)+fraction*compare_at(left+1));
      }
      lit_receivers += visibility == 1;
      shadowed_receivers += visibility == 0;
      filtered_receivers += visibility > 0 && visibility < 1;
    }
    LitSurface surface{LitMultiply(texture,LitVec(.8f*diffuse.x,.6f*diffuse.y,.4f*diffuse.z)),LitVec(.1f,.2f,.15f),
      LitVec(.2f,.15f,.1f),LitVec(.15f,.2f,.1f),.5f,visibility,true,true};
    LitResponse response[3];
    for (uint32_t light=0;light<3;++light) response[light]=EvaluateLitLight(reference.lights[light],position,normal,view,8);
    auto expected=ComposeLitSurface(surface,reference.lights[0],reference.lights[1],reference.lights[2],response[0],response[1],response[2]);
    if(mode>=2) for(const auto &fog:reference.fog) expected=ApplyLitFog(expected,position,camera,fog);
    expected=LitScale(LitAdd(expected,LitVec(reference.colour_grade.x,.02f,.03f)),.9f);
    float rgba[]{expected.x,expected.y,expected.z,diffuse.w*.5f*texture_alpha};
    bool accepted = true;
    if (cutout) {
      const float threshold = mode == 20 && n ? 128.f/255 : mode == 22 || mode == 40 ? 64.f/255 : 1.f;
      const float alpha = rgba[3];
      // Independent oracle, not the production shader predicate or flags.
      switch (mode < 20 ? mode-12 : 0) {
      case 0: accepted = alpha >= threshold; break;
      case 1: accepted = false; break;
      case 2: accepted = alpha < threshold; break;
      case 3: accepted = alpha == threshold; break;
      case 4: accepted = alpha <= threshold; break;
      case 5: accepted = alpha > threshold; break;
      case 6: accepted = alpha < threshold || alpha > threshold; break;
      case 7: accepted = true; break;
      }
      for (uint32_t c=0;c<4;++c)
        rgba[c] = accepted ? rgba[c]*(c == 3 ? 1.f : alpha)+background[c]*(1-alpha) : background[c];
    }
    const auto pixel=eye*size*size+y*size+x;
    for(uint32_t c=0;c<4;++c) {
      const float actual=pixels[pixel*4+c], error=std::abs(actual-rgba[c]);
      if(!std::isfinite(actual)||error>.003f) {
        std::cerr<<"Rigid mode="<<mode<<" eye="<<eye<<" x="<<x<<" y="<<y<<" c="<<c<<" actual="<<actual<<" expected="<<rgba[c]<<'\n';
        throw std::runtime_error("Rigid colour mismatch");
      }
      maximum_error=(std::max)(maximum_error,error);
    }
    Need(std::abs(pixels[2*colour_bytes/4+pixel]-(accepted ? world_z*(eye?.5f:1.f) : 1.f))<1e-5f,"Rigid per-eye depth mismatch");
    Need(std::abs(pixels[(2*colour_bytes+2*depth_bytes)/4+y*size+x]-caster_depth(x))<1e-5f,"Rigid caster depth mismatch");
  }
  readback->unmap();
  if (cutout_receiver) {
    if (!shadow_untextured && right_alpha < .6f && !overlap) {
      Need(lit_receivers > 0,"Cutout holes must light receiver pixels");
      Need(shadowed_receivers > 0 && filtered_receivers > 0,"Cutout receiver needs both occluded pixels and filtered edges");
    } else {
      Need(shadowed_receivers == 128 && lit_receivers == 0 && filtered_receivers == 0,
          "Solid, threshold-passing or complementary casters must occlude both eyes completely");
    }
  }
  std::cout<<"PASS production native rigid shaders: mode="<<mode<<" eyes=2 pixels=128 max_error="<<maximum_error
           <<"; instances="<<instance_count<<" scene cutout="<<cutout<<" shadow cutout="<<shadow_cutout
           <<"; cutout receiver lit/shadowed/filtered="<<lit_receivers<<'/'<<shadowed_receivers<<'/'<<filtered_receivers
           <<"; native indexed indirect; nonzero storage/command offsets; sampled views=2D_ARRAY layer=0 shadow=D32_S8; raw bytes=0\n";
}
}
void CheckNativeRigid(plume::RenderDevice &device) { for(uint32_t mode=0;mode<46;++mode) Run(device,mode); }
