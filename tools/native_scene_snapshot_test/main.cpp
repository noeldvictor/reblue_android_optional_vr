/**
 * @brief Tiny real-Vulkan tests of the actual host scene snapshot command core.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_scene_snapshot.h"
#include <plume_vulkan.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace plume;
using namespace bd::gpu;
using namespace bd::gpu::scene;
namespace {
constexpr uint32_t kSize = 8, kLayerBytes = kSize * kSize * 8;
std::atomic_uint errors = 0, warnings = 0, loader_messages = 0;
void Require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
VKAPI_ATTR VkBool32 VKAPI_CALL Validation(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *data, void *) {
  if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT && data->pMessageIdName &&
      std::strcmp(data->pMessageIdName, "Loader Message") == 0) {
    if (++loader_messages <= 10) std::cerr << "Loader diagnostic: " << data->pMessage << '\n';
  } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    if (++errors <= 20) std::cerr << "Vulkan error: " << data->pMessage << '\n';
  } else if (++warnings <= 10) std::cerr << "Vulkan warning: " << data->pMessage << '\n';
  return VK_FALSE;
}
struct Messenger {
  VkInstance instance;
  VkDebugUtilsMessengerEXT vk{};
  ~Messenger() { if (vk) vkDestroyDebugUtilsMessengerEXT(instance, vk, nullptr); }
};
std::shared_ptr<NativeTargetImage> Image(RenderDevice &device, uint32_t layers, uint32_t samples, bool depth) {
  auto result = std::make_shared<NativeTargetImage>();
  result->shape = {kSize, kSize, layers,
      depth ? RenderFormat::D32_FLOAT_S8_UINT : RenderFormat::R16G16B16A16_FLOAT, samples};
  auto desc = depth ? RenderTextureDesc::DepthTarget(kSize, kSize, result->shape.format, RenderMultisampling(samples))
                    : RenderTextureDesc::ColorTarget(kSize, kSize, result->shape.format, RenderMultisampling(samples));
  desc.arraySize = layers;
  result->image = device.createTexture(desc);
  Require(result->image && static_cast<VulkanTexture *>(result->image.get())->vk, "Image creation failed");
  // No shader/descriptor lookup in this copy-only fixture. This opaque index
  // satisfies the production sampled-image contract; real images/layouts are used.
  result->descriptor = 0;
  return result;
}
void CheckPixels(RenderDevice &device, uint32_t layers, uint32_t samples) {
  auto queue = device.createCommandQueue(RenderCommandListType::DIRECT);
  auto commands = queue->createCommandList();
  auto fence = device.createCommandFence();
  const auto color = Image(device, layers, samples, false), depth = Image(device, layers, samples, true);
  const auto resolved_color = samples > 1 ? Image(device, layers, 1, false) : nullptr;
  const auto resolved_depth = samples > 1 ? Image(device, layers, 1, true) : nullptr;
  const auto first = Image(device, layers, 1, false), second = Image(device, layers, 1, false);
  const RenderTexture *source = color->image.get();
  const RenderTexture *output = resolved_color ? resolved_color->image.get() : nullptr;
  RenderFramebufferDesc desc;
  desc.colorAttachments = &source;
  desc.colorAttachmentsCount = 1;
  desc.depthAttachment = depth->image.get();
  desc.viewMask = layers == 2 ? 3 : 0;
  if (samples > 1) {
    desc.colorResolveAttachments = &output;
    desc.depthResolveAttachment = resolved_depth->image.get();
    desc.depthResolveMode = RenderResolveMode::MIN;
  }
  auto framebuffer = device.createFramebuffer(desc);
  Require(bool(framebuffer), "Native scene framebuffer failed");
  const std::array<SampledImage, 2> resolved = samples > 1
      ? std::array{resolved_color->Sampled(), resolved_depth->Sampled()} : std::array<SampledImage, 2>{};
  auto scene = NativeSceneCommands::Create({color, depth}, framebuffer.get(), resolved, {{0, 0, 0, 1}, 1, 0});
  Require(bool(scene), "Native scene command recipe refused");
  // Distinct per-eye source contents, using explicit layer views rather than
  // shaders. Views/framebuffers stay alive until the submission fence completes.
  std::vector<std::unique_ptr<RenderTextureView>> eye_views;
  std::vector<std::unique_ptr<RenderFramebuffer>> eye_framebuffers;
  for (uint32_t eye = 0; eye < layers; ++eye) {
    auto view_desc = RenderTextureViewDesc::Texture2D(color->shape.format);
    view_desc.mipLevels = view_desc.arraySize = 1;
    view_desc.arrayIndex = eye;
    eye_views.push_back(color->image->createTextureView(view_desc));
    const RenderTextureView *view = eye_views.back().get();
    RenderFramebufferDesc eye_desc;
    eye_desc.colorAttachments = &source;
    eye_desc.colorAttachmentViews = &view;
    eye_desc.colorAttachmentsCount = 1;
    eye_framebuffers.push_back(device.createFramebuffer(eye_desc));
    Require(view && eye_framebuffers.back(), "Per-eye framebuffer failed");
  }
  commands->begin();
  scene->Bind(*commands);
  Require(scene->ApplyClear(*commands), "First clear missing");
  for (uint32_t eye = 0; eye < layers; ++eye) {
    commands->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(color->image.get(), RenderTextureLayout::COLOR_WRITE));
    commands->setFramebuffer(eye_framebuffers[eye].get());
    // Explicit rectangle records this layer write immediately. Deferred clears
    // across different custom views are a separate backend coverage question.
    const RenderRect rect(0, 0, kSize, kSize);
    commands->clearColor(0, RenderColor(2.f + eye, 0.25f + 0.5f * eye, 0.5f, 1.f), &rect, 1);
  }
  scene->Bind(*commands);
  // Start the full native scope without changing the per-eye colours, so its
  // ordinary MSAA attachment resolves complete at the snapshot's pass boundary.
  commands->clearDepthStencil(true, false, 0.375f, 0);
  Require(CopySceneSnapshot(*commands, *scene, first->Sampled()), "First snapshot refused");
  scene->Bind(*commands);
  Require(!scene->ApplyClear(*commands), "Resumed scene cleared again");
  commands->clearColor(0, RenderColor(4, 0.5f, 0.25f, 1));
  Require(CopySceneSnapshot(*commands, *scene, second->Sampled()), "Second snapshot refused");
  scene->Bind(*commands);
  commands->clearColor(0, RenderColor(8, 0.75f, 0.5f, 1));
  commands->setFramebuffer(nullptr);

  const std::array<SampledImage, 3> reads{first->Sampled(), second->Sampled(), scene->ColorReadImage()};
  auto readback = device.createBuffer(RenderBufferDesc::ReadbackBuffer(kLayerBytes * layers * reads.size()));
  Require(bool(readback), "Readback allocation failed");
  for (uint32_t target = 0; target < reads.size(); ++target) {
    const auto &image = reads[target];
    commands->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(image.texture, RenderTextureLayout::COPY_SOURCE));
    *image.layout = RenderTextureLayout::COPY_SOURCE;
    for (uint32_t eye = 0; eye < layers; ++eye)
      commands->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(readback.get(), image.format,
          kSize, kSize, 1, kSize, (target * layers + eye) * kLayerBytes),
          RenderTextureCopyLocation::Subresource(image.texture, 0, eye));
  }
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(static_cast<VulkanCommandList *>(commands.get())->vk,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host, 0, nullptr, 0, nullptr);
  commands->end();
  Require(errors == 0, "Validation failed before GPU submission");
  queue->executeCommandLists(commands.get(), fence.get());
  auto *native_fence = static_cast<VulkanCommandFence *>(fence.get());
  Require(vkWaitForFences(native_fence->device->vk, 1, &native_fence->vk, VK_TRUE, 5'000'000'000ULL) == VK_SUCCESS,
      "GPU fence failed/timed out");
  queue->waitForCommandFence(fence.get());
  auto *native_buffer = static_cast<VulkanBuffer *>(readback.get());
  Require(vmaInvalidateAllocation(native_buffer->device->allocator, native_buffer->allocation, 0, VK_WHOLE_SIZE) == VK_SUCCESS,
      "Readback invalidation failed");
  const auto *bytes = static_cast<const uint16_t *>(readback->map());
  Require(bytes != nullptr, "Readback mapping failed");
  for (uint32_t target = 0; target < reads.size(); ++target) for (uint32_t eye = 0; eye < layers; ++eye) {
    // Exactly representable binary16 values. HDR >1 and distinct eyes ensure
    // UNORM clamping, left-eye duplication and overwrite of old snapshots fail.
    const std::array<uint16_t, 4> expected = target == 0
        ? std::array<uint16_t, 4>{uint16_t(eye ? 0x4200 : 0x4000), uint16_t(eye ? 0x3a00 : 0x3400), 0x3800, 0x3c00}
        : target == 1 ? std::array<uint16_t, 4>{0x4400, 0x3800, 0x3400, 0x3c00}
                      : std::array<uint16_t, 4>{0x4800, 0x3a00, 0x3800, 0x3c00};
    for (uint32_t pixel = 0; pixel < kSize * kSize; ++pixel) for (uint32_t channel = 0; channel < 4; ++channel) {
      const auto index = ((target * layers + eye) * kSize * kSize + pixel) * 4 + channel;
      if (bytes[index] != expected[channel]) {
        std::cerr << "target=" << target << " eye=" << eye << " pixel=" << pixel << " channel=" << channel
                  << " actual half=" << bytes[index] << " expected half=" << expected[channel] << '\n';
        throw std::runtime_error("Snapshot pixel mismatch");
      }
    }
  }
  readback->unmap();
  std::cout << "PASS native snapshot: layers=" << layers << " samples=" << samples
            << " distinct eyes/HDR, two retained snapshots and resumed live writes\n";
}
} // namespace
int main() {
  try {
    VulkanInterfaceOptions options;
    options.extraInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    VkValidationFeatureEnableEXT sync = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    features.enabledValidationFeatureCount = 1;
    features.pEnabledValidationFeatures = &sync;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback = Validation;
    options.createInstance = [&](const VkInstanceCreateInfo *original, VkInstance *instance) {
      uint32_t count = 0;
      Require(vkEnumerateInstanceLayerProperties(&count, nullptr) == VK_SUCCESS, "Layer enumeration failed");
      std::vector<VkLayerProperties> layers(count);
      Require(vkEnumerateInstanceLayerProperties(&count, layers.data()) == VK_SUCCESS, "Layer enumeration failed");
      bool found = false;
      for (const auto &layer : layers) found |= std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0;
      Require(found, "Existing Khronos validation layer required; no unvalidated pass mode");
      const char *layer = "VK_LAYER_KHRONOS_validation";
      auto info = *original;
      info.enabledLayerCount = 1;
      info.ppEnabledLayerNames = &layer;
      features.pNext = info.pNext;
      debug.pNext = &features;
      info.pNext = &debug;
      return vkCreateInstance(&info, nullptr, instance);
    };
    {
      auto render_interface = CreateVulkanInterface(&options);
      Require(bool(render_interface), "Vulkan interface failed");
      const auto instance = static_cast<VulkanInterface *>(render_interface.get())->instance;
      Messenger messenger{instance};
      debug.pNext = nullptr;
      Require(vkCreateDebugUtilsMessengerEXT(instance, &debug, nullptr, &messenger.vk) == VK_SUCCESS,
          "Validation messenger failed");
      auto device = render_interface->createDevice();
      Require(bool(device), "Vulkan device failed");
      auto *native = static_cast<VulkanDevice *>(device.get());
      const auto &caps = device->getCapabilities();
      Require(caps.attachmentResolve && caps.multiview &&
          (caps.depthAttachmentResolveModes & (1u << uint32_t(RenderResolveMode::MIN))), "Required native features missing");
      std::cout << "GPU=" << native->physicalDeviceProperties.deviceName << "; images=8x8; raw bytes=0\n";
      for (uint32_t samples : {1u, 2u, 4u, 8u}) {
        const auto &limits = native->physicalDeviceProperties.limits;
        if (!(limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts &
              limits.framebufferStencilSampleCounts & samples)) {
          std::cout << "UNTESTED unsupported sample count " << samples << '\n';
          continue;
        }
        for (uint32_t layers : {1u, 2u}) CheckPixels(*device, layers, samples);
      }
    }
    std::cout << "Validation errors=" << errors << " warnings=" << warnings << " loaderMessages=" << loader_messages
              << "; retained raw bytes=0\n";
    Require(errors == 0, "Vulkan validation errors");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << "; validation errors=" << errors << " warnings=" << warnings << '\n';
    return 1;
  }
}
