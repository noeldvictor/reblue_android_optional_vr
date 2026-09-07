/**
 * @brief Owned fog publication lookup and diagnostic consumers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_fog.h"
#include <cstdint>
namespace bd::gpu::scene {
std::optional<NativeFogLayers> FindNativeFogLayers();
uint64_t NativeFogRevision();
bool NativeFogIsCurrent(uint64_t revision);
void NativeFogReport();
void CheckNativeFogLayers(const NativeFogLayers &fog, const uint8_t *pixel_constants, uint32_t pixel_bools);
} // namespace bd::gpu::scene
