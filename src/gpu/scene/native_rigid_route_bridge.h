/**
 * @brief Temporary source identity boundary for hard-off native acceptance.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_primitive_policy.h"
#include <cstdint>
#include <memory>
namespace bd::gpu::scene {
class NativeModelRenderData;
struct NativeInstancePose;
// Empty when acceptance is off; otherwise requires a current load-owned model.
std::shared_ptr<const NativeModelRenderData> LoadNativeRigidRouteModel(uint32_t context);
void RequireNativeRigidWalkNode(const std::shared_ptr<const NativeModelRenderData> &model,
    const NativeInstancePose *pose, uint32_t node, uint32_t view,
    const std::optional<PrimitivePolicyInputs> &inputs);
// Must be the first operation of the old node entry, before diagnostics,
// capture, replay or any original-body call. Covers non-host-walk callers too.
void RequireNativeRigidLegacyNode(uint32_t context, uint32_t mesh);
} // namespace bd::gpu::scene
