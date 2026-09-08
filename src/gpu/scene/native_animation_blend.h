/**
 * @brief Source-independent weighted channels and authored hierarchy selection.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include <optional>
#include <tuple>
#include <type_traits>

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
    const NativeJointChannels &rest, float weight, NativeJointChannels &out, uint32_t channel_mask = 7) {
  if (!std::isfinite(weight) || (channel_mask&~7u)) return false;
  auto result=previous;
  if (std::abs(weight) < kNativeAnimationWeightEpsilon) { out=result; return true; }
  auto blend = [&](const auto &from, bool active, const auto &to, bool supplied,
                   const auto &base, bool available, auto &value, bool &enabled) {
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
  if (((channel_mask&1) && !blend(previous.translation,previous.translated,incoming.translation,incoming.translated,
             rest.translation,rest.translated,result.translation,result.translated)) ||
      ((channel_mask&2) && !blend(previous.rotation,previous.rotated,incoming.rotation,incoming.rotated,
             rest.rotation,rest.rotated,result.rotation,result.rotated)) ||
      ((channel_mask&4) && !blend(previous.scale,previous.scaled,incoming.scale,incoming.scaled,
             rest.scale,rest.scaled,result.scale,result.scaled))) return false;
  out=result; return true;
}

// Whole-layer composition differs from clip application: a lone translation or
// rotation is copied, but a lone scale fades to/from unit scale. Both absent
// channels materialize canonical inactive defaults. No hierarchy/rest input.
inline bool MixNativeChannels(const NativeJointChannels &left, const NativeJointChannels &right,
    float weight, NativeJointChannels &out) {
  if (!std::isfinite(weight)) return false;
  NativeJointChannels result;
  result.reset_parent=left.reset_parent || right.reset_parent;
  auto mix=[&](const auto &a,bool active_a,const auto &b,bool active_b,auto &value,bool &active) {
    active=active_a || active_b;
    if (!active) return true;
    if (!active_a || (active_b && weight == 1)) value=b;
    else if (!active_b || std::abs(weight) < kNativeAnimationWeightEpsilon) value=a;
    else if constexpr (std::tuple_size_v<std::remove_cvref_t<decltype(value)>> == 4)
      value=BlendJointRotation(a,b,weight);
    else for (size_t n=0; n<value.size(); ++n) {
      const float target=b[n]*weight;
      value[n]=float(std::fma(double(a[n]),double(1-weight),double(target)));
    }
    for (float component : value) if (!std::isfinite(component)) return false;
    return true;
  };
  if (!mix(left.translation,left.translated,right.translation,right.translated,result.translation,result.translated) ||
      !mix(left.rotation,left.rotated,right.rotation,right.rotated,result.rotation,result.rotated)) return false;
  result.scaled=left.scaled || right.scaled;
  if (left.scaled && right.scaled) {
    if (!mix(left.scale,true,right.scale,true,result.scale,result.scaled)) return false;
  } else if (result.scaled) {
    for (size_t n=0; n<3; ++n) {
      result.scale[n]=left.scaled ? float(std::fma(double(left.scale[n]),double(1-weight),double(weight))) :
          float(std::fma(double(right.scale[n]),double(weight),double(1-weight)));
      if (!std::isfinite(result.scale[n])) return false;
    }
  }
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

struct NativeAnimationFilter {
  std::string_view included;
  std::span<const std::string_view> excluded;
  // Direct keyed sampling searches descendants even before a name matches.
  // Weighted traversal prunes an unmatched branch instead; neither is a hash test.
  bool search_descendants=false;
};
struct NativeAnimationSelection {
  std::vector<uint8_t> headers, channels;
};
inline std::optional<NativeAnimationSelection> SelectNativeAnimationNodes(
    std::span<const NativeSkeletonJoint> joints, uint32_t pose, bool single_subtree,
    const NativeAnimationFilter &filter) {
  auto scope=SelectNativeAnimationSubtree(joints,pose,single_subtree);
  if (!scope || filter.included.size() >= 16 || filter.excluded.size() > 30) return {};
  for (auto name : filter.excluded) if (name.size() >= 16) return {};
  NativeAnimationSelection result{std::vector<uint8_t>(joints.size()),std::vector<uint8_t>(joints.size())};
  std::vector<uint8_t> descend(joints.size());
  for (size_t n=0; n<joints.size(); ++n) {
    if (!(*scope)[n]) continue;
    const auto &joint=joints[n];
    const bool scoped_parent=joint.parent != kNativeSkeletonRoot && (*scope)[joint.parent];
    if (scoped_parent && !descend[joint.parent]) continue;
    bool selected=true, excluded=false;
    if (!filter.included.empty()) {
      selected=scoped_parent && result.channels[joint.parent];
      if (!selected) {
        if (!joint.animation_name.Valid()) return {};
        selected=joint.animation_name.View() == filter.included;
      }
    } else if (!filter.excluded.empty()) {
      if (!joint.animation_name.Valid()) return {};
      excluded=std::ranges::find(filter.excluded,joint.animation_name.View()) != filter.excluded.end();
      selected=!excluded;
    }
    result.channels[n]=selected;
    result.headers[n]=!excluded && (filter.search_descendants || selected);
    descend[n]=!excluded && (filter.search_descendants || selected);
  }
  return result;
}
} // namespace bd::gpu::scene
