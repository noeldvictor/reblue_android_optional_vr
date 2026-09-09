/**
 * @brief Temporary instance-source lookup; native poses contain no source keys.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_visual_inputs.h"

namespace bd::gpu::scene {
struct NodeTag;
struct NativeNodeLightBinding;
struct NativeLightSourceBinding;
namespace material_image_source { struct Binding; }
// Source identity lookup only at compatibility/producer boundaries. Native
// deferred entries and their consumer never resolve an instance back to a VA.
NativeVisualIdentity FindNativeVisualIdentity(uint32_t visual);
bool PublishNativeMaterialUVs(uint32_t visual, NativeVisualIdentity identity,
    uint32_t table, const NativeMaterialUVs &material);
void InvalidateNativeMaterialUVs(uint32_t visual);
std::shared_ptr<const NativeMaterialUVs> ReadNativeMaterialUVs(uint32_t visual, uint64_t generation);
std::shared_ptr<const NativeMaterialUVProgram> ReadNativeMaterialUVProgram(uint32_t visual, uint64_t generation);
bool PublishNativeMaterialImages(uint32_t visual, NativeVisualIdentity identity,
    const material_image_source::Binding &binding, const NativeMaterialImages &images);
void InvalidateNativeMaterialImages(uint32_t visual);
std::shared_ptr<const NativeMaterialImages> ReadNativeMaterialImages(uint32_t visual, uint64_t generation);
// Late handoff after authored scene preparation and known later writers.
// Uses the existing bounded instance index; no source address survives in output.
bool CollectNativeVisualInputs(std::span<const NativeVisualIdentity> requested,
    std::vector<NativeVisualInputs> &out);
// One render-thread publication scope, reusing the existing instance index.
// Known authored writers republish after their complete side effects. Native
// consumers read immutable values without source identity/field lookups.
class NativeVisualInputScope {
public:
  NativeVisualInputScope() = default;
  NativeVisualInputScope(const NativeVisualInputScope &) = delete;
  NativeVisualInputScope &operator=(const NativeVisualInputScope &) = delete;
  ~NativeVisualInputScope();
  bool Begin(std::span<const NativeVisualIdentity> requested, uint32_t frame);
  auto Read(NativeVisualIdentity identity, uint32_t frame) const { return inputs_.Read(identity, frame); }
  // Current visual only, after receiver compatibility writes. Reuses the
  // instance producer; avoids republishing every visual at every transition.
  std::optional<NativeVisualInputs> ReadAfterWriter(NativeVisualIdentity identity, uint32_t frame) const;
  uint64_t Refreshes() const { return refreshes_; }
private:
  friend void RefreshNativeVisualInputsAfterWriter();
  NativeVisualPublication inputs_;
  std::vector<NativeVisualIdentity> requested_;
  uint32_t frame_ = 0;
  uint64_t refreshes_ = 0;
};
void RefreshNativeVisualInputsAfterWriter();
// Called only at the synchronized game/render handoff, after completed poses.
// Reuses the existing source index; returned bindings contain native IDs only.
bool CollectNativeInstanceLightInputs(std::vector<NativeNodeLightBinding> &out,
    std::vector<NativeLightSourceBinding> &sources, size_t &unavailable);
std::shared_ptr<const NativeInstancePose> FindNativeInstancePose(
    uint32_t visual, uint32_t graph, uint32_t palette);
// Render-only view of an exact completed native publication. No source lookup,
// scratch palette or shader-register upload; culling and packets share a lease.
std::shared_ptr<const NativeInstancePose> ResolveNativeRenderPose(const NativeInstancePose &completed);
bool CopyNativeInstanceWorld(const NodeTag &tag, float out[16]);
} // namespace bd::gpu::scene
