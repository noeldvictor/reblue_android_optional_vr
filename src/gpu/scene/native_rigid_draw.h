/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
namespace bd::gpu { struct VideoState; }
namespace bd::gpu::scene {
struct NativeRigidDrawStore;
bool NativeRigidShadowEnabled();
// Called before the per-node interpreter/replay/capture, after host culling.
// Once the selected asset is recognized, refusal is fatal in this acceptance
// mode; it cannot silently warm a template. Other families remain untouched.
bool SubmitNativeRigidShadow(const NativeInstancePose &pose, uint32_t node,
                             const std::optional<PrimitivePolicyInputs> &inputs);
void DrainNativeRigidDrawsLocked(VideoState &state, uint32_t slot);
} // namespace bd::gpu::scene
