/**
 * @brief Checked load/update adapters for native skeletal evaluation.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_skeleton.h"
#include <bit>
#include <tuple>
#include <unordered_set>

namespace bd::gpu::scene::skeleton_source {
template <size_t N, class ReadWord>
bool Floats(uint64_t address, std::array<float,N> &out, ReadWord &&read) {
  for (size_t n=0; n<N; ++n) {
    const auto word = read(address + n*4);
    if (!word) return false;
    out[n] = std::bit_cast<float>(*word);
    if (!std::isfinite(out[n])) return false;
  }
  return true;
}

// bdBoneInitSkinned's by-value root: five BE register pairs (r6..r10),
// followed by six float words at caller SP+88. No PPC context in native math.
template <class ReadWord>
std::optional<RenderMatrix> ReadRoot(const std::array<uint64_t,5> &pairs,
    uint32_t stack, ReadWord &&read) {
  RenderMatrix root;
  for (size_t n=0; n<pairs.size(); ++n) {
    root[n*2] = std::bit_cast<float>(uint32_t(pairs[n]>>32));
    root[n*2+1] = std::bit_cast<float>(uint32_t(pairs[n]));
  }
  std::array<float,6> tail;
  if (!Floats(uint64_t(stack)+88,tail,read)) return {};
  std::copy(tail.begin(),tail.end(),root.begin()+10);
  return JointAffine(root) ? std::optional(root) : std::nullopt;
}

// NodeProcess copies the 80-byte base or 104-byte extended node, then relocates
// +56/+60. bdAnimBoneEvaluate reads these same authored TRS/pre/post fields.
// Camera-facing nodes need a separate owned view contract, never identity math.
template <class ReadWord>
std::optional<std::vector<NativeSkeletonJoint>> ReadSkeleton(uint32_t root, ReadWord &&read,
    std::vector<uint32_t> *animation_targets = nullptr) {
  struct Pending { uint32_t source, parent; };
  std::vector<Pending> pending;
  std::vector<NativeSkeletonJoint> joints;
  std::unordered_set<uint32_t> visited;
  std::vector<std::pair<uint32_t,uint32_t>> targets;
  std::unordered_set<uint32_t> names;
  bool unique_names = true;
  if (root) pending.push_back({root,kNativeSkeletonRoot});
  while (!pending.empty()) {
    const auto item = pending.back(); pending.pop_back();
    if (joints.size() >= kMaxNativeJoints || !visited.insert(item.source).second) return {};
    const uint64_t source = item.source;
    const auto index = read(source), flags = read(source+8), child = read(source+56), sibling = read(source+60);
    if (!index || !flags || !child || !sibling || *index >= kMaxNativeJoints || (*flags & 0x00600000)) return {};
    NativeSkeletonJoint joint; joint.pose_index = *index; joint.parent = item.parent;
    if (animation_targets) {
      const auto name = read(source+4);
      if (!name) return {};
      targets.emplace_back(*index,*name);
      unique_names &= names.insert(*name).second;
    }
    joint.inherit_parent_scale = (*flags & 0x40) != 0;
    if ((*flags & 1) && !Floats(source+16,joint.translation,read)) return {};
    if ((*flags & 8) && !Floats(source+44,joint.scale,read)) return {};
    joint.blend_rest.translated = Floats(source+16,joint.blend_rest.translation,read);
    joint.blend_rest.scaled = Floats(source+44,joint.blend_rest.scale,read);
    JointVector rest_angles;
    joint.blend_rest.rotated = Floats(source+28,rest_angles,read);
    if (joint.blend_rest.rotated) joint.blend_rest.rotation = JointEulerQuaternion(rest_angles);
    for (const auto [flag,offset,matrix] : std::array{
        std::tuple{4u,28u,&joint.rotation}, std::tuple{16u,80u,&joint.before_rotation},
        std::tuple{32u,92u,&joint.after_rotation}}) {
      if (!(*flags & flag)) continue;
      JointVector radians;
      if (!Floats(source+offset,radians,read)) return {};
      *matrix = JointEulerRotation(radians);
    }
    const auto parent = static_cast<uint32_t>(joints.size());
    joints.push_back(joint);
    if (*sibling) pending.push_back({*sibling,item.parent});
    if (*child) pending.push_back({*child,parent});
  }
  if (!ValidNativeSkeleton(joints)) return {};
  if (animation_targets) {
    std::vector<uint32_t> dense;
    if (unique_names) {
      dense.resize(joints.size());
      for (auto [index,name] : targets) dense[index] = name;
    }
    *animation_targets = std::move(dense);
  }
  return joints;
}

// bdAnimBoneEvaluate indexes the 48-byte authored channel record by model-local
// joint ID. Only enabled values are read; inactive bytes are not a native input.
template <class ReadWord>
std::optional<std::vector<NativeJointChannels>> ReadChannels(uint32_t source, size_t count, ReadWord &&read) {
  if (!source || !count || count > kMaxNativeJoints || uint64_t(source)+count*48 > uint64_t(UINT32_MAX)+1) return {};
  std::vector<NativeJointChannels> channels(count);
  for (size_t n=0; n<count; ++n) {
    const uint64_t address = uint64_t(source)+n*48;
    const auto flags = read(address);
    if (!flags) return {};
    auto &channel = channels[n];
    channel.translated = (*flags & 1) != 0; channel.rotated = (*flags & 2) != 0;
    channel.scaled = (*flags & 4) != 0; channel.reset_parent = (*flags & 64) != 0;
    if ((channel.translated && !Floats(address+8,channel.translation,read)) ||
        (channel.rotated && !Floats(address+20,channel.rotation,read)) ||
        (channel.scaled && !Floats(address+36,channel.scale,read))) return {};
  }
  return channels;
}
} // namespace bd::gpu::scene::skeleton_source
