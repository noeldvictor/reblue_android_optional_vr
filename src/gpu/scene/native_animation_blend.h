/**
 * @brief Source-independent weighted channels and authored hierarchy selection.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include <optional>

namespace bd::gpu::scene {
inline constexpr float kNativeAnimationWeightEpsilon = 0x1p-23f;

inline JointQuaternion BlendJointRotation(const JointQuaternion &from, const JointQuaternion &to, float weight) {
  const float dot = (from[0]*to[0]+from[1]*to[1])+(from[2]*to[2]+from[3]*to[3]);
  const float cosine = std::abs(dot);
  float a=1-weight,b=weight;
  // Same short-arc and near-parallel linear branch as the authored blend.
  // Do not normalize inputs/output: that would alter accumulated layer values.
  if (cosine < 1-0x1p-16f) {
    const float sine = std::sqrt(1-cosine*cosine);
    const float angle = std::atan2(sine,cosine);
    a=std::sin((1-weight)*angle)/sine; b=std::sin(weight*angle)/sine;
  }
  if (dot < 0) b=-b;
  JointQuaternion result;
  for (size_t n=0; n<4; ++n) result[n]=from[n]*a+to[n]*b;
  return result;
}

inline bool BlendNativeChannels(const NativeJointChannels &previous, const NativeJointChannels &incoming,
    const NativeJointChannels &rest, float weight, NativeJointChannels &out) {
  if (!std::isfinite(weight)) return false;
  auto result=previous;
  if (std::abs(weight) < kNativeAnimationWeightEpsilon) { out=result; return true; }
  auto blend = [&](const auto &from, bool active, const auto &to, bool supplied,
                   const auto &base, bool available, auto &value, bool &enabled, bool rotation) {
    if (weight == 1) { enabled=supplied; if (supplied) value=to; }
    else if (active || supplied) {
      if ((!active || !supplied) && !available) return false;
      const auto &a=active ? from : base, &b=supplied ? to : base;
      for (float component : a) if (!std::isfinite(component)) return false;
      for (float component : b) if (!std::isfinite(component)) return false;
      if constexpr (std::tuple_size_v<std::remove_cvref_t<decltype(value)>> == 4) {
        value=BlendJointRotation(a,b,weight);
      } else {
        for (size_t n=0; n<value.size(); ++n) {
          const float target=b[n]*weight;
          value[n]=float(std::fma(double(1-weight),double(a[n]),double(target)));
        }
      }
      enabled=true;
    }
    if (enabled) for (float component : value) if (!std::isfinite(component)) return false;
    return true;
  };
  if (!blend(previous.translation,previous.translated,incoming.translation,incoming.translated,
             rest.translation,rest.translated,result.translation,result.translated,false) ||
      !blend(previous.rotation,previous.rotated,incoming.rotation,incoming.rotated,
             rest.rotation,rest.rotated,result.rotation,result.rotated,true) ||
      !blend(previous.scale,previous.scaled,incoming.scale,incoming.scaled,
             rest.scale,rest.scaled,result.scale,result.scaled,false)) return false;
  out=result; return true;
}

// Result is indexed by native preorder; external pose identities need not be
// preorder. A normal traversal includes following siblings; a forced subtree
// stops after the selected root's descendants, preserving all other channels.
inline std::optional<std::vector<uint8_t>> SelectNativeAnimationSubtree(
    std::span<const NativeSkeletonJoint> joints, uint32_t pose, bool single_subtree) {
  if (!ValidNativeSkeleton(joints)) return {};
  const auto start=std::ranges::find(joints,pose,&NativeSkeletonJoint::pose_index);
  if (start == joints.end()) return {};
  const size_t first=start-joints.begin();
  std::vector<uint8_t> selected(joints.size());
  selected[first]=1;
  for (size_t n=first+1; n<joints.size(); ++n) {
    const auto parent=joints[n].parent;
    selected[n]=(parent != kNativeSkeletonRoot && selected[parent]) || (!single_subtree && parent == start->parent);
  }
  return selected;
}
} // namespace bd::gpu::scene
