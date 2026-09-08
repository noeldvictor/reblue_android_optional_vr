/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
namespace bd::gpu { struct VideoState; struct GraphicsBindings; struct QueuedDraw; }
namespace bd::gpu::scene {
struct NativeRigidDrawStore;
struct NativeRigidBatchItem;
bool NativeRigidShadowEnabled();
bool NativeRigidSceneEnabled();
// Called before the per-node interpreter/replay/capture, after host culling.
// Once a supported family is recognized, missing owners/resources are fatal;
// it cannot silently warm a template. Unsupported participation stays legacy.
bool SubmitNativeRigidShadow(const NativeInstancePose &pose, uint32_t node,
                             const std::optional<PrimitivePolicyInputs> &inputs);
bool SubmitNativeRigidScene(const NativeInstancePose &pose, uint32_t node,
                            const std::optional<PrimitivePolicyInputs> &inputs,
                            const std::optional<std::array<float, 4>> &world_bounds);
// Called only after the shared emitter records a real draw command.
void NoteNativeRigidEmission(const GraphicsBindings &bindings, uint32_t render_view, uint32_t instances, uint64_t generation,
                            std::span<const NativeRigidBatchItem *const> items);
// Shared queue calls this under the renderer lock after exact batch admission.
// Creates explicit storage/image bindings and one indexed indirect command.
void PrepareNativeRigidBatchDraw(std::span<const NativeRigidBatchItem *const> items, QueuedDraw &draw);
void DrainNativeRigidDrawsLocked(VideoState &state, uint32_t slot);
} // namespace bd::gpu::scene
