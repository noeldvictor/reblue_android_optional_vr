/**
 * @brief Immutable, model-bound animation curves producing native joint channels.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include <numbers>
#include <optional>
#include <memory>
#include <limits>

namespace bd::gpu::scene {
struct NativeAnimationKey {
  float seconds = 0;
  JointVector value{};
};
struct NativeAnimationSplineKey {
  float seconds = 0, value = 0, tangent = 0; // tangent per second; rotation in turns
  bool linear_to_next = false; // authored short angular segment, not global unwrap
};
struct NativeAnimationSplineChannel {
  std::array<std::vector<NativeAnimationSplineKey>,3> axes;
  float key_epsilon = 0;
  bool active = false;
};
struct NativeAnimationSplines {
  NativeAnimationSplineChannel translation, rotation, scale;
};
struct NativeAnimationTrack {
  uint32_t pose_index = 0;
  std::vector<NativeAnimationKey> translation, rotation, scale;
  std::unique_ptr<const NativeAnimationSplines> splines;
  bool Animated() const {
    return translation.size()>1 || rotation.size()>1 || scale.size()>1 ||
        (splines && (splines->translation.active || splines->rotation.active || splines->scale.active));
  }
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
      if (track.splines) {
        if (!account(1,sizeof(NativeAnimationSplines))) return {};
        const auto &splines = *track.splines;
        if ((splines.translation.active && !track.translation.empty()) ||
            (splines.rotation.active && !track.rotation.empty()) ||
            (splines.scale.active && !track.scale.empty())) return {};
        for (const auto *channel : {&splines.translation,&splines.rotation,&splines.scale}) {
          if (!std::isfinite(channel->key_epsilon) || channel->key_epsilon < 0) return {};
          for (const auto &axis : channel->axes) {
            if ((!channel->active && !axis.empty()) || !account(axis.capacity(),sizeof(NativeAnimationSplineKey))) return {};
            float previous = -std::numeric_limits<float>::infinity();
            for (const auto &key : axis) {
              if (!std::isfinite(key.seconds) || key.seconds <= previous || !std::isfinite(key.value) ||
                  !std::isfinite(key.tangent) || (channel == &splines.rotation && std::abs(key.value) > .5f)) return {};
              previous = key.seconds;
            }
          }
        }
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
      const auto *splines = track.splines.get();
      const bool cubic_translation = splines && splines->translation.active;
      const bool cubic_rotation = splines && splines->rotation.active;
      const bool cubic_scale = splines && splines->scale.active;
      channel.translated = cubic_translation || !track.translation.empty();
      channel.rotated = cubic_rotation || !track.rotation.empty();
      channel.scaled = cubic_scale || !track.scale.empty();
      if (channel.translated) channel.translation = cubic_translation ? SampleSpline(splines->translation,seconds) : SampleCurve(track.translation,seconds,false);
      if (channel.scaled) channel.scale = cubic_scale ? SampleSpline(splines->scale,seconds) : SampleCurve(track.scale,seconds,false);
      if (channel.rotated) {
        auto angles = cubic_rotation ? SampleSpline(splines->rotation,seconds) : SampleCurve(track.rotation,seconds,true);
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
  static JointVector SampleSpline(const NativeAnimationSplineChannel &channel, float seconds) {
    JointVector result{};
    for (size_t axis=0; axis<3; ++axis) {
      const auto &keys = channel.axes[axis];
      if (keys.empty()) continue; // a missing authored scalar curve evaluates to zero
      const auto next = std::upper_bound(keys.begin(),keys.end(),seconds,
          [](float time,const auto &key){return time < key.seconds;});
      if (next == keys.begin()) { result[axis]=keys.front().value; continue; }
      const auto &previous = *(next-1);
      const float elapsed = seconds-previous.seconds;
      if (next == keys.end() || elapsed < channel.key_epsilon || elapsed == 0) {
        result[axis]=previous.value; continue;
      }
      const float span = next->seconds-previous.seconds;
      float a=previous.value,b=next->value,ta=previous.tangent,tb=next->tangent;
      if (previous.linear_to_next) {
        if (b-a < -.5f) b += 1;
        if (b-a > .5f) a += 1;
        ta=tb=(b-a)/span;
      }
      // Cubic Hermite, keeping the source's fused accumulation order. Tangents
      // are native derivatives, not packed decoder state or per-frame samples.
      const float inverse=1/span, square=elapsed*elapsed;
      const float normalized_square=square*(inverse*inverse);
      const float square_over_span=square*inverse;
      const float cube_over_span2=normalized_square*elapsed;
      const float end_tangent=cube_over_span2-square_over_span;
      const float cube=cube_over_span2*inverse;
      const float triple=normalized_square*3, twice=cube*2;
      const float start_tangent=(end_tangent-square_over_span)+elapsed;
      const float start_value=((twice-triple)+1)*a;
      const float first=float(std::fma(double(start_tangent),double(ta),double(start_value)));
      const float second=float(std::fma(double(triple-twice),double(b),double(first)));
      result[axis]=float(std::fma(double(end_tangent),double(tb),double(second)));
    }
    return result;
  }
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
