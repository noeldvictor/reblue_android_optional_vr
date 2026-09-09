/**
 * @brief Completed native controller -> skeleton channel handoff.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include <optional>
#include <memory>
#include <functional>

namespace bd::gpu::scene {
namespace material_image_source { struct LoadedAnimation; struct LoadedCatalog; }
void BindNativeImageCatalog(uint32_t visual, uint64_t instance, uint64_t generation);
void RetireNativeImageCatalog(uint32_t visual);
std::shared_ptr<const material_image_source::LoadedCatalog> FindNativeImageCatalog(
    uint32_t visual, uint64_t instance, uint64_t generation,
    const std::function<std::optional<uint32_t>(uint64_t)> &read);
std::shared_ptr<const material_image_source::LoadedAnimation> FindNativeImageAnimation(
    uint32_t owner, const std::function<std::optional<uint32_t>(uint64_t)> &read);
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
