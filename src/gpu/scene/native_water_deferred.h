/**
 * @brief Owned water work awaiting ordered material production in the scene queue.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_scene_lights.h"
#include "gpu/scene/native_texture_binding.h"
#include "gpu/scene/native_visual_inputs.h"

namespace bd::gpu::scene {
// Opaque, bounded outgoing-state adapter, not an input to the water GPU program.
// It retires with this frame's deferred work, never with a GPU fence or asset.
struct NativeWaterProducerBridge;
struct NativeWaterDeferred {
  std::shared_ptr<const NativeInstancePose> pose;
  uint32_t node = 0, primitive = 0, frame = 0, alpha_reference = 0;
  float depth = 0;
  PrimitiveCull cull = PrimitiveCull::None;
  bool shadow_allowed = true;
  NativeTextureGpuHandle environment;
  NativeSceneLightRecipe lights;
  std::shared_ptr<const NativeWaterProducerBridge> bridge;

  NativeVisualIdentity Identity() const {
    return pose ? NativeVisualIdentity{pose->instance,pose->model_generation} : NativeVisualIdentity{};
  }
  const NativeModelMaterialProgram *Program() const {
    return pose && pose->model && pose->model->Generation() == pose->model_generation &&
        node < pose->transforms.size() ? pose->model->FindNode(node) : nullptr;
  }
  bool Valid(uint32_t current_frame) const {
    const auto *program = Program();
    return Identity() && frame == current_frame && std::isfinite(depth) && lights.update &&
        cull <= PrimitiveCull::Back &&
        program && program->valid && primitive < program->ranges.size() &&
        primitive < program->geometries.size() && program->geometries[primitive] &&
        primitive < program->shadow_policies.size() && !program->ranges[primitive].skin;
  }
};
} // namespace bd::gpu::scene
