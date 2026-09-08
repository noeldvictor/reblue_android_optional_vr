/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/screenshot_readback.h"
#if !defined(REBLUE_D3D12)
#include <plume_vulkan.h>
#endif
namespace bd::gpu {
using namespace plume;
std::unique_ptr<ScreenshotReadback> ScreenshotReadback::Create(RenderDevice &device, ScreenshotPlan plan, Capture capture) {
  const auto checked = ScreenshotPlan::Make(plan.width,plan.height);
  if (!checked || checked->pitch != plan.pitch || checked->bytes != plan.bytes || !capture.rgba.empty()) return {};
  auto result = std::make_unique<ScreenshotReadback>();
  result->plan_ = plan; result->capture_ = std::move(capture);
  result->capture_.width = plan.width; result->capture_.height = plan.height;
  result->buffer_ = device.createBuffer(RenderBufferDesc::ReadbackBuffer(plan.bytes));
  return result->buffer_ ? std::move(result) : nullptr;
}
bool ScreenshotReadback::Record(RenderCommandList &cmd, RenderTexture &image, RenderFramebuffer &framebuffer) {
  if (recorded_ || !buffer_) return false;
#if !defined(REBLUE_D3D12)
  const auto &texture = static_cast<const VulkanTexture &>(image);
  if (texture.desc.width != plan_.width || texture.desc.height != plan_.height || texture.desc.arraySize != 1 ||
      texture.desc.multisampling.sampleCount != RenderSampleCount::COUNT_1 ||
      texture.desc.format != RenderFormat::B8G8R8A8_UNORM || texture.textureLayout != RenderTextureLayout::COLOR_WRITE) return false;
#endif
  cmd.setFramebuffer(nullptr);
  cmd.barriers(RenderBarrierStage::COPY,RenderTextureBarrier(&image,RenderTextureLayout::COPY_SOURCE));
  auto destination = RenderTextureCopyLocation::PlacedFootprint(buffer_.get(),RenderFormat::B8G8R8A8_UNORM,
      plan_.width,plan_.height,1,plan_.pitch/4,0);
  destination.texture = &image; // Existing Plume placed-footprint sampling contract.
  cmd.copyTextureRegion(destination,RenderTextureCopyLocation::Subresource(&image,0,0));
#if !defined(REBLUE_D3D12)
  VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(static_cast<VulkanCommandList &>(cmd).vk,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,
      0,1,&host,0,nullptr,0,nullptr);
#endif
  cmd.barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(&image,RenderTextureLayout::COLOR_WRITE));
  cmd.setFramebuffer(&framebuffer);
  recorded_ = true;
  return true;
}
std::optional<Capture> ScreenshotReadback::CollectAfterFence() {
  if (!recorded_ || collected_) return {};
#if !defined(REBLUE_D3D12)
  auto &buffer = static_cast<VulkanBuffer &>(*buffer_);
  if (vmaInvalidateAllocation(buffer.device->allocator,buffer.allocation,0,VK_WHOLE_SIZE) != VK_SUCCESS) return {};
#endif
  const auto *mapped = static_cast<const uint8_t *>(buffer_->map());
  if (!mapped) return {};
  capture_.rgba.resize(uint64_t(plan_.width)*plan_.height*4);
  for (uint32_t y=0;y<plan_.height;++y) for (uint32_t x=0;x<plan_.width;++x) {
    const auto *source = mapped+uint64_t(y)*plan_.pitch+x*4;
    auto *output = capture_.rgba.data()+(uint64_t(y)*plan_.width+x)*4;
    output[0] = source[2]; output[1] = source[1]; output[2] = source[0]; output[3] = 255;
  }
  buffer_->unmap(); collected_ = true;
  return std::move(capture_);
}
} // namespace bd::gpu
