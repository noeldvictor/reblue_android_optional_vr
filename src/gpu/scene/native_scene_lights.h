/**
 * @brief Frame-owned scene lights and instance/node selection inputs.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_light_selection.h"
#include <algorithm>
#include <span>
#include <tuple>
#include <vector>

namespace bd::gpu::scene {
struct NativeSceneLight {
  NativeLightCandidate candidate;
  std::optional<LitLight> value;
};
struct NativeSceneLightSet {
  static constexpr size_t kMaxLights = 300;
  std::array<NativeSceneLight, kMaxLights> lights{};
  size_t count = 0;
  NativeLightScoreParameters scoring;
  uint32_t mode = 0;
  int32_t priority_light = -1;
  bool special_scene = false;
};
struct NativeObjectLightInputs {
  LitVector centre{};
  float radius = 0;
  uint32_t object_class = 0;
};
struct NativeNodeLightBinding {
  uint64_t instance = 0, model_generation = 0;
  uint32_t node = 0;
  // Empty is an authored Keep, not a missing/invalid import (which has no key).
  std::optional<NativeObjectLightInputs> inputs;
  auto Key() const { return std::tuple(instance, model_generation, node); }
};
struct NativeSceneLightSelection {
  NativeLightSelection selection;
  NativeSelectedLights lights{};
};
struct NativeSceneLightTicket {
  NativeSelectedLights lights{};
  uint64_t update = 0, revision = 0;
  bool inherited = false;
};
// Capture the authored action before sorting; resolve Keep only at the draw's
// final position. No object/source keys or speculative order revision survive.
struct NativeSceneLightRecipe {
  std::optional<NativeSelectedLights> bind;
  uint64_t update = 0;
};

inline std::optional<NativeSceneLightSelection> SelectNativeSceneLights(
    const NativeSceneLightSet &scene, const NativeObjectLightInputs &object, uint32_t light_view) {
  if (scene.count > scene.kMaxLights || scene.mode > 2 || light_view >= 16 ||
      object.object_class >= 32 || (scene.special_scene && object.object_class > 19)) return {};
  NativeLightSelectionInputs input;
  input.centre = object.centre; input.radius = object.radius; input.object_class = object.object_class;
  input.view = light_view; input.mode = scene.mode; input.priority_light = scene.priority_light;
  input.special_scene = scene.special_scene;
  NativeSceneLightSelection result;
  for (size_t n = 0; n < scene.count; ++n) {
    // Native shaders consume semantic lights, not the legacy category byte.
    // Never classify from the mutable live owner during native selection.
    if (!InsertNativeLight(result.selection, scene.lights[n].candidate, input, scene.scoring,
        [](const NativeLightSelection &) -> std::optional<uint8_t> { return 0; })) return {};
  }
  for (size_t slot = 0; slot < result.lights.size(); ++slot) {
    const auto id = result.selection.slots[slot].id;
    if (id < 0) continue;
    if (size_t(id) >= scene.count || !scene.lights[id].value) return {};
    result.lights[slot] = *scene.lights[id].value;
  }
  return result;
}

// One publication, not another instance registry. The bridge serializes access
// and publishes after DrawEnd/preparation/transfers, before DrawStart. Select
// returns VALUES: queued packets need no source/frame lease or retained history.
// Replacement and instance retirement cannot mutate already returned lights.
class NativeSceneLightingPublication {
public:
  static constexpr size_t kMaxBindings = 65536, kMaxBindingBytes = 4u << 20;
  bool Publish(uint32_t frame, NativeSceneLightSet lights, std::vector<NativeNodeLightBinding> objects) {
    // The authored snapshot invalidates lookup IDs, not the last light VALUES.
    // Keep known values across a successful handoff; an invalid producer breaks
    // the chain. No prior source/model lease is needed by these copies.
    valid_ = false; objects_.clear();
    const auto refuse = [&] { Reset(); return false; };
    if (update_ == UINT64_MAX) return refuse();
    ++update_;
    if (lights.count > lights.kMaxLights || lights.mode > 2 ||
        objects.size() > kMaxBindings || objects.capacity() > kMaxBindingBytes / sizeof(NativeNodeLightBinding))
      return refuse();
    for (size_t n = 0; n < lights.count; ++n)
      if (lights.lights[n].candidate.id != int32_t(n)) return refuse();
    std::sort(objects.begin(), objects.end(), [](const auto &a, const auto &b) { return a.Key() < b.Key(); });
    for (size_t n = 0; n < objects.size(); ++n)
      if (!objects[n].instance || !objects[n].model_generation ||
          objects[n].node >= 4096 || (n && objects[n-1].Key() == objects[n].Key())) return refuse();
    lights_ = std::move(lights); objects_ = std::move(objects); frame_ = frame; valid_ = true;
    return true;
  }
  void Reset() { valid_ = false; objects_.clear(); InvalidateInherited(); }
  void InvalidateInherited() {
    inherited_.reset();
    if (revision_ != UINT64_MAX) ++revision_;
  }
  uint64_t Update(uint32_t frame) const { return valid_ && frame_ == frame ? update_ : 0; }
  size_t Bindings() const { return valid_ ? objects_.size() : 0; }
  std::optional<NativeSceneLightSelection> Select(uint32_t frame, uint64_t update,
      uint64_t instance, uint64_t model_generation, uint32_t node, uint32_t light_view) const {
    if (!update || Update(frame) != update) return {};
    const auto key = std::tuple(instance, model_generation, node);
    const auto it = std::lower_bound(objects_.begin(), objects_.end(), key,
        [](const auto &binding, const auto &value) { return binding.Key() < value; });
    if (it == objects_.end() || it->Key() != key) return {};
    return it->inputs ? SelectNativeSceneLights(lights_, *it->inputs, light_view) : std::nullopt;
  }
  std::optional<NativeSceneLightRecipe> Capture(uint32_t frame, uint64_t update,
      uint64_t instance, uint64_t model_generation, uint32_t node, uint32_t light_view) const {
    if (!update || Update(frame) != update || light_view >= 16 || revision_ == UINT64_MAX) return {};
    const auto key = std::tuple(instance, model_generation, node);
    const auto it = std::lower_bound(objects_.begin(), objects_.end(), key,
        [](const auto &binding, const auto &value) { return binding.Key() < value; });
    if (it == objects_.end() || it->Key() != key) return {};
    if (!it->inputs) return NativeSceneLightRecipe{std::nullopt,update};
    const auto selected = SelectNativeSceneLights(lights_, *it->inputs, light_view);
    return selected ? std::optional(NativeSceneLightRecipe{selected->lights,update}) : std::nullopt;
  }
  std::optional<NativeSceneLightTicket> Resolve(uint32_t frame, const NativeSceneLightRecipe &recipe) const {
    if (!recipe.update || Update(frame) != recipe.update || revision_ == UINT64_MAX) return {};
    const auto &lights = recipe.bind ? recipe.bind : inherited_;
    return lights ? std::optional(NativeSceneLightTicket{*lights,recipe.update,revision_,!recipe.bind}) : std::nullopt;
  }
  std::optional<NativeSceneLightTicket> Prepare(uint32_t frame, uint64_t update,
      uint64_t instance, uint64_t model_generation, uint32_t node, uint32_t light_view) const {
    const auto recipe = Capture(frame,update,instance,model_generation,node,light_view);
    return recipe ? Resolve(frame,*recipe) : std::nullopt;
  }
  bool CanCommit(uint32_t frame, const NativeSceneLightTicket &ticket) const {
    return ticket.update && Update(frame) == ticket.update && ticket.revision == revision_ && revision_ != UINT64_MAX;
  }
  bool Commit(uint32_t frame, const NativeSceneLightTicket &ticket) {
    if (!CanCommit(frame,ticket)) return false;
    inherited_ = ticket.lights; ++revision_;
    return true;
  }
  template <class Matches> void ValidateInherited(Matches matches) {
    if (inherited_ && !matches(*inherited_)) InvalidateInherited();
  }
private:
  NativeSceneLightSet lights_;
  std::vector<NativeNodeLightBinding> objects_;
  uint64_t update_ = 0;
  uint64_t revision_ = 0;
  std::optional<NativeSelectedLights> inherited_;
  uint32_t frame_ = 0;
  bool valid_ = false;
};
} // namespace bd::gpu::scene
