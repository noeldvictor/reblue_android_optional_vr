/**
 * @brief Temporary precision getter publication for native FP16 scenes.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>

namespace bd::gpu::scene {
// Remaining engine cache/device readers expect scene precision enabled. These
// words do not choose native storage, pipeline formats or clear behaviour.
inline constexpr uint32_t kScenePrecisionCache = 0x82DBE1A8 + 308;
inline constexpr uint32_t kScenePrecisionDeviceSlot = 56 + 308;
inline constexpr uint32_t kScenePrecisionSetter = 0x82472540;
inline constexpr uint32_t kScenePrecisionRequest = 11756;

// The caller validates the device/cache before any scene publication. The old
// clear-time off/on bracket and its mutable surface-format/packet/dirty-bit
// writes are deliberately absent: native colour attachments stay FP16.
template <class WriteWord>
void PublishScenePrecisionGetters(uint32_t device, WriteWord write) {
  write(device + kScenePrecisionRequest, 1u);
  write(kScenePrecisionCache, 1u);
}
} // namespace bd::gpu::scene
