/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/screenshot_readback.h"
#include <plume_vulkan.h>
#include <array>
#include <iostream>
#include <stdexcept>
using namespace bd::gpu;
using namespace plume;
namespace {
void Need(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
}
void CheckNativeScreenshot(RenderDevice &device) {
  uint32_t cases = 0;
  for (auto extent : {std::array{1u,1u},std::array{65u,3u},std::array{257u,5u},std::array{2048u,1u}}) {
    const auto [width,height] = extent;
    auto plan = ScreenshotPlan::Make(width,height);
    Need(bool(plan),"Screenshot layout refused");
    auto queue = device.createCommandQueue(RenderCommandListType::DIRECT);
    for (uint32_t variant=0;variant<2;++variant) {
      auto cmd = queue->createCommandList(); auto fence = device.createCommandFence();
      auto image = device.createTexture(RenderTextureDesc::ColorTarget(width,height,RenderFormat::B8G8R8A8_UNORM));
      const RenderTexture *color = image.get();
      RenderFramebufferDesc desc; desc.colorAttachments = &color; desc.colorAttachmentsCount = 1;
      auto framebuffer = device.createFramebuffer(desc);
      Capture metadata; metadata.request=1+cases; metadata.frame=100+cases; metadata.input=7; metadata.output=9; metadata.descriptor=11;
      auto copy = ScreenshotReadback::Create(device,*plan,metadata);
      Need(copy && !copy->CollectAfterFence(),"Unrecorded copy became evidence");
      cmd->begin(); cmd->barriers(RenderBarrierStage::GRAPHICS,RenderTextureBarrier(image.get(),RenderTextureLayout::COLOR_WRITE));
      cmd->setFramebuffer(framebuffer.get());
      cmd->clearColor(0,RenderColor(1,0,0,1));
      const RenderRect right(width/2,0,width,height);
      cmd->clearColor(0,RenderColor(0,0,1,1),&right,1);
      Need(copy->Record(*cmd,*image,*framebuffer) && !copy->Record(*cmd,*image,*framebuffer),"Readback not exactly once");
      // A subsequent write to the same image must not rewrite the copied frame.
      cmd->clearColor(0,RenderColor(0,1,0,1));
      cmd->end();
      const RenderCommandList *lists[] = {cmd.get()};
      queue->executeCommandLists(lists,1,nullptr,0,nullptr,0,fence.get());
      const auto &native = static_cast<VulkanCommandFence &>(*fence);
      Need(vkWaitForFences(static_cast<VulkanDevice &>(device).vk,1,&native.vk,VK_TRUE,5'000'000'000ull) == VK_SUCCESS,"Screenshot fence timeout");
      if (variant) { framebuffer.reset(); image.reset(); } // CPU collection owns only the completed copy.
      auto capture = copy->CollectAfterFence();
      Need(capture && !copy->CollectAfterFence(),"Readback collection not exactly once");
      Need(capture->frame == metadata.frame && capture->request == metadata.request && capture->input == 7 &&
          capture->output == 9 && capture->descriptor == 11,"Screenshot lost originating frame");
      for (uint32_t y=0;y<height;++y) for (uint32_t x=0;x<width;++x) {
        const auto *pixel = capture->rgba.data()+(uint64_t(y)*width+x)*4;
        Need(pixel[0] == (x < width/2 ? 255 : 0) && pixel[1] == 0 && pixel[2] == (x < width/2 ? 0 : 255) && pixel[3] == 255,
            "Screenshot channel/row-pitch/frame mismatch");
      }
      std::cout << "PASS native screenshot " << width << 'x' << height << " source-released=" << variant << " frame=" << capture->frame << '\n';
      ++cases;
    }
  }
  std::cout << "PASS screenshot cases=" << cases << "; disk images=0\n";
}
