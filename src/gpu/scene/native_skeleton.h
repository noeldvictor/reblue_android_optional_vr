/**
 * @brief Native joint hierarchy and pose evaluation, independent of source memory.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_transform.h"
#include <algorithm>
#include <span>
#include <vector>

namespace bd::gpu::scene {
inline constexpr uint32_t kNativeSkeletonRoot = UINT32_MAX;
inline constexpr size_t kMaxNativeJoints = 4096;
using JointVector = std::array<float, 3>;
using JointQuaternion = std::array<float, 4>; // xyz, w; radians at Euler import

inline RenderMatrix JointIdentity() {
  return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
}
inline bool JointAffine(const RenderMatrix &matrix) {
  for (float value : matrix) if (!std::isfinite(value)) return false;
  return matrix[3] == 0 && matrix[7] == 0 && matrix[11] == 0 && matrix[15] == 1;
}
inline RenderMatrix JointRotation(const JointQuaternion &q) {
  // Authored quaternions are not silently normalized. Row-vector convention.
  const float x=q[0], y=q[1], z=q[2], w=q[3];
  return {1-2*y*y-2*z*z, 2*x*y+2*z*w, 2*x*z-2*y*w, 0,
          2*x*y-2*z*w, 1-2*x*x-2*z*z, 2*y*z+2*x*w, 0,
          2*x*z+2*y*w, 2*y*z-2*x*w, 1-2*x*x-2*y*y, 0,
          0,0,0,1};
}
inline JointQuaternion MultiplyJointQuaternions(const JointQuaternion &a, const JointQuaternion &b) {
  return {a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
          a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
          a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
          a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]};
}
inline JointQuaternion JointEulerQuaternion(const JointVector &radians) {
  const float x=radians[0]*.5f, y=radians[1]*.5f, z=radians[2]*.5f;
  // The source appends each new axis to the right: qZ * qY * qX.
  return MultiplyJointQuaternions(
      MultiplyJointQuaternions({0,0,std::sin(z),std::cos(z)}, {0,std::sin(y),0,std::cos(y)}),
      {std::sin(x),0,0,std::cos(x)});
}
inline RenderMatrix JointEulerRotation(const JointVector &radians) { return JointRotation(JointEulerQuaternion(radians)); }

struct NativeJointChannels {
  JointVector translation{}, scale{1,1,1};
  JointQuaternion rotation{0,0,0,1};
  bool translated = false, rotated = false, scaled = false, reset_parent = false;
};

// Parent is a preorder ordinal, pose_index a model-local joint identity. No
// pointers, resource wrappers, guest flags or source-address identity survives.
struct NativeSkeletonJoint {
  uint32_t pose_index = 0, parent = kNativeSkeletonRoot;
  JointVector translation{}, scale{1,1,1};
  RenderMatrix before_rotation = JointIdentity(), rotation = JointIdentity(), after_rotation = JointIdentity();
  bool inherit_parent_scale = false;
  // Blending reads authored rest values even when the base transform disables
  // that channel. Availability is independent of base-transform activation.
  NativeJointChannels blend_rest;
};

inline bool ValidNativeSkeleton(std::span<const NativeSkeletonJoint> joints) {
  if (joints.empty() || joints.size() > kMaxNativeJoints) return false;
  std::array<bool, kMaxNativeJoints> seen{};
  for (size_t n=0; n<joints.size(); ++n) {
    const auto &joint = joints[n];
    if (joint.pose_index >= joints.size() || seen[joint.pose_index] ||
        (joint.parent != kNativeSkeletonRoot && joint.parent >= n) ||
        !JointAffine(joint.before_rotation) || !JointAffine(joint.rotation) || !JointAffine(joint.after_rotation)) return false;
    seen[joint.pose_index] = true;
    for (float value : joint.translation) if (!std::isfinite(value)) return false;
    for (float value : joint.scale) if (!std::isfinite(value)) return false;
  }
  return true;
}

// Whole-pose transaction. The matrix sent to children is the pre-scale matrix;
// a separate parent scale affects their translation or full linear transform.
// This preserves segment-scale compensation and does not flatten it to TRS.
inline bool EvaluateNativeSkeleton(std::span<const NativeSkeletonJoint> joints,
    std::span<const NativeJointChannels> channels, const RenderMatrix &root,
    std::vector<RenderMatrix> &out) {
  if (!ValidNativeSkeleton(joints) || channels.size() != joints.size() || !JointAffine(root)) return false;
  std::vector<RenderMatrix> unscaled(joints.size()), result(joints.size());
  std::vector<JointVector> scales(joints.size());
  for (size_t n=0; n<joints.size(); ++n) {
    const auto &joint = joints[n]; const auto &input = channels[joint.pose_index];
    auto parent = input.reset_parent ? JointIdentity() :
        joint.parent == kNativeSkeletonRoot ? root : unscaled[joint.parent];
    auto translation = input.translated ? input.translation : joint.translation;
    const auto scale = input.scaled ? input.scale : joint.scale;
    for (float value : translation) if (!std::isfinite(value)) return false;
    for (float value : scale) if (!std::isfinite(value)) return false;
    if (input.rotated) for (float value : input.rotation) if (!std::isfinite(value)) return false;
    if (joint.parent != kNativeSkeletonRoot) {
      const auto &parent_scale = scales[joint.parent];
      if (joint.inherit_parent_scale) {
        auto inherited = JointIdentity();
        for (size_t axis=0; axis<3; ++axis) inherited[axis*5] = parent_scale[axis];
        parent = MultiplyRenderMatrices(inherited, parent);
      } else {
        for (size_t axis=0; axis<3; ++axis) translation[axis] *= parent_scale[axis];
      }
    }
    auto local = JointIdentity();
    std::copy(translation.begin(), translation.end(), local.begin()+12);
    auto world = MultiplyRenderMatrices(local, parent);
    world = MultiplyRenderMatrices(joint.before_rotation, world);
    world = MultiplyRenderMatrices(input.rotated ? JointRotation(input.rotation) : joint.rotation, world);
    world = MultiplyRenderMatrices(joint.after_rotation, world);
    if (!JointAffine(world)) return false;
    unscaled[n] = world; scales[n] = scale;
    auto scaling = JointIdentity();
    for (size_t axis=0; axis<3; ++axis) scaling[axis*5] = scale[axis];
    world = MultiplyRenderMatrices(scaling, world);
    if (!JointAffine(world)) return false;
    result[joint.pose_index] = world;
  }
  out = std::move(result);
  return true;
}
} // namespace bd::gpu::scene
