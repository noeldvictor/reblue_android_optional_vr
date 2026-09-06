/**
 * @brief   Per-view completed images with native target pins and temporary UI references.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/post_chain.h"
#include "gpu/scene/native_scene_result.h"
#include <array>
namespace bd::gpu::scene {
struct CompletedSceneImages {
  HostPostInputs inputs;
  GuestTexture *output = nullptr; // remaining UI publication destination
  std::array<GuestTexture *, 2> source_pins{};
  std::array<uint32_t, 2> output_references{};
  CompletedSceneImages() = default;
  CompletedSceneImages(const CompletedSceneImages &) = delete;
  CompletedSceneImages &operator=(const CompletedSceneImages &) = delete;
  CompletedSceneImages(CompletedSceneImages &&other) noexcept;
  CompletedSceneImages &operator=(CompletedSceneImages &&other) noexcept;
  ~CompletedSceneImages();
  void Reset();
};

class NativeSceneResultScope {
public:
  explicit NativeSceneResultScope(uint32_t view);
  ~NativeSceneResultScope();
  NativeSceneResultScope(const NativeSceneResultScope &) = delete;
  NativeSceneResultScope &operator=(const NativeSceneResultScope &) = delete;
  void Clear() { result_.Clear(); }
  void Complete(uint32_t color_getter, uint32_t depth_getter,
                const HostPostInputs &inputs, GuestTexture *color, GuestTexture *depth,
                GuestTexture *output, GuestTexture *depth_output);
  std::optional<CompletedSceneImages> Take(uint32_t view);
private:
  NativeSceneResultScope *previous_ = nullptr;
  uint32_t view_ = 0, color_getter_ = 0, depth_getter_ = 0;
  uint64_t frame_ = 0;
  SceneResultSlot<CompletedSceneImages> result_;
};
std::optional<CompletedSceneImages> TakeCompletedSceneImages(uint32_t view);
} // namespace bd::gpu::scene
