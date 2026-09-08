/**
 * @file    gpu/screenshot.h
 * @brief   One-shot capture of the presented game frame (pre-overlay) to PNG.
 *
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license   BSD 3-Clause License
 *            See LICENSE file in the project root for full license text.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace plume {
struct RenderTexture;
struct RenderFramebuffer;
} // namespace plume

namespace bd::gpu {
struct VideoState;
} // namespace bd::gpu

namespace bd::gpu {

// Tightly packed RGBA8 image (alpha forced opaque). Empty == capture failed.
struct Capture {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> rgba; // width*height*4, row-major, no padding
  uint64_t request = 0, frame = 0, input = 0, output = 0;
  uint32_t descriptor = ~0u;
};

// UI thread: latch a one-shot capture of the next presented frame.
void RequestScreenshot();

// UI thread: true once a capture (or a failure result) is available.
bool ReadyScreenshot();

// UI thread: move out the latest capture and clear the ready state.
// Returns an empty Capture if the capture failed or none is ready.
Capture TakeScreenshot();

// UI thread: abandon any pending/ready capture (call when the dialog closes).
void CancelScreenshot();

// UI thread: encode an RGBA capture to PNG bytes via miniz. Empty on failure.
std::vector<uint8_t> EncodePng(const Capture &c);
// Desktop diagnostic encoder: bounded in-memory output, no resize or disk write.
// Returns empty if unsupported, malformed or unable to fit at the fixed qualities.
std::vector<uint8_t> EncodeJpeg(const Capture &c, size_t maximum_bytes);

// Render thread: called once per Present, after the gamma pass and before the
// ImGui overlay, with the swapchain back texture (COLOR_WRITE) and its bound
// framebuffer. No-op unless a request is latched. Completed copies are collected
// separately at the slot-fence boundary. Recording briefly unbinds for the copy
// and rebinds back_fb so the overlay draws normally.
void ServiceOnPresent(VideoState &s, plume::RenderTexture *back,
                      plume::RenderFramebuffer *back_fb, uint32_t width, uint32_t height,
                      uint64_t input, uint32_t descriptor);
// Renderer lock; called only at the real slot-fence retirement boundary.
void CollectScreenshotAfterFence(VideoState &s, uint32_t slot);

} // namespace bd::gpu
