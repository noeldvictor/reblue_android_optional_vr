/**
 * @file    pass_dispatch_import.h
 * @brief   Temporary engine phase and participant-result decoding.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
namespace bd::gpu::scene {
inline bool ImportPassLightSpace(uint32_t phase) {
  return phase == 1 || phase == 4 || phase == 8;
}
inline bool ImportParticipantAccepted(uint32_t result) {
  return uint8_t(result) == 1;
}
inline bool ImportParticipantActive(uint8_t flag) { return flag == 1; }
} // namespace bd::gpu::scene
