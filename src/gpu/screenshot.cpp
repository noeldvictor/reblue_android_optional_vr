/**
 * @file    gpu/screenshot.cpp
 * @brief   Frame-identified post-gamma readback, retired by the actual slot fence.
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license BSD 3-Clause License, see LICENSE.
 */
#include "gpu/screenshot_readback.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/settings.h"
#include <rex/cvar.h>
#include <array>
#include <atomic>
#include <fstream>
#include <mutex>
#include <span>
#define MINIZ_HEADER_FILE_ONLY
#include <miniz.h>
#if !defined(REBLUE_D3D12)
#include <plume_vulkan.h>
#endif

REXCVAR_DEFINE_BOOL(bd_native_frame_probe, false, kCvarGroup,
    "Accept one bounded logs/native_frame_probe.request and save a fence-identified post-gamma JPEG; desktop diagnostic only.");

namespace bd::gpu {
namespace {
std::atomic<bool> g_ready{false};
std::atomic<uint64_t> g_requested{0};
std::atomic<uint64_t> g_request_version{1};
std::mutex g_mutex;
Capture g_capture;
struct Pending {
  std::unique_ptr<ScreenshotReadback> readback;
  uint64_t version = 0;
  bool probe = false;
};
std::array<Pending,kNumFrames> g_pending;
uint64_t g_last_ui_request = 0;
bool g_probe_attempted = false;
void Publish(Capture capture, uint64_t version) {
  std::lock_guard lock(g_mutex);
  if (version != g_request_version.load(std::memory_order_acquire)) return;
  g_capture = std::move(capture); g_ready.store(true,std::memory_order_release);
}
bool WriteProbe(std::span<const uint8_t> bytes) {
#if defined(_WIN32)
  if (bytes.empty() || bytes.size() > 110u*1024) return false;
  HANDLE file = CreateFileW(L"logs/native_frame_probe.jpg",GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  DWORD written = 0;
  const bool good = WriteFile(file,bytes.data(),DWORD(bytes.size()),&written,nullptr) && written == bytes.size();
  CloseHandle(file);
  return good;
#else
  return false;
#endif
}
bool CopySupported(VideoState &s, plume::RenderTexture &image) {
#if !defined(REBLUE_D3D12)
  const auto &texture = static_cast<const plume::VulkanTexture &>(image);
  if (texture.ownership) return true;
  if (!s.swap_chain) return false;
  bool swap_image = false;
  for (uint32_t i=0;i<s.framebuffers.size();++i) swap_image |= s.swap_chain->getTexture(i) == &image;
  if (!swap_image) return false; // External XR images require their own usage contract.
  auto &swap = static_cast<plume::VulkanSwapChain &>(*s.swap_chain);
  VkSurfaceCapabilitiesKHR caps{};
  return vkGetPhysicalDeviceSurfaceCapabilitiesKHR(static_cast<plume::VulkanDevice &>(*s.device).physicalDevice,
      swap.surface,&caps) == VK_SUCCESS && (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
#else
  return true;
#endif
}
}
void RequestScreenshot() {
  std::lock_guard lock(g_mutex);
  const auto version = g_request_version.fetch_add(1,std::memory_order_acq_rel)+1;
  g_capture = {};
  g_ready.store(false,std::memory_order_release);
  g_requested.store(version,std::memory_order_release);
}
bool ReadyScreenshot() { return g_ready.load(std::memory_order_acquire); }
Capture TakeScreenshot() {
  std::lock_guard lock(g_mutex);
  Capture result = std::move(g_capture); g_capture = {};
  g_ready.store(false,std::memory_order_release);
  return result;
}
void CancelScreenshot() {
  std::lock_guard lock(g_mutex);
  g_request_version.fetch_add(1,std::memory_order_acq_rel);
  g_requested.store(0,std::memory_order_release);
  g_capture = {}; g_ready.store(false,std::memory_order_release);
  // Pending GPU buffers survive cancellation until their actual slot fence.
}
std::vector<uint8_t> EncodePng(const Capture &capture) {
  const auto plan = ScreenshotPlan::Make(capture.width,capture.height);
  if (!plan || capture.rgba.size() != uint64_t(capture.width)*capture.height*4) return {};
  size_t length = 0;
  void *png = tdefl_write_image_to_png_file_in_memory(capture.rgba.data(),int(capture.width),int(capture.height),4,
      int(capture.width*4),&length);
  if (!png) return {};
  std::vector<uint8_t> result(static_cast<const uint8_t *>(png),static_cast<const uint8_t *>(png)+length);
  mz_free(png);
  return result;
}
void CollectScreenshotAfterFence(VideoState &, uint32_t slot) {
  if (slot >= g_pending.size() || !g_pending[slot].readback) return;
  auto pending = std::move(g_pending[slot]); g_pending[slot] = {};
  if (!pending.probe && pending.version != g_request_version.load(std::memory_order_acquire)) return;
  auto capture = pending.readback->CollectAfterFence();
  if (!pending.probe) { Publish(capture ? std::move(*capture) : Capture{},pending.version); return; }
  if (!capture) { BD_ERROR("[native-frame-probe] GPU readback failed after slot fence"); return; }
  const auto encoded = EncodeJpeg(*capture,110u*1024);
  if (!WriteProbe(encoded)) { BD_ERROR("[native-frame-probe] encoding/size/exclusive-write refused"); return; }
  BD_INFO("[native-frame-probe] saved request {} frame {} slot {} input {:012X} output {:012X} descriptor {} size {}x{} bytes {}; post-gamma, GPU fence complete",
      capture->request,capture->frame,slot,capture->input,capture->output,capture->descriptor,
      capture->width,capture->height,encoded.size());
}
void ServiceOnPresent(VideoState &s, plume::RenderTexture *back, plume::RenderFramebuffer *framebuffer,
                      uint32_t width, uint32_t height, uint64_t input, uint32_t descriptor) {
  std::optional<ScreenshotProbeRequest> probe;
  const auto frame = FrameStatFrameCount();
  if (REXCVAR_GET(bd_native_frame_probe) && !g_probe_attempted && frame%5 == 0) {
    std::ifstream request("logs/native_frame_probe.request",std::ios::binary);
    if (request) {
      g_probe_attempted = true;
      std::array<char,65> text{}; request.read(text.data(),text.size());
      probe = ParseScreenshotProbe({text.data(),size_t(request.gcount())});
      if (!probe || !probe->Accept(frame) || width > 2048 || height > 1200) {
        BD_ERROR("[native-frame-probe] malformed/expired request or unsupported extent; frame {}",frame); return;
      }
    }
  }
  const auto version = g_requested.exchange(0,std::memory_order_acq_rel);
  const bool ui = version && version != g_last_ui_request && version == g_request_version.load(std::memory_order_acquire);
  if (!probe && !ui) return;
  if (ui) g_last_ui_request = version;
  const auto slot = Video::CurrentFrameSlot();
  const auto plan = ScreenshotPlan::Make(width,height);
  uint64_t used = 0;
  for (const auto &entry : g_pending) if (entry.readback) used += entry.readback->Bytes();
  if ((probe && ui) || !plan || !back || !framebuffer || !s.device || !s.command_list_open ||
      slot >= g_pending.size() || g_pending[slot].readback || used > ScreenshotPlan::kBudget ||
      plan->bytes > ScreenshotPlan::kBudget-used || !CopySupported(s,*back)) {
    if (probe) BD_ERROR("[native-frame-probe] pending work, unsupported copy or shared readback budget refused");
    if (ui) Publish({},version);
    return;
  }
  Capture metadata; metadata.request = probe ? probe->id : version; metadata.frame = frame;
  metadata.input = input; metadata.output = uint64_t(reinterpret_cast<uintptr_t>(back)); metadata.descriptor = descriptor;
  auto readback = ScreenshotReadback::Create(*s.device,*plan,std::move(metadata));
  if (!readback || !readback->Record(*s.command_list,*back,*framebuffer)) {
    if (probe) BD_ERROR("[native-frame-probe] readback allocation or native image contract refused");
    if (ui) Publish({},version);
    return;
  }
  if (probe) BD_INFO("[native-frame-probe] recorded request {} frame {} slot {} input {:012X} output {:012X} descriptor {} size {}x{}; awaiting submission fence",
      probe->id,frame,slot,input,readback->Metadata().output,descriptor,width,height);
  g_pending[slot] = {std::move(readback),version,probe.has_value()};
}
} // namespace bd::gpu
