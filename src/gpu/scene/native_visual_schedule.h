/**
 * @brief Bounded host ordering and dispatch for sorted models and primitives.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace bd::gpu::scene {
inline constexpr uint32_t kSortedModelLimit = 2048;
inline constexpr uint32_t kSortedPrimitiveLimit = 4096;
inline constexpr uint32_t kSortedDeferredLimit = 512;
struct VisualSortInput { float depth; uint32_t flags; };
struct VisualSortRange { float lower, upper, zero, one, scale; };

// The quantized authored layering is visible for overlapping translucent
// geometry. Preserve it and reverse insertion ties, not an arbitrary float sort.
inline std::optional<uint32_t> VisualSortLayer(
    VisualSortInput input, VisualSortRange range, bool model) {
  if (input.flags & 0x10000) return 1023;
  if (input.flags & 0x20000) return 0;
  double extent = float(double(range.upper) - double(range.lower));
  if (extent == range.zero) extent = range.one;
  double t = float(double(input.depth) - double(range.lower));
  t = float(t / extent);
  if (model) {
    if (t < range.zero) t = range.zero;
    else if (t > range.one) t = range.one;
  } else {
    if (!(t > range.zero)) t = range.zero;
    else if (!(t < range.one)) t = range.one;
  }
  const double scaled = float(double(float(double(range.one) - t)) * double(range.scale));
  const int32_t integer = std::isnan(scaled) || scaled < double(INT32_MIN) ? INT32_MIN :
      scaled >= double(INT32_MAX) ? INT32_MAX : int32_t(scaled);
  // Preserve the original wrapping byte-offset arithmetic, including model
  // NaNs mapping to layer 1. Refuse genuinely out-of-range layers safely.
  const uint32_t layer = (1u - uint32_t(integer)) & 0x3fffffffu;
  return layer < 1024 ? std::optional(layer) : std::nullopt;
}

class NativeVisualOrder {
  std::array<uint64_t, kSortedPrimitiveLimit> keys_;
  uint32_t count_ = 0;
public:
  template <class Read>
  bool Build(uint32_t count, VisualSortRange range, bool model, Read read) {
    count_ = 0;
    if (count > (model ? kSortedModelLimit : kSortedPrimitiveLimit)) return false;
    for (uint32_t index = 0; index < count; ++index) {
      const auto input = read(index);
      if (!input) return false;
      const auto layer = VisualSortLayer(*input, range, model);
      if (!layer) return false;
      keys_[index] = (uint64_t(*layer) << 32) | index;
    }
    std::sort(keys_.begin(), keys_.begin() + count, std::greater<uint64_t>());
    count_ = count;
    return true;
  }
  std::span<const uint64_t> Keys() const { return {keys_.data(), count_}; }
};

// Imports/callbacks are a port, not the owner of the ordering storage. Fields
// that callbacks can change are read when consumed. Primitives are sorted only
// after model callbacks and pass startup, and deferred count is reloaded after
// each immediate draw, just as it is by the deferred producer.
template <class Port>
void ExecuteVisualSchedule(Port &port, NativeVisualOrder &order,
                           uint32_t models, uint32_t initial_primitives) {
  if (!models && !initial_primitives) return;
  if (models) {
    port.ModelScope(true);
    for (auto key : order.Keys()) port.Model(uint32_t(key));
    port.ModelScope(false);
  }
  port.BeginPrimitives();
  if (port.PrimitiveCount()) {
    port.PreparePrimitives();
    port.SortPrimitives(order);
    port.PrepareSharedMaterial();
    uint32_t mode = 3, deferred = port.DeferredCount();
    for (auto key : order.Keys()) {
      const auto index = uint32_t(key);
      const auto flags = port.PrimitiveFlags(index);
      if (flags & 0xc0000) {
        if (deferred < kSortedDeferredLimit) port.Defer(index, deferred++);
        else port.DeferredOverflow();
        continue;
      }
      const uint32_t requested = flags & 0x10 ? 4 : 3;
      if (requested != mode) { port.SelectMode(requested); mode = requested; }
      port.Primitive(index);
      deferred = port.DeferredCount();
    }
  }
  port.ResetColour();
  port.EndPrimitives();
}
} // namespace bd::gpu::scene
