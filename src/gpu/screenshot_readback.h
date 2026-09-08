/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/screenshot.h"
#include <plume_render_interface.h>
#include <memory>
#include <optional>
#include <charconv>
#include <string_view>

namespace bd::gpu {
struct ScreenshotPlan {
  static constexpr uint64_t kBudget = 64ull << 20;
  uint32_t width, height, pitch;
  uint64_t bytes;
  static std::optional<ScreenshotPlan> Make(uint32_t width, uint32_t height) {
    if (!width || !height || width > 8192 || height > 8192) return {};
    const uint32_t pitch = (width*4u+255u)&~255u;
    const uint64_t bytes = uint64_t(pitch)*height;
    if (bytes > kBudget) return {};
    return ScreenshotPlan{width,height,pitch,bytes};
  }
};
struct ScreenshotProbeRequest {
  uint64_t id = 0;
  uint32_t first = 0, last = 0;
  bool Valid() const { return id && first && first <= last && last-first <= 120; }
  bool Accept(uint32_t frame) const { return Valid() && first <= frame && frame <= last; }
};
inline std::optional<ScreenshotProbeRequest> ParseScreenshotProbe(std::string_view text) {
  if (text.empty() || text.size() > 64) return {};
  ScreenshotProbeRequest request;
  const auto number = [&](auto &value) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\n' || text.front() == '\r')) text.remove_prefix(1);
    const auto result = std::from_chars(text.data(),text.data()+text.size(),value);
    if (result.ec != std::errc{}) return false;
    text.remove_prefix(size_t(result.ptr-text.data()));
    return text.empty() || text.front() == ' ' || text.front() == '\n' || text.front() == '\r';
  };
  if (!number(request.id) || !number(request.first) || !number(request.last)) return {};
  for (const char c : text) if (c != ' ' && c != '\n' && c != '\r') return {};
  return request.Valid() ? std::optional(request) : std::nullopt;
}
// Owns the copy buffer through the originating submission. Record once; only
// the caller's actual completed fence authorizes CollectAfterFence. No counters
// or present-index arithmetic pretend to prove that fence.
class ScreenshotReadback {
public:
  static std::unique_ptr<ScreenshotReadback> Create(plume::RenderDevice &, ScreenshotPlan, Capture);
  bool Record(plume::RenderCommandList &, plume::RenderTexture &, plume::RenderFramebuffer &);
  std::optional<Capture> CollectAfterFence();
  uint64_t Bytes() const { return plan_.bytes; }
  const Capture &Metadata() const { return capture_; }
private:
  ScreenshotPlan plan_{};
  Capture capture_;
  std::unique_ptr<plume::RenderBuffer> buffer_;
  bool recorded_ = false, collected_ = false;
};
} // namespace bd::gpu
