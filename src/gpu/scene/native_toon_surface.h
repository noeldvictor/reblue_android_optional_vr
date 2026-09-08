/**
 * @brief Explicit owned Toon material inputs; unknown production is not default lighting.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lighting.h"

namespace bd::gpu::scene {
// The game's Toon family is distinct from the optional cel-shading asset slot.
// No register number, source address or shader variant key belongs in this API.
enum class NativeSceneSurface : uint8_t { Ordinary, Toon };
struct NativeToonSurface {
  LightingVector diffuse_scale{}, diffuse_add{}, ambient_scale{}, ambient_add{};
  std::array<LightingVector, 3> texture_colours{};
  bool ignore_texture_alpha = false;
};
} // namespace bd::gpu::scene
