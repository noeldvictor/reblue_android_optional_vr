/**
 * @brief Production native water shaders and owned inputs; tiny in-memory stereo readback.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_water_program.h"
#include "gpu/scene/native_water_material_source.h"
#include "gpu/scene/native_scene_snapshot.h"
#include "gpu/draw_bindings.h"
#include <plume_vulkan.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <map>

namespace {
using namespace plume;
using namespace bd::gpu;
using namespace bd::gpu::scene;
constexpr uint32_t size = 8, pixels_per_eye = size*size, colour_bytes = pixels_per_eye*8;
void Need(bool valid, const char *message) { if (!valid) throw std::runtime_error(message); }
RenderMatrix Identity() { return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; }
bool HalfStoreMatches(float actual, float expected) {
  // An attachment store can select either bracketing binary16 value. Match
  // the representation exactly, not a tuned decimal epsilon. In particular
  // .890577 has a 2^-11 quantum, larger than the low-colour fixture tolerance.
  if (!std::isfinite(actual) || !std::isfinite(expected) || std::abs(expected) > 65504) return false;
  if (!expected) return actual == 0;
  const float quantum = std::ldexp(1.f,(std::max)(-24,std::ilogb(std::abs(expected))-10));
  return actual == std::floor(expected/quantum)*quantum || actual == std::ceil(expected/quantum)*quantum;
}
NativeRigidPassInputs Pass() {
  NativeRigidPassInputs pass;
  pass.world_to_clip = {Identity(),Identity()}; pass.world_to_shadow = Identity();
  pass.world_to_clip[1][12] = .125f; pass.world_to_clip[1][10] = .75f;
  pass.cameras = {{{0,0,3,0},{1,0,3,0}}};
  pass.colour_grade = {0,0,0,1}; pass.ambient = {.2f,.3f,.4f,0};
  for (auto &fog : pass.fog) { fog.disabled = true; fog.blend = LitFogBlend; }
  return pass;
}
void CheckImport() {
  Need(HalfStoreMatches(.89013671875f,.890577f) && HalfStoreMatches(.890625f,.890577f) &&
      !HalfStoreMatches(.8896484375f,.890577f) && !HalfStoreMatches(.89111328125f,.890577f),
      "Water HDR16 oracle accepts only the two bracketing representations");
  constexpr uint32_t material = 0x10000, owner = 0x20000, buffer = 0x30000;
  const uint32_t descriptors[]{4768,4788,4808,4828,4848,4868,4888,4908,4928,
      4960,4980,5000,5020,5040,5060,5080,5100,5120,5140,5160,5180};
  std::map<uint64_t,uint32_t> words;
  const auto bits = [](float value) { return std::bit_cast<uint32_t>(value); };
  for (uint32_t i = 0; i < std::size(descriptors); ++i) {
    words[material+descriptors[i]+4] = owner;
    words[material+descriptors[i]+12] = i;
    words[material+descriptors[i]+16] = 0;
    for (uint32_t j = 0; j < 4; ++j) words[buffer+i*16+j*4] = bits(float(i+j));
  }
  words[owner+12] = buffer;
  auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    const auto it = words.find(address); return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  const auto initial = ReadNativeWaterMaterial(material,read);
  Need(initial && initial->tint.x == 4 && initial->tint.w == 7 && initial->wave_direction == 7 &&
      initial->highlight == 16 && initial->shore_gain == 20, "Water semantic destination mapping");
  // A later writer aliases a publication into another parameter. Re-read the
  // completed destination; the already retained value must not change.
  words[material+descriptors[0]+12] = 3;
  words[buffer+3*16] = bits(.75f);
  auto changed = ReadNativeWaterMaterial(material,read);
  Need(changed && changed->scroll_u == .75f && changed->phase == .75f && initial->phase == 3,
      "Water late alias and retained material copy");
  words.erase(buffer+20*16);
  Need(!ReadNativeWaterMaterial(material,read), "Water truncated final parameter refuses");
  words.clear(); // no source storage survives GPU packing/consumption
  const std::array bottom{Identity(),Identity()};
  Need(bool(BuildNativeWaterInstance(Identity(),*changed,Pass(),bottom,{1,2,1},0)), "Retired source water packing");
  Need(!BuildNativeWaterInstance(Identity(),*changed,Pass(),bottom,{1,0,1},0), "Water invalid image layer count");
  Need(!BuildNativeWaterInstance(Identity(),*changed,Pass(),bottom,{1,1,1},WaterRefraction), "Material owns refraction flag");
  auto bad = *changed; bad.shininess = -1;
  Need(!BuildNativeWaterInstance(Identity(),bad,Pass(),bottom,{1,1,1},0), "Water negative shininess");
  bad = *changed; bad.phase = std::numeric_limits<float>::infinity();
  Need(!BuildNativeWaterInstance(Identity(),bad,Pass(),bottom,{1,1,1},0), "Water nonfinite phase");
  auto singular = Identity(); singular[0] = 0;
  Need(!BuildNativeWaterInstance(singular,*changed,Pass(),bottom,{1,1,1},0), "Water singular transform");
}
std::unique_ptr<RenderBuffer> Upload(RenderDevice &device, const void *data, uint32_t bytes, RenderBufferFlags flags) {
  auto buffer = device.createBuffer(RenderBufferDesc::UploadBuffer(bytes,flags));
  Need(bool(buffer), "Water upload allocation");
  auto *mapped = buffer->map(); Need(mapped != nullptr,"Water upload map");
  std::memcpy(mapped,data,bytes); buffer->unmap();
  auto &native = *static_cast<VulkanBuffer *>(buffer.get());
  Need(vmaFlushAllocation(native.device->allocator,native.allocation,0,VK_WHOLE_SIZE) == VK_SUCCESS,"Water upload flush");
  return buffer;
}
struct Image {
  std::shared_ptr<NativeTargetImage> owner;
  std::unique_ptr<RenderTextureView> view;
  std::vector<std::unique_ptr<RenderTextureView>> layer_views;
  std::vector<std::unique_ptr<RenderFramebuffer>> framebuffers;
};
Image MakeImage(RenderDevice &device, uint32_t layers, bool cube = false, bool depth = false) {
  Image result; result.owner = std::make_shared<NativeTargetImage>();
  const auto format = depth ? RenderFormat::D32_FLOAT_S8_UINT : RenderFormat::R16G16B16A16_FLOAT;
  auto desc = depth ? RenderTextureDesc::DepthTarget(size,size,format) :
      RenderTextureDesc::ColorTarget(size,size,format,{},nullptr,cube ? RenderTextureFlag::CUBE : RenderTextureFlag::NONE);
  desc.arraySize = layers;
  result.owner->shape = {size,size,layers,format,1}; result.owner->descriptor = 0;
  result.owner->image = device.createTexture(desc);
  Need(result.owner->image && static_cast<VulkanTexture *>(result.owner->image.get())->vk,"Water image allocation");
  RenderTextureViewDesc view;
  view.format = format; view.dimension = cube ? RenderTextureViewDimension::TEXTURE_CUBE : RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  view.mipLevels = 1; view.arraySize = layers;
  result.view = result.owner->image->createTextureView(view);
  Need(result.view && static_cast<VulkanTextureView *>(result.view.get())->vk,"Water sampled image view");
  for (uint32_t layer = 0; layer < layers; ++layer) {
    view.dimension = RenderTextureViewDimension::TEXTURE_2D; view.arraySize = 1; view.arrayIndex = layer;
    result.layer_views.push_back(result.owner->image->createTextureView(view));
    const auto *image = result.owner->image.get(); const auto *layer_view = result.layer_views.back().get();
    RenderFramebufferDesc fb;
    if (depth) { fb.depthAttachment = image; fb.depthAttachmentView = layer_view; }
    else { fb.colorAttachments = &image; fb.colorAttachmentViews = &layer_view; fb.colorAttachmentsCount = 1; }
    result.framebuffers.push_back(device.createFramebuffer(fb));
    Need(layer_view && result.framebuffers.back(),"Water layer framebuffer");
  }
  return result;
}
std::vector<float> Run(RenderDevice &device, uint32_t mode, float phase = 0) {
  NativeMeshData mesh; mesh.streams.push_back({0,80,{}});
  mesh.attributes = {{MeshSemantic::Position,0,0},{MeshSemantic::Normal,0,16},{MeshSemantic::TexCoord,0,32},
                     {MeshSemantic::Color,0,48},{MeshSemantic::Tangent,0,64}};
  NativeVertexInputLibrary library;
  auto input = NativeWaterVertexInput(mesh,library); Need(bool(input),"Water named mesh input");
  auto program = CreateNativeWaterProgram(device,input); Need(bool(program),"Production water program");
  mesh.attributes.pop_back(); Need(!NativeWaterVertexInput(mesh,library),"Missing water tangent refuses");
  mesh = {}; input.reset(); // the production program retains its immutable input
  std::array<NativeWaterInstanceGPU,3> instances{};
  std::array<NativeWaterMaterial,2> materials;
  auto pass = Pass();
  if (mode == 5) {
    pass.lights[0] = {LitVec(0,0,3),LitVec(0,0,-1),LitVec(.4f,.5f,.6f),0,0,0,LitDirectional};
    pass.fog[0] = {LitVec(0,0,0),LitVec(0,0,1),LitVec(.1f,.2f,.3f),0,1,.5f,false,false,LitFogAdd};
  }
  const bool lit = mode >= 8 && mode <= 11, blend = mode == 12 || mode == 13;
  if (lit) {
    pass.lights[0] = {LitVec(.5f,0,3),LitVec(0,0,-1),LitVec(.2f,.3f,.1f),.1f,.25f,1,
                     mode == 9 ? LitSpot : mode == 10 ? LitPoint : LitDirectional};
    pass.lights[1] = {LitVec(-1,1,2),LitVec(0,0,-1),LitVec(.05f,.1f,.15f),.1f,0,0,LitPoint};
    if (mode == 11) {
      pass.fog[0] = {LitVec(0,0,0),LitVec(1,0,0),LitVec(.2f,.1f,.3f),0,5,.25f,false,true,LitFogBlend};
      pass.fog[1] = {LitVec(0,0,0),LitVec(1,0,0),LitVec(.1f,.2f,.1f),-1,1,.1f,false,false,LitFogSubtract};
    }
  }
  for (uint32_t i = 0; i < 2; ++i) {
    auto &m = materials[i]; m.tint = {.2f+.2f*i,.3f,.4f,.4f+.2f*i};
    m.uv_scale = .2f; m.scroll_u = .3f; m.scroll_v = -.2f; m.phase = phase;
    m.reflection = mode == 2 ? WaterReflectionEnvironment : mode == 1 || mode == 6 || mode == 7 ? WaterReflectionPlanar : WaterReflectionNone;
    m.refraction = mode == 3 || mode == 4; m.shore = mode == 4;
    m.depth_opacity = 1; m.shininess = 8; m.highlight = mode == 5 || lit ? .2f : 0;
    m.reflection_distortion = mode == 6 ? .2f : 0;
    m.normal_blend = mode == 6 ? .5f : 0;
    m.wave_amplitude = mode == 7 ? .15f : 0; m.wave_frequency = 4; m.wave_speed = 1;
    auto world = Identity(); world[0] = .5f; world[12] = i ? .5f : -.5f;
    auto bottom = std::array{Identity(),Identity()}; bottom[1][14] = .125f;
    auto instance_pass = pass;
    if (lit) instance_pass.lights[1].colour.x += .1f*i; // not batch-wide lighting
    auto packed = BuildNativeWaterInstance(world,m,instance_pass,bottom,{2,2,2},
        mode == 5 ? WaterDiffuse|WaterFog|WaterShadow : lit ? WaterDiffuse|WaterFog : 0);
    Need(bool(packed),"Water instance packing"); instances[i+1] = *packed;
  }
  auto storage = Upload(device,instances.data(),sizeof(instances),RenderBufferFlag::STORAGE);
  struct Vertex { RigidFloat4 position,normal,uv,colour,tangent; };
  std::array<Vertex,4> vertices;
  const float xy[4][2]{{-1,-1},{1,-1},{1,1},{-1,1}};
  for (uint32_t i = 0; i < 4; ++i) vertices[i] = {{xy[i][0],xy[i][1],.5f,1},{0,0,1,0},
      {xy[i][0],xy[i][1],0,0},{1,1,1,1},{1,0,0,0}};
  const uint16_t indices[]{0,1,2,0,2,3};
  auto vb = Upload(device,vertices.data(),sizeof(vertices),RenderBufferFlag::VERTEX);
  auto ib = Upload(device,indices,sizeof(indices),RenderBufferFlag::INDEX);
  struct Indexed { uint32_t count, instances, first; int32_t base; uint32_t instance; };
  const Indexed indexed{6,2,0,0,1}; // poison instance zero detects ignored firstInstance
  auto indirect = Upload(device,&indexed,sizeof(indexed),RenderBufferFlag::INDIRECT);
  auto colour = MakeImage(device,2), depth = MakeImage(device,2,false,true);
  std::array<Image,6> images{MakeImage(device,1),MakeImage(device,2),MakeImage(device,2),
      MakeImage(device,2),MakeImage(device,6,true),MakeImage(device,1,false,true)};
  NativeWaterDescriptorSchema schema;
  std::array<std::unique_ptr<RenderDescriptorSet>,3> sets;
  for (uint32_t i = 0; i < 3; ++i) { sets[i] = schema.sets[i].create(&device); Need(bool(sets[i]),"Water descriptors"); }
  const RenderBufferStructuredView storage_view(sizeof(NativeWaterInstanceGPU));
  sets[0]->setBuffer(0,storage.get(),sizeof(instances),&storage_view);
  RenderSamplerDesc sampler_desc;
  sampler_desc.minFilter = sampler_desc.magFilter = RenderFilter::NEAREST; sampler_desc.mipmapMode = RenderMipmapMode::NEAREST;
  sampler_desc.addressU = sampler_desc.addressV = sampler_desc.addressW = RenderTextureAddressMode::CLAMP;
  auto sampler = device.createSampler(sampler_desc);
  sampler_desc.addressU = sampler_desc.addressV = RenderTextureAddressMode::WRAP;
  auto bump_sampler = device.createSampler(sampler_desc);
  sampler_desc.comparisonEnabled = true; sampler_desc.comparisonFunc = RenderComparisonFunction::LESS_EQUAL;
  auto sun_sampler = device.createSampler(sampler_desc);
  Need(sampler && bump_sampler && sun_sampler,"Water samplers");
  for (uint32_t i = 0; i < 6; ++i) {
    sets[1]->setTexture(i,images[i].owner->image.get(),RenderTextureLayout::SHADER_READ,images[i].view.get());
    sets[2]->setSampler(i,i == 5 ? sun_sampler.get() : i == 0 ? bump_sampler.get() : sampler.get());
  }
  const auto *target = colour.owner->image.get();
  RenderFramebufferDesc fb; fb.colorAttachments = &target; fb.colorAttachmentsCount = 1;
  fb.depthAttachment = depth.owner->image.get(); fb.viewMask = 3;
  auto framebuffer = device.createFramebuffer(fb); Need(bool(framebuffer),"Water scene framebuffer");
  auto scene = NativeSceneCommands::Create({colour.owner,depth.owner},framebuffer.get(),{},{{0,0,0,1},1,0});
  Need(bool(scene),"Water native scene command owner");
  RenderGraphicsPipelineDesc pipeline_desc; ApplyNativePipelineProgram(*program,pipeline_desc);
  const RenderInputSlot slot(0,sizeof(Vertex));
  pipeline_desc.inputSlots = &slot; pipeline_desc.inputSlotsCount = 1;
  pipeline_desc.renderTargetCount = 1; pipeline_desc.renderTargetFormat[0] = colour.owner->shape.format;
  pipeline_desc.renderTargetBlend[0] = blend ? RenderBlendDesc::AlphaBlend() : RenderBlendDesc::Copy();
  pipeline_desc.depthTargetFormat = depth.owner->shape.format; pipeline_desc.depthEnabled = pipeline_desc.depthWriteEnabled = true;
  pipeline_desc.depthWriteEnabled = mode != 12;
  pipeline_desc.depthFunction = RenderComparisonFunction::LESS; pipeline_desc.cullMode = RenderCullMode::NONE; pipeline_desc.viewMask = 3;
  auto pipeline = device.createGraphicsPipeline(pipeline_desc);
  Need(pipeline && static_cast<VulkanGraphicsPipeline *>(pipeline.get())->vk,"Water native pipeline");
  auto queue = device.createCommandQueue(RenderCommandListType::DIRECT);
  auto commands = queue->createCommandList(); auto fence = device.createCommandFence();
  commands->begin(); commands->setViewports(RenderViewport(0,0,size,size)); commands->setScissors(RenderRect(0,0,size,size));
  const RenderRect full(0,0,size,size);
  for (uint32_t i = 0; i < 6; ++i) {
    if (i == 2) continue; // the real native scene snapshot produces this image
    auto &image = images[i];
    const auto layout = i == 5 ? RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::COLOR_WRITE;
    commands->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(image.owner->image.get(),layout));
    image.owner->layout = layout;
    for (uint32_t layer = 0; layer < image.framebuffers.size(); ++layer) {
      commands->setFramebuffer(image.framebuffers[layer].get());
      if (i == 5) commands->clearDepthStencil(true,false,.25f,0);
      else {
        // Exact binary16 source colours isolate the sampled-image route from
        // implementation-permitted clear/store rounding at format conversion.
        const auto value = i == 0 ? RenderColor(.5f,.5f,1,1) : i == 1 ? RenderColor(.25f+.25f*layer,.125f,.0625f,1) :
            i == 3 ? RenderColor(.75f+.125f*layer,0,0,1) : RenderColor(.5f,.375f,.25f,1);
        commands->clearColor(0,value,&full,1);
        if (mode == 6 && i == 0) {
          const RenderRect half(0,0,size/2,size);
          commands->clearColor(0,RenderColor(.875f,.25f,.75f,1),&half,1);
        }
      }
    }
    commands->setFramebuffer(nullptr);
    commands->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(image.owner->image.get(),RenderTextureLayout::SHADER_READ));
    image.owner->layout = RenderTextureLayout::SHADER_READ;
  }
  scene->Bind(*commands); Need(scene->ApplyClear(*commands),"Water scene initial clear");
  for (uint32_t eye = 0; eye < 2; ++eye) {
    commands->setFramebuffer(colour.framebuffers[eye].get());
    commands->clearColor(0,RenderColor(.125f,.25f+.25f*eye,.5f,1),&full,1);
  }
  scene->Bind(*commands);
  Need(CopySceneSnapshot(*commands,*scene,images[2].owner->Sampled()),"Water ordered native snapshot");
  scene->Bind(*commands); Need(!scene->ApplyClear(*commands),"Water snapshot resume must not clear");
  commands->clearColor(0,blend ? RenderColor(.125f,.25f,.5f,1) : RenderColor(9,8,7,1)); // must NOT change retained snapshot samples
  GraphicsBindings bindings; bindings.layout = program->Layout(); bindings.set_count = 3;
  for (uint32_t i = 0; i < 3; ++i) bindings.sets[i] = sets[i].get();
  GraphicsBindingState binding_state; Need(ApplyGraphicsBindings(*commands,bindings,binding_state),"Water explicit bindings");
  commands->setPipeline(pipeline.get());
  const RenderVertexBufferView vertex_view({vb.get(),0},sizeof(vertices));
  const RenderIndexBufferView index_view({ib.get(),0},sizeof(indices),RenderFormat::R16_UINT);
  commands->setVertexBuffers(0,&vertex_view,1,&slot); commands->setIndexBuffer(&index_view);
  commands->drawIndexedIndirect(indirect.get(),0,1,sizeof(Indexed));
  commands->setFramebuffer(nullptr);
  auto readback = device.createBuffer(RenderBufferDesc::ReadbackBuffer(2*colour_bytes+2*pixels_per_eye*4));
  Need(bool(readback),"Water readback allocation");
  for (uint32_t i = 0; i < 2; ++i) {
    const auto &image = i ? depth.owner : colour.owner;
    commands->barriers(RenderBarrierStage::COPY,RenderTextureBarrier(image->image.get(),RenderTextureLayout::COPY_SOURCE));
    for (uint32_t eye = 0; eye < 2; ++eye) {
      if (i) {
        // Read only depth, not a combined depth/stencil buffer-copy aspect.
        VkBufferImageCopy copy{}; copy.bufferOffset = 2*colour_bytes+eye*pixels_per_eye*4;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        copy.imageSubresource.baseArrayLayer = eye; copy.imageSubresource.layerCount = 1;
        copy.imageExtent = {size,size,1};
        vkCmdCopyImageToBuffer(static_cast<VulkanCommandList *>(commands.get())->vk,
            static_cast<VulkanTexture *>(image->image.get())->vk,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            static_cast<VulkanBuffer *>(readback.get())->vk,1,&copy);
      } else commands->copyTextureRegion(
          RenderTextureCopyLocation::PlacedFootprint(readback.get(),image->shape.format,size,size,1,size,eye*colour_bytes),
          RenderTextureCopyLocation::Subresource(image->image.get(),0,eye));
    }
  }
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER}; host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(static_cast<VulkanCommandList *>(commands.get())->vk,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
      0,1,&host,0,nullptr,0,nullptr);
  commands->end(); queue->executeCommandLists(commands.get(),fence.get());
  auto &native_fence = *static_cast<VulkanCommandFence *>(fence.get());
  Need(vkWaitForFences(native_fence.device->vk,1,&native_fence.vk,VK_TRUE,5'000'000'000ULL) == VK_SUCCESS,"Water fence timeout");
  queue->waitForCommandFence(fence.get());
  auto &native_buffer = *static_cast<VulkanBuffer *>(readback.get());
  Need(vmaInvalidateAllocation(native_buffer.device->allocator,native_buffer.allocation,0,VK_WHOLE_SIZE) == VK_SUCCESS,"Water readback invalidate");
  const auto *mapped = static_cast<const uint16_t *>(readback->map()); Need(mapped != nullptr,"Water readback map");
  std::vector<float> pixels(pixels_per_eye*10);
  for (uint32_t i = 0; i < pixels_per_eye*8; ++i) {
    const uint32_t word = mapped[i], exponent = (word>>10)&31, mantissa = word&1023;
    const float value = exponent == 31 ? (mantissa ? NAN : INFINITY) :
        std::ldexp(float(mantissa+(exponent ? 1024 : 0)),exponent ? int(exponent)-25 : -24);
    pixels[i] = word&0x8000 ? -value : value;
  }
  std::memcpy(pixels.data()+pixels_per_eye*8,reinterpret_cast<const uint8_t *>(mapped)+2*colour_bytes,pixels_per_eye*8);
  readback->unmap();
  // Independent fixed-source oracle: distinct firstInstance, transforms, camera,
  // per-eye reflection/snapshot/bottom and depth. Never reads a shader register.
  for (uint32_t eye = 0; eye < 2; ++eye) for (uint32_t y = 1; y < size-1; ++y) for (uint32_t x = 1; x < size-1; ++x) {
    const uint32_t index = eye*pixels_per_eye+y*size+x;
    const float world_x = 2*(x+.5f)/size-1-.125f*eye, world_y = 1-2*(y+.5f)/size;
    const uint32_t instance = world_x >= 0;
    const auto &m = materials[instance];
    for (uint32_t channel = 0; channel < 4; ++channel) Need(std::isfinite(pixels[index*4+channel]),"Water finite pixel");
    if (mode == 6 || mode == 7) continue; // separate changed-animation comparison below
    const float delta_x = float(eye)-world_x, delta_y = -world_y, delta_z = 2.5f;
    const float length = std::sqrt(delta_x*delta_x+delta_y*delta_y+delta_z*delta_z);
    const float facing = delta_z/length, fresnel = .2f+.8f*std::pow(1-facing,3.5f);
    std::array<float,4> expected{.9f*m.tint.x,.9f*m.tint.y,.9f*m.tint.z,m.tint.w};
    const std::array<float,3> reflection = mode == 2 ? std::array{.5f,.375f,.25f} : std::array{.25f+.25f*eye,.125f,.0625f};
    const float snapshot[]{.125f,.25f+.25f*eye,.5f};
    if (mode == 1 || mode == 2) for (uint32_t c = 0; c < 3; ++c) expected[c] += .9f*fresnel*reflection[c];
    if (mode == 3 || mode == 4) {
      for (uint32_t c = 0; c < 3; ++c) expected[c] = snapshot[c]+(expected[c]-snapshot[c])*m.tint.w;
      expected[3] = 1;
    }
    if (mode == 4) {
      // Eye1 bottom depth AND projected depth both differ by .125: opacity=.25.
      const float tint[]{m.tint.x,m.tint.y,m.tint.z}, shallow_cube = .75f*.75f*.75f;
      for (uint32_t c = 0; c < 3; ++c)
        expected[c] = .25f*expected[c]+.75f*(tint[c]+(1-tint[c])*shallow_cube)*snapshot[c];
    }
    if (mode == 5) {
      // Primary light is fully shadowed (.5 > .25); no secondary specular.
      const float ambient[]{.2f,.3f,.4f}, light[]{.4f,.5f,.6f}, fog[]{.1f,.2f,.3f};
      for (uint32_t c = 0; c < 3; ++c) expected[c] = expected[c]*(ambient[c]+.5f*light[c])+fog[c]*.125f;
    }
    if (lit) {
      const auto position = LitVec(world_x,world_y,.5f), camera = LitVec(float(eye),0,3);
      const auto view = LitNormalize(LitSubtract(camera,position));
      auto lights = pass.lights; lights[1].colour.x += .1f*instance;
      LitVector diffuse{}, specular{};
      for (const auto &light : lights) {
        const auto response = EvaluateLitLight(light,position,LitVec(0,0,1),view,m.shininess);
        diffuse = LitAdd(diffuse,LitScale(light.colour,response.diffuse));
        specular = LitAdd(specular,LitScale(light.colour,response.specular));
      }
      auto result = LitMultiply(LitVec(expected[0],expected[1],expected[2]),
          LitAdd(LitScale(diffuse,.5f),LitVec(.2f,.3f,.4f)));
      result = LitAdd(result,LitScale(LitMultiply(specular,LitVec(1.05f,.97f,1.27f)),2*m.highlight));
      for (auto fog : pass.fog) result = ApplyLitFog(result,position,camera,fog);
      expected = {result.x,result.y,result.z,expected[3]+LitDot(specular,LitVec(.223f,.693f,.091566f))};
    }
    if (blend) {
      const float background[]{.125f,.25f,.5f};
      for (uint32_t c = 0; c < 3; ++c) expected[c] = expected[c]*expected[3]+background[c]*(1-expected[3]);
      expected[3] = 1; // separate-alpha source-over, not colour factors applied to alpha
    }
    for (uint32_t c = 0; c < 4; ++c) {
      // HDR16 quantization of both sampled inputs and the output, all oracle
      // values below one; this bound also includes ordinary shader arithmetic.
      const bool matches = lit ? HalfStoreMatches(pixels[index*4+c],expected[c]) :
          std::abs(pixels[index*4+c]-expected[c]) <= 4e-4f;
      if (!matches) {
        std::cerr << "Water mode=" << mode << " eye=" << eye << " x=" << x << " y=" << y << " c=" << c
                  << " actual=" << pixels[index*4+c] << " expected=" << expected[c] << '\n';
        Need(false,"Water independent pixel oracle");
      }
    }
    Need(std::abs(pixels[2*pixels_per_eye*4+index]-(mode == 12 ? 1.f : eye ? .375f : .5f)) < 1e-6f,"Water per-eye depth oracle");
  }
  std::cout << "PASS water mode=" << mode << " phase=" << phase << "; two-eye instanced indirect draw, source/program leases and snapshot\n";
  return pixels;
}
} // namespace
void CheckNativeWater(RenderDevice &device) {
  CheckImport();
  for (uint32_t mode = 0; mode < 6; ++mode) Run(device,mode);
  for (uint32_t mode = 8; mode <= 13; ++mode) Run(device,mode);
  for (uint32_t mode : {6u,7u}) {
    const auto first = Run(device,mode,0), second = Run(device,mode,.37f);
    for (uint32_t eye = 0; eye < 2; ++eye) {
      uint32_t changed = 0;
      for (uint32_t i = eye*pixels_per_eye*4; i < (eye+1)*pixels_per_eye*4; ++i)
        changed += std::abs(first[i]-second[i]) > 1e-4f;
      Need(changed >= 8,"Water authored animation must change both eyes");
    }
  }
}
