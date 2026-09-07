/**
 * @brief Temporary instance-source lookup; native poses contain no source keys.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"

namespace bd::gpu::scene {
struct NodeTag;
struct NativeNodeLightBinding;
struct NativeLightSourceBinding;
// Called only at the synchronized game/render handoff, after completed poses.
// Reuses the existing source index; returned bindings contain native IDs only.
bool CollectNativeInstanceLightInputs(std::vector<NativeNodeLightBinding> &out,
    std::vector<NativeLightSourceBinding> &sources, size_t &unavailable);
std::shared_ptr<const NativeInstancePose> FindNativeInstancePose(
    uint32_t visual, uint32_t graph, uint32_t palette);
bool CopyNativeInstanceWorld(const NodeTag &tag, float out[16]);
} // namespace bd::gpu::scene
