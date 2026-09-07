/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
namespace bd::gpu { struct VideoState; struct GraphicsBindings; }
namespace bd::gpu::scene {
struct NativeRigidDrawStore;
bool NativeRigidShadowEnabled();
bool NativeRigidSceneEnabled();
// Called before the per-node interpreter/replay/capture, after host culling.
// Once the selected asset is recognized, refusal is fatal in this acceptance
// mode; it cannot silently warm a template. Other families remain untouched.
bool SubmitNativeRigidShadow(const NativeInstancePose &pose, uint32_t node,
                             const std::optional<PrimitivePolicyInputs> &inputs);
bool SubmitNativeRigidScene(const NativeInstancePose &pose, uint32_t node);
// Called only after the shared emitter records a real draw command.
void NoteNativeRigidEmission(const GraphicsBindings &bindings, uint32_t render_view);
void DrainNativeRigidDrawsLocked(VideoState &state, uint32_t slot);
} // namespace bd::gpu::scene
