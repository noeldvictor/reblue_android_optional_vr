/**
 * @brief Shared native animated-material UV values and authored gaze evaluation.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <cstring>
#include <vector>

namespace bd::gpu::scene {
struct NativeEyeControl {
  std::array<float,2> gaze{}, origin{}, minimum{}, maximum{};
};
inline std::optional<std::array<std::array<float,2>,2>> EvaluateNativeEyeUV(const NativeEyeControl &input) {
  for (const auto &pair : {input.gaze,input.origin,input.minimum,input.maximum})
    for (float value : pair) if (!std::isfinite(value)) return {};
  std::array<std::array<float,2>,2> result;
  for (size_t eye=0; eye<2; ++eye) for (size_t axis=0; axis<2; ++axis) {
    const float control=eye == 0 && axis == 0 ? -input.gaze[axis] : input.gaze[axis];
    // Authored controls extrapolate; do not clamp gaze or reorder the limits.
    // Preserve the rounded subtraction followed by double FMA and float store.
    const float extent=control < 0 ? float(double(input.origin[axis])-input.minimum[axis]) :
        float(double(input.maximum[axis])-input.origin[axis]);
    result[eye][axis]=float(std::fma(double(extent),double(control),double(input.origin[axis])));
    if (!std::isfinite(result[eye][axis])) return {};
  }
  return result;
}
enum class NativeMaterialUVOrigin : uint8_t { Effect, Eye };
struct NativeMaterialUVs {
  static constexpr uint32_t kMaxSlots=256;
  struct Entry {
    uint32_t slot=0, selector=0, channel=0;
    std::array<float,2> uv{};
    bool enabled=false;
    NativeMaterialUVOrigin origin=NativeMaterialUVOrigin::Eye;
    bool Same(const Entry &other) const {
      return slot == other.slot && selector == other.selector && channel == other.channel &&
          enabled == other.enabled && origin == other.origin &&
          std::memcmp(uv.data(),other.uv.data(),sizeof(uv)) == 0;
    }
  };
  std::vector<Entry> entries; // sparse, sorted model-local material slots
  uint32_t count=0;
  bool Valid() const {
    if (!count || count > kMaxSlots || entries.size() > count) return false;
    for (size_t i=0; i<entries.size(); ++i) {
      const auto &entry=entries[i];
      if (entry.slot >= count || (i && entries[i-1].slot >= entry.slot) ||
          uint32_t(entry.origin) > uint32_t(NativeMaterialUVOrigin::Eye)) return false;
      for (float value : entry.uv) if (!std::isfinite(value)) return false;
    }
    return true;
  }
  const Entry *Find(uint32_t slot) const {
    const auto it=std::lower_bound(entries.begin(),entries.end(),slot,
        [](const Entry &entry,uint32_t value) { return entry.slot < value; });
    return it != entries.end() && it->slot == slot ? &*it : nullptr;
  }
  bool Same(const NativeMaterialUVs &other) const {
    if (count != other.count || entries.size() != other.entries.size()) return false;
    for (size_t n=0; n<entries.size(); ++n) if (!entries[n].Same(other.entries[n])) return false;
    return true;
  }
  bool HasEyes() const {
    return std::any_of(entries.begin(),entries.end(),[](const Entry &entry) {
      return entry.enabled && entry.origin == NativeMaterialUVOrigin::Eye;
    });
  }
};
} // namespace bd::gpu::scene
