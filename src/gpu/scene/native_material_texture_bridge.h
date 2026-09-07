/**
 * @brief Object-pass texture publication and native primitive consumers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_textures.h"
#include "gpu/scene/native_texture_binding.h"
#include "gpu/scene/native_primitive_policy.h"
#include "gpu/scene/native_object_primitive.h"
#include <memory>
namespace bd::gpu::scene {
struct NodeTag;
struct NativeObjectTextureState;
using NativeMaterialTextureValues = MaterialTextureValues<NativeTextureBinding>;
// Bounded object defaults plus explicit node-light updates during traversal. A
// nested traversal gets its own publication, and restores the parent on exit.
class NativeObjectTextureScope {
  std::unique_ptr<NativeObjectTextureState> owned_;
  NativeObjectTextureState *previous_;
public:
  NativeObjectTextureScope(uint32_t traverse_context, std::shared_ptr<const NativeInstancePose> pose, uint32_t stack);
  ~NativeObjectTextureScope();
  NativeObjectTextureScope(const NativeObjectTextureScope &) = delete;
  NativeObjectTextureScope &operator=(const NativeObjectTextureScope &) = delete;
};
using NativeObjectPrimitiveInputs = NativeObjectPrimitive<NativeTextureBinding>;
// Exact pose publication and native ordinals, not a source graph/mesh/buffer key.
// Returned packets own their leases and remain valid after this scope retires.
std::optional<NativeObjectPrimitiveInputs> FindNativeObjectPrimitive(
    const NativeInstancePose &pose, uint32_t node, uint32_t primitive);
std::optional<NativeMaterialObjectInputs> FindNativeMaterialObjectInputs(const NodeTag &tag);
std::optional<NativeSelectedLights> FindNativeMaterialLights(const NodeTag &tag);
std::optional<NativeFogLayers> FindNativeMaterialFog(const NodeTag &tag);
// Authored-selection binding is resolved only at the producer boundary.
bool PublishNativeMaterialLights(uint32_t selection, const NativeSelectedLights &lights);
void InvalidateNativeMaterialLights();
void NativeMaterialObjectInputCheck(bool same);
// Returned values live only through the current object scope. No source memory,
// image-resource lookup or table-registry lock is required by this consumer.
const NativeMaterialTextureValues *FindNativeMaterialTextures(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count);
std::optional<NativePrimitivePolicy> FindNativePrimitivePolicy(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count);
std::optional<NativePrimitiveShaderInputs> FindNativePrimitiveShaderInputs(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count);
void NativePrimitiveShaderCheck(bool same);
void NativePrimitiveShaderNoteDraw();
std::optional<NativeMaterialFeatures> FindNativeMaterialFeatures(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count);
void NativeMaterialFeatureCheck(bool same);
void NativeMaterialFeatureNoteDraw();
std::optional<NativeMaterialSamplers> FindNativeMaterialSamplers(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count);
void NativeMaterialSamplerCheck(bool same);
void NativeMaterialSamplerNoteDraw();
std::optional<NativePrimitivePlan> FindNativePrimitivePlan(const NodeTag &tag);
void NativePrimitivePolicyCheck(bool same);
void NativePrimitivePolicyNoteDraw(bool changed);
void NativePrimitivePolicyRefresh();
void NativeMaterialTextureCheck(bool same, uint32_t channel, uint32_t visual);
// Failure-only provenance. Never imports values into a native packet or changes
// the strict comparison; bounded to the first four reported mismatches.
void ReportNativeMaterialUvMismatch(const NodeTag &tag,
    const NativeMaterialTextureValues &values, const void *actual);
void NativeMaterialTextureNoteDraw(uint32_t image_mask, bool uv);
void NativeMaterialTextureReport();
} // namespace bd::gpu::scene
