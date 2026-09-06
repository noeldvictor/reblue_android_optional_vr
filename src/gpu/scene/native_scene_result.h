/**
 * @brief   Single-use, frame-bounded completed-scene ownership.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
#include <optional>
#include <utility>
namespace bd::gpu::scene {
template <class T> class SceneResultSlot {
public:
  void Clear() { result_.reset(); }
  void Complete(uint64_t frame, T result) {
    result_.reset();
    frame_ = frame;
    result_.emplace(std::move(result));
  }
  std::optional<T> Take(uint64_t frame) {
    if (frame != frame_) Clear();
    return std::exchange(result_, std::nullopt);
  }
private:
  uint64_t frame_ = 0;
  std::optional<T> result_;
};
} // namespace bd::gpu::scene
