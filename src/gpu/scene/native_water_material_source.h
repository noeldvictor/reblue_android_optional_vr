/**
 * @brief Copy completed water publications into semantic native values at the import boundary.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_water_inputs.h"
#include "gpu/scene/water_material_import.h"

namespace bd::gpu::scene {
// Call only after the resource writer's factor/clamp/parameter publications.
// Read the final destinations, not the original authored fields: writes can
// alias descriptors, other parameters or unrelated visual inputs. No writes,
// source pointers or register arrays are retained. The scene handoff must still
// supply generation/frame ownership and repeat this after a later writer.
template <class Read>
std::optional<NativeWaterMaterial> ReadNativeWaterMaterial(uint32_t material, Read read) {
  if (!material || uint64_t(material) + 5200 > uint64_t(UINT32_MAX) + 1) return {};
  WaterUpdateBuilder<Read> boundary{read};
  const auto scalar = [&](uint32_t offset) {
    return boundary.Float(boundary.Destination(uint64_t(material) + offset, false));
  };
  NativeWaterMaterial result;
  result.scroll_u = scalar(4768); result.scroll_v = scalar(4788);
  result.uv_scale = scalar(4808); result.phase = scalar(4828);
  const auto tint = boundary.Destination(uint64_t(material) + 4848, true);
  result.tint = {boundary.Float(tint), boundary.Float(tint+4), boundary.Float(tint+8), boundary.Float(tint+12)};
  result.wave_amplitude = scalar(4868); result.wave_speed = scalar(4888);
  result.wave_direction = scalar(4908); result.wave_frequency = scalar(4928);
  result.reflection_distortion = scalar(4960); result.distance_fade = scalar(4980);
  const float refraction = scalar(5000);
  result.refraction_distortion = scalar(5020);
  const float reflection = scalar(5040);
  result.normal_blend = scalar(5060); result.shininess = scalar(5080);
  result.highlight = scalar(5100);
  const float shore = scalar(5120);
  result.depth_opacity = scalar(5140); result.shore_brightness = scalar(5160); result.shore_gain = scalar(5180);
  if (!boundary.valid || !std::isfinite(refraction) || !std::isfinite(reflection) || !std::isfinite(shore)) return {};
  result.refraction = refraction >= .5f;
  result.reflection = reflection < .5f ? WaterReflectionNone :
      reflection < 1.5f ? WaterReflectionPlanar : WaterReflectionEnvironment;
  result.shore = shore > .5f;
  return result;
}
} // namespace bd::gpu::scene
