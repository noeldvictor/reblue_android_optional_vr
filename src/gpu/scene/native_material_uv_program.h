/**
 * @brief Immutable authored material animation descriptors, independent of source layout.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_uv.h"
#include "gpu/scene/native_effect_animation.h"

namespace bd::gpu::scene {
struct NativeMaterialUVProgram {
  struct Slot {
    uint32_t selector=0, channel=0, joint=0;
    NativeEffectUVMode mode=NativeEffectUVMode::Scroll;
    bool enabled=false;
    bool image_enabled=false;
    int32_t image_animation=0;
    std::array<float,2> rate{}, translation_divisor{1,1}, rotation_degrees{90,90};
    bool Same(const Slot &other) const {
      return selector == other.selector && channel == other.channel && joint == other.joint &&
          mode == other.mode && enabled == other.enabled && image_enabled == other.image_enabled &&
          image_animation == other.image_animation &&
          std::memcmp(rate.data(),other.rate.data(),sizeof(rate)) == 0 &&
          std::memcmp(translation_divisor.data(),other.translation_divisor.data(),sizeof(translation_divisor)) == 0 &&
          std::memcmp(rotation_degrees.data(),other.rotation_degrees.data(),sizeof(rotation_degrees)) == 0;
    }
    NativeEffectUVMotion Motion(bool has_channels, float radians_per_degree) const {
      NativeEffectUVMotion result;
      result.mode=has_channels ? mode : NativeEffectUVMode::Scroll;
      result.joint=joint; result.rate=rate;
      result.divisor=translation_divisor;
      if (result.mode == NativeEffectUVMode::Rotation)
        for (size_t axis=0; axis<2; ++axis)
          result.divisor[axis]=float(double(rotation_degrees[axis])*double(radians_per_degree));
      return result;
    }
  };
  std::vector<Slot> slots;
  NativeEyeControl eye; // origin/limits only; each evaluation supplies live gaze
  float radians_per_degree=0.017453292f;
  bool Valid() const {
    if (slots.empty() || slots.size() > NativeMaterialUVs::kMaxSlots) return false;
    for (const auto &slot : slots)
      if (uint32_t(slot.mode) > uint32_t(NativeEffectUVMode::Rotation) ||
          (slot.mode != NativeEffectUVMode::Scroll && slot.joint >= kMaxNativeJoints)) return false;
    // Dormant nonfinite rates/divisors/eye limits are checked only if evaluated.
    return true;
  }
  bool Same(const NativeMaterialUVProgram &other) const {
    if (slots.size() != other.slots.size() ||
        std::memcmp(&radians_per_degree,&other.radians_per_degree,sizeof(float)) ||
        std::memcmp(eye.origin.data(),other.eye.origin.data(),sizeof(eye.origin)) ||
        std::memcmp(eye.minimum.data(),other.eye.minimum.data(),sizeof(eye.minimum)) ||
        std::memcmp(eye.maximum.data(),other.eye.maximum.data(),sizeof(eye.maximum))) return false;
    for (size_t n=0; n<slots.size(); ++n) if (!slots[n].Same(other.slots[n])) return false;
    return true;
  }
};
} // namespace bd::gpu::scene
