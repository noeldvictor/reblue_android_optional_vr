/**
 * @brief   Per-view completed images with native target pins and temporary UI references.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/post_chain.h"
#include "gpu/scene/native_scene_result.h"
#include "gpu/scene/native_scene_resolves.h"
#include "gpu/scene/native_scene_commands.h"
#include <array>
namespace bd::gpu::scene {
struct CompletedSceneImages {
  HostPostInputs inputs;
  NativeSceneResolveHandle resolves;
  GuestTexture *output = nullptr; // remaining UI publication destination
  bool pending_scene_color = false;
  std::array<GuestTexture *, 2> source_pins{};
  std::array<uint32_t, 2> output_references{};
  CompletedSceneImages() = default;
  CompletedSceneImages(const CompletedSceneImages &) = delete;
  CompletedSceneImages &operator=(const CompletedSceneImages &) = delete;
  CompletedSceneImages(CompletedSceneImages &&other) noexcept;
  CompletedSceneImages &operator=(CompletedSceneImages &&other) noexcept;
  ~CompletedSceneImages();
  // Only for a skipped/refused post sequence or an unconsumed normal scope.
  // A published native post result supersedes this initial colour publication.
  void PublishPendingColor();
  void Reset();
};

class NativeSceneResultScope {
public:
  explicit NativeSceneResultScope(uint32_t view);
  ~NativeSceneResultScope();
  NativeSceneResultScope(const NativeSceneResultScope &) = delete;
  NativeSceneResultScope &operator=(const NativeSceneResultScope &) = delete;
  void Clear(); // publishes an unconsumed result before its source can be reused
  bool Complete(uint32_t color_getter, uint32_t depth_getter,
                const HostPostInputs &inputs, GuestTexture *color, GuestTexture *depth,
                GuestTexture *output, GuestTexture *depth_output,
                NativeSceneResolveHandle resolves = {}, bool pending_scene_color = false);
  std::optional<CompletedSceneImages> Take(uint32_t view);
private:
  NativeSceneResultScope *previous_ = nullptr;
  uint32_t view_ = 0, color_getter_ = 0, depth_getter_ = 0;
  uint64_t frame_ = 0;
  SceneResultSlot<CompletedSceneImages> result_;
};
std::optional<CompletedSceneImages> TakeCompletedSceneImages(uint32_t view);
// Active native producer only, exact physical pair; never infer a resolve by
// dimensions, tiles or a currently bound sampled image. The scene scope owns it.
const NativeSceneResolves *ActiveNativeSceneResolves(plume::RenderTexture *color,
                                                     plume::RenderTexture *depth);
// Both MSAA and direct-source scenes select an already owned native framebuffer.
plume::RenderFramebuffer *ActiveNativeSceneFramebuffer(plume::RenderTexture *color,
                                                      plume::RenderTexture *depth);
NativeSceneCommands *ActiveNativeSceneCommands(plume::RenderTexture *color, plume::RenderTexture *depth);
void BindNativeSceneCommands(VideoState &s, NativeSceneCommands &commands);
void ApplyNativeSceneClear(VideoState &s, NativeSceneCommands &commands);
} // namespace bd::gpu::scene
