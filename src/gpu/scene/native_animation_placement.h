/**
 * @brief Owned attachment placement; checked inputs at the remaining source boundary.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include "gpu/scene/native_instance_source.h"
#include <bit>
#include <optional>

namespace bd::gpu::scene {
// AnimeData_method_4638: parent Euler rotation precedes the sampled quaternion.
// Translation follows the parent rotation, NOT the sampled rotation or child
// scale. Zero/inactive sampled fields are intentional (missing clip/root).
inline std::optional<RenderMatrix> ComposeNativeAttachmentPlacement(
    const NativeJointChannels &root, const JointVector &position,
    const JointVector &angles, const JointVector &scale) {
#pragma clang fp contract(off)
  auto finite=[](const auto &values) {
    return std::ranges::all_of(values,[](float value) { return std::isfinite(value); });
  };
  if (!finite(root.translation) || !finite(root.rotation) || !finite(position) ||
      !finite(angles) || !finite(scale)) return {};
  RenderMatrix parent=JointIdentity();
  // Source SetRotationYXZ left-multiplies Z, then Y, then X: Rx * Ry * Rz.
  for (int axis=2; axis>=0; --axis) {
    if (angles[axis] == 0) continue;
    const float s=std::sin(angles[axis]), c=std::cos(angles[axis]);
    auto rotation=JointIdentity();
    const size_t a=(axis+1)%3, b=(axis+2)%3;
    rotation[a*4+a]=c; rotation[a*4+b]=s;
    rotation[b*4+a]=-s; rotation[b*4+b]=c;
    parent=MultiplyRenderMatrices(rotation,parent);
  }
  // bdMatrixMultiply3(dst, sampled, parent) uses reversed argument order.
  auto result=MultiplyRenderMatrices(parent,JointRotation(root.rotation));
  for (size_t c=0; c<3; ++c) {
    // sub_824911F8 accumulates z, then y, then x, without contraction.
    const float translated=root.translation[0]*parent[c] +
        (root.translation[1]*parent[4+c] + (root.translation[2]*parent[8+c]+parent[12+c]));
    result[12+c]=position[c]+translated;
  }
  auto scaling=JointIdentity();
  scaling[0]=scale[0]; scaling[5]=scale[1]; scaling[10]=scale[2];
  result=MultiplyRenderMatrices(scaling,result);
  return JointAffine(result) ? std::optional(result) : std::nullopt;
}

inline bool SameNativePlacementRoot(const RenderMatrix &owned, const RenderMatrix &boundary) {
  for (size_t c=0; c<16; ++c)
    if (std::bit_cast<uint32_t>(owned[c]) != std::bit_cast<uint32_t>(boundary[c])) return false;
  return true;
}

namespace animation_source {
// Parent is its own source view: fields are +8 relative to the ordinary visual
// layout. Do not interpret it as a normal visual or add another +8 to it.
struct AttachmentInput {
  bool enabled=false, attached=false, sampled=false;
  uint32_t parent=0, parent_graph=0, world=0, animation=0, entry=0;
  RenderMatrix parent_world{};
  JointVector position{}, angles{}, scale{};
  std::array<uint32_t,4> appearance{};
  float ticks=0, parent_ticks=0, weight=0;
};

template<class ReadWord>
std::optional<AttachmentInput> ReadAttachmentInput(
    uint32_t visual, float ticks, uint32_t thread, ReadWord &&read) {
  if (!visual || (visual&3) || uint64_t(visual)+15336 > UINT32_MAX) return {};
  AttachmentInput input;
  const auto enabled=read(uint64_t(visual)+15240);
  if (!enabled) return {};
  input.enabled=*enabled != 0;
  if (!input.enabled) return input;
  const auto attached=read(uint64_t(visual)+15228);
  if (!attached) return {};
  input.attached=*attached != 0;
  if (!input.attached) return input;
  if (!std::isfinite(ticks)) return {};
  input.ticks=ticks;
  const auto parent=read(uint64_t(visual)+15188), update_thread=read(instance_source::kUpdateThread);
  if (!parent || !*parent || (*parent&3) || uint64_t(*parent)+5644 > UINT32_MAX || !update_thread) return {};
  input.parent=*parent;
  const auto graph=read(uint64_t(*parent)+2628), sampled=read(uint64_t(*parent)+5640);
  if (!graph || !*graph || !sampled) return {};
  input.parent_graph=*graph; input.sampled=*sampled != 0;
  const uint32_t lane=thread == *update_thread ? 0 : 64;
  input.world=visual+2388+lane;
  for (size_t c=0; c<4; ++c) {
    const auto value=read(uint64_t(*parent)+3012+c*4);
    if (!value || !std::isfinite(std::bit_cast<float>(*value))) return {};
    input.appearance[c]=*value;
  }
  auto scalar=[&](uint64_t address, float &destination) {
    const auto value=read(address);
    if (!value) return false;
    destination=std::bit_cast<float>(*value); return std::isfinite(destination);
  };
  if (!input.sampled) {
    for (size_t c=0; c<16; ++c)
      if (!scalar(uint64_t(*parent)+2396+lane+c*4,input.parent_world[c])) return {};
    if (!JointAffine(input.parent_world)) return {};
    const auto animation=read(uint64_t(*parent)+1880);
    if (!animation) return {};
    input.animation=*animation;
    if (*animation-1 <= 9) {
      const auto mapped=read(uint64_t(*parent)+4*(*animation+1387));
      if (!mapped) return {};
      input.animation=*mapped;
    }
  } else {
    input.animation=12;
    for (size_t c=0; c<3; ++c)
      if (!scalar(uint64_t(*parent)+5592+c*4,input.position[c]) ||
          !scalar(uint64_t(*parent)+5604+c*4,input.angles[c]) ||
          !scalar(uint64_t(visual)+15216+c*4,input.scale[c])) return {};
    const auto entry=read(uint64_t(*parent)+1928);
    if (!entry) return {};
    input.entry=*entry;
    if (*entry && (!scalar(uint64_t(*parent)+1904,input.parent_ticks) ||
                   !scalar(uint64_t(*parent)+1892,input.weight))) return {};
  }
  return input;
}
} // namespace animation_source
} // namespace bd::gpu::scene
