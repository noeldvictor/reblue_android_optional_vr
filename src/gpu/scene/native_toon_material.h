/**
 * @brief Native Toon animation and edge-parameter production.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace bd::gpu::scene {
constexpr uint32_t ToonFrameIndex(uint32_t counter) {
  return uint32_t(std::bit_cast<int32_t>(counter) / 6);
}
constexpr uint32_t AdvanceToonCounter(uint32_t counter, int32_t limit) {
  const auto next = counter + 1;
  return std::bit_cast<int32_t>(next) >= limit ? 0 : next;
}
// Floating-point load/store round trips preserve finite words and quiet sNaNs.
constexpr uint32_t ToonFloatWord(uint32_t word) {
  return (word & 0x7f800000u) == 0x7f800000u && (word & 0x007fffffu)
      ? word | 0x00400000u : word;
}
inline std::array<uint32_t, 8> BuildToonEdgeParameters(
    const std::array<uint32_t, 6> &authored,
    const std::array<uint32_t, 2> &inherited) {
  std::array<uint32_t, 8> result;
  for (uint32_t i = 0; i < 6; ++i) result[i] = ToonFloatWord(authored[i]);
  for (uint32_t i = 0; i < 2; ++i) result[6 + i] = ToonFloatWord(inherited[i]);
  return result;
}
template <class Port>
auto SelectToonImage(Port &port, uint32_t index) {
  const auto list = port.TextureList();
  if (!list) return decltype(port.FallbackImage()){};
  if (list == port.ActiveList()) index += port.ActiveOffset();
  return index < port.ImageCount(list) ? port.Image(list, index) : port.FallbackImage();
}
template <class Port>
void UpdateToonMaterial(Port &port) {
  // Binding may change the second source; reload it after the first bind.
  port.BindTexture(6, SelectToonImage(port, ToonFrameIndex(port.Counter(0))));
  port.BindTexture(7, SelectToonImage(port, ToonFrameIndex(port.Counter(1)) + 3u));
  for (uint32_t i = 0; i < 3; ++i)
    port.SetCounter(i, AdvanceToonCounter(port.Counter(i), i == 2 ? 21 : 18));
}
} // namespace bd::gpu::scene
