/**
 * @brief Immutable, model-bound animation curves producing native joint channels.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include <numbers>
#include <optional>

namespace bd::gpu::scene {
struct NativeAnimationKey {
  float seconds = 0;
  JointVector value{};
};
struct NativeAnimationTrack {
  uint32_t pose_index = 0;
  std::vector<NativeAnimationKey> translation, rotation, scale;
  // Rotation keys are Euler turns in [-.5,.5]. Power-of-two turn fractions
  // preserve exact half-turn ties; rounded radians can reverse their arc.
  // Interpolate each authored axis, convert to radians, compose qZ * qY * qX.
};

class NativeAnimationClip {
public:
  NativeAnimationClip(NativeAnimationClip &&) = default;
  NativeAnimationClip &operator=(NativeAnimationClip &&) = default;
  NativeAnimationClip(const NativeAnimationClip &) = delete;
  NativeAnimationClip &operator=(const NativeAnimationClip &) = delete;

  // The eventual model/instance owner supplies its remaining residency budget.
  // No private unbounded cache or source-address identity lives in this object.
  static std::optional<NativeAnimationClip> Create(size_t joints, float duration,
      std::vector<NativeAnimationTrack> tracks, size_t maximum_bytes) {
    if (!joints || joints > kMaxNativeJoints || !std::isfinite(duration) || duration < 0 ||
        tracks.size() > joints || maximum_bytes < sizeof(NativeAnimationClip)) return {};
    size_t bytes = sizeof(NativeAnimationClip);
    auto account = [&](size_t count, size_t stride) {
      if (count > (maximum_bytes-bytes)/stride) return false;
      bytes += count*stride; return true;
    };
    if (!account(tracks.capacity(),sizeof(NativeAnimationTrack))) return {};
    std::array<bool,kMaxNativeJoints> seen{};
    for (const auto &track : tracks) {
      if (track.pose_index >= joints || seen[track.pose_index]) return {};
      seen[track.pose_index] = true;
      for (const auto *keys : {&track.translation,&track.rotation,&track.scale}) {
        if (!account(keys->capacity(),sizeof(NativeAnimationKey))) return {};
        float previous = -1;
        for (const auto &key : *keys) {
          if (!std::isfinite(key.seconds) || key.seconds < 0 || key.seconds <= previous) return {};
          for (float value : key.value) if (!std::isfinite(value)) return {};
          if (keys == &track.rotation)
            for (float value : key.value) if (std::abs(value) > .5f) return {};
          previous = key.seconds;
        }
        // A constant ignores its key time. Multi-key curves must cover the
        // beginning: never synthesize a predecessor before authored storage.
        if (keys->size() > 1 && keys->front().seconds != 0) return {};
      }
    }
    return NativeAnimationClip(joints,duration,std::move(tracks),bytes);
  }
  size_t JointCount() const { return joints_; }
  float Duration() const { return duration_; }
  size_t RetainedBytes() const { return bytes_; }
  std::span<const NativeAnimationTrack> Tracks() const { return tracks_; }

  bool Sample(float seconds, std::vector<NativeJointChannels> &out) const {
    if (!std::isfinite(seconds)) return false;
    seconds = std::clamp(seconds,0.0f,duration_);
    std::vector<NativeJointChannels> channels(joints_);
    for (const auto &track : tracks_) {
      auto &channel = channels[track.pose_index];
      channel.translated = !track.translation.empty();
      channel.rotated = !track.rotation.empty();
      channel.scaled = !track.scale.empty();
      if (channel.translated) channel.translation = SampleCurve(track.translation,seconds,false);
      if (channel.scaled) channel.scale = SampleCurve(track.scale,seconds,false);
      if (channel.rotated) {
        auto angles = SampleCurve(track.rotation,seconds,true);
        for (auto &angle : angles) angle *= 2*std::numbers::pi_v<float>;
        const float x=angles[0]*.5f, y=angles[1]*.5f, z=angles[2]*.5f;
        channel.rotation = MultiplyJointQuaternions(
            MultiplyJointQuaternions({0,0,std::sin(z),std::cos(z)}, {0,std::sin(y),0,std::cos(y)}),
            {std::sin(x),0,0,std::cos(x)});
      }
      for (float value : channel.translation) if (!std::isfinite(value)) return false;
      for (float value : channel.scale) if (!std::isfinite(value)) return false;
      for (float value : channel.rotation) if (!std::isfinite(value)) return false;
    }
    out = std::move(channels); return true;
  }

private:
  NativeAnimationClip(size_t joints, float duration, std::vector<NativeAnimationTrack> tracks, size_t bytes)
      : joints_(joints), duration_(duration), bytes_(bytes), tracks_(std::move(tracks)) {}
  static JointVector SampleCurve(std::span<const NativeAnimationKey> keys, float seconds, bool angular) {
    if (keys.size() == 1 || seconds <= keys.front().seconds) return keys.front().value;
    const auto next = std::lower_bound(keys.begin(),keys.end(),seconds,
        [](const auto &key, float time) { return key.seconds < time; });
    if (next == keys.end()) return keys.back().value;
    if (next->seconds == seconds) return next->value;
    const auto &previous = *(next-1);
    const float before = (next->seconds-seconds)/(next->seconds-previous.seconds), after = 1-before;
    JointVector result;
    for (size_t axis=0; axis<3; ++axis) {
      float a=previous.value[axis], b=next->value[axis];
      if (angular) {
        if (b-a < -.5f) b += 1;
        else if (b-a > .5f) a += 1;
        // Keep exact endpoint values (and quaternion signs) distinct from the
        // interior arc. Do not globally unwrap a clip or nlerp its Euler keys.
        result[axis] = float(std::fma(double(b),double(after),double(float(a*before))));
      } else {
        result[axis] = float(std::fma(double(a),double(before),double(float(b*after))));
      }
    }
    return result;
  }
  size_t joints_;
  float duration_;
  size_t bytes_;
  std::vector<NativeAnimationTrack> tracks_;
};
} // namespace bd::gpu::scene
