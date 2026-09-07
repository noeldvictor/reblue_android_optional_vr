/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
#include <memory>
namespace bd::gpu::scene {
class NativeModelRenderData;
bool NativeRigidLifecycleEnabled();
void NoteNativeRigidModelLoaded(const std::shared_ptr<const NativeModelRenderData> &model);
uint64_t NativeRigidRetiringGeneration(const std::shared_ptr<const NativeModelRenderData> &model);
void NoteNativeRigidSourceRetired(uint64_t generation);
void NoteNativeRigidSubmitted(uint64_t generation, uint64_t instance, uint32_t view);
void NoteNativeRigidEmitted(uint64_t generation, uint32_t view, uint32_t count);
void NoteNativeRigidFenceRetired(uint64_t generation, uint32_t view);
// The existing autoplay publishes readiness and obeys one bounded round-trip
// request. No game mutation from the input thread, no additional input harness.
void ObserveNativeRigidReloadField(bool walking, uint64_t stage);
struct NativeRigidReloadInput { bool paused; uint32_t serial; };
NativeRigidReloadInput GetNativeRigidReloadInput();
} // namespace bd::gpu::scene
