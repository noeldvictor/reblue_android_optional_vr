/**
 * @brief Readiness-driven unattended pad policy; no runtime or guest-memory API.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace bd::xr {

// The selected debug panel survives while hidden. Only the separate visibility
// flag establishes whether the overlay owns input; unreadable is not hidden.
inline bool AutoplayOverlayBlocksInput(std::optional<uint32_t> hidden) {
  return !hidden || *hidden == 0;
}

struct AutoplayObservation {
  bool field_active = false, idle = false, player = false;
  bool event = false, movie = false, panel = false, directional_input = false;
  // Native stage category/number, not an allocation address. Zero means absent.
  uint64_t stage = 0;
  std::array<float, 3> position{};

  uint32_t Blockers() const {
    const bool finite = std::isfinite(position[0]) && std::isfinite(position[1]) &&
                        std::isfinite(position[2]);
    return (!field_active ? 1 : 0) | (!idle ? 2 : 0) | (!player ? 4 : 0) |
        (event ? 8 : 0) | (movie ? 16 : 0) | (panel ? 32 : 0) |
        (!directional_input ? 64 : 0) | (!stage ? 128 : 0) | (!finite ? 256 : 0);
  }
  bool Ready() const { return Blockers() == 0; }
};

struct AutoplayInput {
  bool start = false, confirm = false, walking = false;
  float x = 0, y = 0;
};

// One polling owner, externally serialized by the driver. No allocation/output.
// Call Reset when disabled. A gap, lost readiness or stage change invalidates
// the whole walking episode: loading/teleports cannot count as walked distance.
class Autoplay {
public:
  static constexpr double kSettleSeconds = 0.5;
  static constexpr double kMaxPollGap = 0.25;
  void Reset() { *this = Autoplay{}; }

  AutoplayInput Step(double seconds, const AutoplayObservation &observation) {
    AutoplayInput input;
    if (!std::isfinite(seconds) || seconds < 0) {
      Reset();
      return input;
    }
    const bool gap = last_poll_ < 0 || seconds < last_poll_ ||
                     seconds - last_poll_ > kMaxPollGap;
    last_poll_ = seconds;
    if (gap || !observation.Ready() || stage_ != observation.stage)
      EndWalk();
    if (seconds < 6.0)
      return input;
    if (seconds < 6.4) {
      input.start = true;
      return input;
    }
    // Preserve the title/dialogue recovery pulse, including after transitions.
    input.confirm = std::fmod(seconds - 6.4, 1.2) < 0.2;
    if (!observation.Ready())
      return input;
    if (ready_since_ < 0) {
      ready_since_ = seconds;
      stage_ = observation.stage;
    }
    if (seconds - ready_since_ < kSettleSeconds)
      return input;
    if (walk_since_ < 0) {
      walk_since_ = seconds;
      previous_position_ = observation.position;
      ++episode_;
    } else {
      // Count actual observed horizontal displacement, not calls or stick time.
      const double dx = double(observation.position[0]) - previous_position_[0];
      const double dz = double(observation.position[2]) - previous_position_[2];
      const double distance = std::hypot(dx, dz);
      previous_position_ = observation.position;
      // A discontinuity invalidates evidence and requires readiness again.
      // This conservative guard is not a game speed limit.
      if (distance > 10.0) {
        EndWalk();
        return input;
      }
      if (distance > 0.0001) {
        distance_ += distance;
        ++moved_samples_;
      }
    }
    walk_seconds_ = seconds - walk_since_;
    const double angle = walk_seconds_ * 0.35;
    input.x = static_cast<float>(std::sin(angle));
    input.y = static_cast<float>(std::cos(angle));
    input.walking = true;
    return input;
  }

  uint64_t Episode() const { return episode_; }
  uint64_t MovedSamples() const { return moved_samples_; }
  double Distance() const { return distance_; }
  double WalkSeconds() const { return walk_seconds_; }

private:
  void EndWalk() {
    ready_since_ = walk_since_ = -1;
    stage_ = moved_samples_ = 0;
    distance_ = walk_seconds_ = 0;
  }
  double last_poll_ = -1, ready_since_ = -1, walk_since_ = -1;
  double distance_ = 0, walk_seconds_ = 0;
  uint64_t stage_ = 0, episode_ = 0, moved_samples_ = 0;
  std::array<float, 3> previous_position_{};
};

} // namespace bd::xr
