/**
 * @brief Owned controller channels -> authored material UV motion and cue clocks.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"

namespace bd::gpu::scene {
struct NativeEffectClock {
  float time=0, speed=1;
  bool active=false, loop=false, queued=false;
  bool NeedsDuration() const { return active && (!queued || speed > 0); }
};
struct NativeEffectClockStep { float time=0; bool transition=false; };
inline std::optional<NativeEffectClockStep> AdvanceNativeEffectClock(
    const NativeEffectClock &clock, float delta, std::optional<double> duration) {
  if (!clock.active) return NativeEffectClockStep{clock.time}; // dormant payload is not validated
  if (!std::isfinite(clock.time) || !std::isfinite(clock.speed) || !std::isfinite(delta) ||
      (clock.NeedsDuration() && (!duration || !std::isfinite(*duration)))) return {};
  NativeEffectClockStep result{float(std::fma(double(clock.speed),double(delta),double(clock.time)))};
  if (!std::isfinite(result.time)) return {};
  if (clock.queued) {
    result.transition=(clock.speed > 0 && double(result.time) > *duration) ||
        (clock.speed < 0 && result.time < 0);
  } else if (double(result.time) > *duration) {
    // Authored clocks subtract once, even for a step spanning several cycles.
    result.time=float(clock.loop ? double(result.time)-*duration : *duration);
  } else if (result.time < 0) result.time=0;
  return std::isfinite(result.time) ? std::optional(result) : std::nullopt;
}

enum class NativeEffectUVMode : uint8_t { Scroll, Translation, Rotation };
struct NativeEffectUVMotion {
  NativeEffectUVMode mode=NativeEffectUVMode::Scroll;
  uint32_t joint=0; // dense model-local pose identity, never a source node/address
  std::array<float,2> offset{}, rate{}, divisor{1,1};
};
inline std::optional<std::array<float,2>> EvaluateNativeEffectUV(const NativeEffectUVMotion &motion,
    float delta, std::span<const NativeJointChannels> channels) {
  std::array<float,2> value;
  if (motion.mode == NativeEffectUVMode::Scroll) {
    for (size_t axis=0; axis<2; ++axis)
      value[axis]=float(std::fma(double(motion.rate[axis]),double(delta),double(motion.offset[axis])));
  } else {
    if (motion.joint >= channels.size()) return {};
    const auto &channel=channels[motion.joint];
    // Authored UV drivers read payloads even when hierarchy activation is off.
    // A channel driver REPLACES scrolling, including dormant nonfinite rates.
    if (motion.mode == NativeEffectUVMode::Translation) {
      value={channel.translation[0],channel.translation[1]};
    } else {
      const auto matrix=JointRotation(channel.rotation);
      for (float component : matrix) if (!std::isfinite(component)) return {};
      const float x=matrix[8], y=matrix[9], z=matrix[10];
      const float xx=float(double(x)*double(x));
      const float horizontal=float(std::sqrt(double(float(std::fma(double(z),double(z),double(xx))))));
      const float pitch=y == 0 && horizontal == 0 ? 0.0f : -float(std::atan2(double(y),double(horizontal)));
      const float yaw=x == 0 && z == 0 ? 0.0f : float(std::atan2(double(x),double(z)));
      value={yaw,pitch};
    }
    for (size_t axis=0; axis<2; ++axis) {
      if (!std::isfinite(motion.divisor[axis]) || motion.divisor[axis] == 0) return {};
      value[axis]=float(double(value[axis])/double(motion.divisor[axis]));
    }
  }
  for (float component : value) if (!std::isfinite(component)) return {};
  return value;
}
} // namespace bd::gpu::scene
