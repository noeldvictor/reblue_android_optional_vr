/**
 * @brief Completed native controller -> skeleton channel handoff.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include <optional>

namespace bd::gpu::scene {
// One-shot, model-generation checked. Until every late writer is native, the
// outgoing boundary must still compare unchanged before using the owned values.
std::optional<std::vector<NativeJointChannels>> TakeNativeAnimationChannels(
    uint32_t visual, uint32_t graph, uint64_t generation, uint32_t source);
void RetireNativeAnimationChannels(uint32_t visual);
// Scoped attachment producer -> actual bone evaluation. The by-value source
// root remains a strict late-writer guard, not the native input on admission.
std::optional<RenderMatrix> TakeNativeAnimationPlacementRoot(
    uint32_t visual, uint32_t graph, uint64_t generation, const RenderMatrix &boundary);
}
