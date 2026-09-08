/**
 * @brief Bounded late authored visual inputs identified by native instances.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace bd::gpu::scene {
struct NativeVisualIdentity {
  uint64_t instance = 0, model_generation = 0;
  explicit operator bool() const { return instance && model_generation; }
  auto operator<=>(const NativeVisualIdentity &) const = default;
};
enum class NativeVisualBlend : uint8_t {
  Alpha, AdditiveAlpha, ModulateSource, ScreenDestination, FadeDestination, ModulateInverseSource
};
struct NativeVisualInputs {
  NativeVisualIdentity identity;
  NativeVisualBlend blend = NativeVisualBlend::Alpha;
  uint32_t diffuse_class = 0;
};
// A batch publication, not another instance registry. Authored preparation must
// finish before Publish; dynamic effect outputs still resolve at their own late
// producers. Values/identities survive source-address reuse and batch retirement.
class NativeVisualPublication {
public:
  static constexpr size_t kMaxVisuals = 4096, kMaxBytes = 256u << 10;
  bool Publish(uint32_t frame, std::vector<NativeVisualInputs> inputs) {
    inputs_.clear(); valid_ = false;
    if (inputs.empty() || inputs.size() > kMaxVisuals ||
        inputs.capacity() > kMaxBytes / sizeof(NativeVisualInputs)) return false;
    std::sort(inputs.begin(), inputs.end(), [](const auto &a, const auto &b) { return a.identity < b.identity; });
    for (size_t i = 0; i < inputs.size(); ++i)
      if (!inputs[i].identity || uint32_t(inputs[i].blend) > 5 ||
          (i && inputs[i-1].identity == inputs[i].identity)) return false;
    inputs_ = std::move(inputs); frame_ = frame; valid_ = true;
    return true;
  }
  std::optional<NativeVisualInputs> Read(NativeVisualIdentity identity, uint32_t frame) const {
    if (!valid_ || !identity || frame != frame_) return {};
    const auto it = std::lower_bound(inputs_.begin(), inputs_.end(), identity,
        [](const auto &item, const auto &key) { return item.identity < key; });
    return it != inputs_.end() && it->identity == identity ? std::optional(*it) : std::nullopt;
  }
private:
  std::vector<NativeVisualInputs> inputs_;
  uint32_t frame_ = 0;
  bool valid_ = false;
};
} // namespace bd::gpu::scene
