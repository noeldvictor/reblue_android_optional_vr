/**
 * @brief Controller boundary sidecars; native TRS values between layer consumers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_animation_controller.h"
#include "gpu/scene/native_animation_source.h"

namespace bd::gpu::scene::animation_source {
inline bool ControllerSourceExtentFits(size_t joints, uint32_t active, bool sequential) {
  // A legacy multi-layer producer spaces records by 512 joints. Larger imported
  // inputs overlap its buffers and have no independent-layer contract. This is
  // an admission check for that source, NOT a limit on native controller plans.
  return joints && joints <= kMaxNativeJoints && active <= kNativeAnimationSlots &&
      (active == 1 || sequential || joints <= 512);
}
// Only flags/labels remain as an outgoing sidecar. Payloads are native channels,
// not a packed 48-byte scratch mirror. Dormant values survive until overwritten:
// legacy in-place mixing can activate them before reading its aliased input.
struct ControllerLayer {
  struct Header { uint32_t flags=0, name=0; };
  std::vector<NativeJointChannels> channels;
  std::vector<Header> boundary;
  ControllerLayer(size_t count=0) : channels(count), boundary(count) {
    for (auto &value : channels) { value.rotation={}; value.scale={}; }
  }
  bool Valid() const { return !channels.empty() && channels.size() <= kMaxNativeJoints && channels.size() == boundary.size(); }
  static ControllerLayer Decode(std::span<const ChannelRecord> records) {
    if (records.empty() || records.size() > kMaxNativeJoints) return {};
    ControllerLayer result(records.size());
    for (size_t n=0; n<records.size(); ++n) {
      const auto &record=records[n]; auto &value=result.channels[n];
      result.boundary[n]={record[0],record[1]};
      value.translated=(record[0]&1)!=0; value.rotated=(record[0]&2)!=0;
      value.scaled=(record[0]&4)!=0; value.reset_parent=(record[0]&64)!=0;
      auto get=[&](auto &components,unsigned word) { for (float &component : components) component=std::bit_cast<float>(record[word++]); };
      get(value.translation,2); get(value.rotation,5); get(value.scale,9);
    }
    return result;
  }
  std::vector<ChannelRecord> Encode() const {
    if (!Valid()) return {};
    std::vector<ChannelRecord> records(channels.size());
    for (size_t n=0; n<channels.size(); ++n) {
      auto &record=records[n]; const auto &value=channels[n];
      record[0]=boundary[n].flags; record[1]=boundary[n].name;
      auto put=[&](const auto &components,unsigned word) { for (float component : components) record[word++]=std::bit_cast<uint32_t>(component); };
      put(value.translation,2); put(value.rotation,5); put(value.scale,9);
    }
    return records;
  }
  bool Apply(const NativeAnimationAsset &asset, std::span<const uint32_t> names,
      std::span<const NativeSkeletonJoint> skeleton, float seconds, float weight,
      uint32_t root, bool subtree, const NativeAnimationFilter &filter, bool reset) {
    if (!Valid() || names.size() != channels.size() || skeleton.size() != names.size() || !std::isfinite(weight)) return false;
    const bool indexed=asset.Indexed();
    const auto selected=SelectNativeAnimationNodes(skeleton,root,indexed ? false : subtree,indexed ? NativeAnimationFilter{} : filter);
    if (!selected) return false;
    if (std::abs(weight) < kNativeAnimationWeightEpsilon) return true;
    if (!std::isfinite(seconds)) return false;
    if (!indexed) {
      std::unordered_set<uint32_t> unique;
      for (auto name : names) if (!unique.insert(name).second) return false;
    }
    auto result=reset ? ControllerLayer(channels.size()) : *this;
    for (size_t n=0; n<skeleton.size(); ++n) {
      const auto &joint=skeleton[n]; const auto index=joint.pose_index;
      auto &header=result.boundary[index]; auto &value=result.channels[index];
      if (selected->headers[n] && (!indexed || reset)) header.name=names[index];
      if (!selected->channels[n]) continue;
      const auto *track=asset.FindTrack(names[index],index);
      if (!track) { if (indexed) return false; continue; }
      NativeJointChannels sampled;
      if (!asset.SampleTarget(names[index],index,seconds,sampled) ||
          !BlendNativeChannels(value,sampled,joint.blend_rest,weight,value,asset.ChannelMask())) return false;
      header.flags=(header.flags&~7u) | (value.translated ? 1u : 0u) | (value.rotated ? 2u : 0u) | (value.scaled ? 4u : 0u);
      if (track->Animated()) header.flags|=128;
    }
    *this=std::move(result); return true;
  }
  static bool Mix(const ControllerLayer &left, const ControllerLayer &right,
      float weight, bool alias_left, ControllerLayer &output) {
    if (!left.Valid() || !right.Valid() || left.channels.size() != right.channels.size()) return false;
    ControllerLayer result(left.channels.size());
    for (size_t n=0; n<left.channels.size(); ++n) {
      const auto flags=left.boundary[n].flags|right.boundary[n].flags;
      auto a=left.channels[n];
      if (alias_left) { a.translated=(flags&1)!=0; a.rotated=(flags&2)!=0; a.scaled=(flags&4)!=0; a.reset_parent=(flags&64)!=0; }
      if (!MixNativeChannels(a,right.channels[n],weight,result.channels[n])) return false;
      result.boundary[n]={flags,left.boundary[n].name};
    }
    output=std::move(result); return true;
  }
};

// A root-motion request is one selected joint, not a one-node hierarchy or a
// whole-body sample. Dense source dispatch uses its FIRST descriptor even when
// the selected model node has another pose identity; named clips use that name.
inline std::optional<ControllerLayer> SampleRootMotion(const NativeAnimationAsset &asset,
    uint32_t name, const NativeSkeletonJoint &joint, float seconds, ControllerLayer previous) {
  if (!previous.Valid() || previous.channels.size() != 1 || !std::isfinite(seconds)) return {};
  const auto track_index=asset.Indexed() ? 0u : joint.pose_index;
  const auto *track=asset.FindTrack(name,track_index);
  if (!asset.Indexed()) previous.boundary[0].name=name;
  if (!track) return asset.Indexed() ? std::nullopt : std::optional(std::move(previous));
  NativeJointChannels sampled;
  auto &channel=previous.channels[0]; auto &header=previous.boundary[0];
  if (!asset.SampleTarget(name,track_index,seconds,sampled) ||
      !BlendNativeChannels(channel,sampled,joint.blend_rest,1,channel,asset.ChannelMask())) return {};
  header.flags=(header.flags&~7u) | (channel.translated ? 1u : 0u) | (channel.rotated ? 2u : 0u) | (channel.scaled ? 4u : 0u);
  if (track->Animated()) header.flags|=128;
  return previous;
}

struct ControllerExecution {
  ControllerLayer output;
  uint32_t compression=0, exclusions=0;
  uint32_t sampled=0, mixed=0, subtree=0, interior=0;
};
inline std::optional<ControllerExecution> ExecuteController(
    const NativeAnimationControllerPlan &plan,
    const std::array<std::shared_ptr<const NativeAnimationAsset>,kNativeAnimationSlots> &assets,
    std::span<const uint32_t> names, std::span<const NativeSkeletonJoint> skeleton,
    ControllerLayer previous, uint32_t compression, std::span<const NativeJointName> exclusions,
    bool begin_update=true) {
  if (!previous.Valid() || names.size() != previous.channels.size() || !ValidNativeSkeleton(skeleton) ||
      skeleton.size() != names.size() || exclusions.size() > 30 || plan.count > plan.steps.size()) return {};
  std::array<ControllerLayer,8> layers;
  layers[kNativeAnimationOutput]=std::move(previous);
  if (begin_update) for (auto &header : layers[kNativeAnimationOutput].boundary) header.flags&=~128u;
  std::array<std::string_view,30> excluded_views;
  size_t excluded_count=0;
  for (size_t n=0; n<exclusions.size(); ++n) {
    // Invalid-name sentinel represents an inherited null exclusion pointer;
    // it counts for dispatcher selection, but is not a string to compare.
    if (exclusions[n].Valid()) excluded_views[excluded_count++]=exclusions[n].View();
  }
  ControllerExecution result; result.compression=compression; result.exclusions=uint32_t(exclusions.size());
  for (size_t n=0; n<plan.count; ++n) {
    const auto &step=plan.steps[n];
    if (step.destination >= layers.size() || step.left >= layers.size() || step.right >= layers.size()) return {};
    auto &destination=layers[step.destination];
    if (step.kind == NativeAnimationStep::Kind::Copy) {
      if (!layers[step.left].Valid()) return {};
      destination=layers[step.left]; continue;
    }
    if (step.kind == NativeAnimationStep::Kind::Mix) {
      // Only intermediate layer3+ mixes alias their left input in the source.
      // Final previous/output composition uses a separate copy, not an alias.
      const bool alias_left=step.destination == kNativeAnimationCombined && step.left == step.destination;
      if (!ControllerLayer::Mix(layers[step.left],layers[step.right],step.weight,alias_left,destination)) return {};
      ++result.mixed; continue;
    }
    if (step.slot >= assets.size()) return {};
    if (!step.reset && std::abs(step.weight) < kNativeAnimationWeightEpsilon) continue;
    const auto &asset=assets[step.slot];
    if (!asset || !std::isfinite(step.seconds) || (!step.reset && (step.seconds < 0 || step.seconds > asset->Duration())) ||
        (asset->Indexed() && result.compression)) return {};
    uint32_t root=skeleton.front().pose_index;
    if (step.subtree) {
      const auto found=std::ranges::find_if(skeleton,[&](const auto &joint) {
        return joint.animation_name.Valid() && joint.animation_name.View() == step.root.View();
      });
      if (found == skeleton.end()) return {};
      root=found->pose_index;
    }
    const bool direct=step.reset || (step.weight == 1 && !step.subtree && !result.exclusions);
    const auto excluded=direct || !step.included.View().empty() ? std::span<const std::string_view>{} :
        std::span<const std::string_view>(excluded_views).first(result.exclusions ? excluded_count : 0);
    const NativeAnimationFilter filter{step.included.View(),excluded,direct};
    if (step.reset) destination=ControllerLayer(names.size());
    if (!destination.Apply(*asset,names,skeleton,step.seconds,step.weight,root,step.subtree,filter,step.reset)) return {};
    if (!asset->Indexed()) { result.compression=0; if (!step.reset) result.exclusions=0; }
    ++result.sampled; result.subtree+=step.subtree;
    result.interior+=step.seconds > 0 && step.seconds < asset->Duration();
  }
  result.output=std::move(layers[kNativeAnimationOutput]); return result;
}

class ControllerHandoff {
 public:
  struct Pending {
    uint32_t visual=0, graph=0, source=0;
    uint64_t generation=0;
    std::vector<NativeJointChannels> channels;
    std::vector<ChannelRecord> boundary;
    bool late=false;
  };
  struct Validated { ControllerLayer layer; bool late=false; };
  struct Difference { size_t joint=0, word=0; uint32_t expected=0; std::optional<uint32_t> actual; };
  bool Publish(Pending pending) {
    pending_.reset();
    if (!pending.visual || !pending.graph || !pending.source || !pending.generation || pending.channels.empty() ||
        pending.channels.size() > kMaxNativeJoints || pending.channels.size() != pending.boundary.size() ||
        uint64_t(pending.source)+pending.boundary.size()*48 > uint64_t(UINT32_MAX)+1 || (pending.source&3)) return false;
    pending_=std::move(pending); return true;
  }
  template<class ReadWord>
  std::optional<Validated> TakeLayer(uint32_t visual, uint32_t graph, uint64_t generation,
      uint32_t source, ReadWord &&read, bool &changed, Difference *difference=nullptr) {
    auto pending=std::move(pending_); pending_.reset(); changed=false;
    if (!pending || pending->visual != visual || pending->graph != graph || pending->generation != generation || pending->source != source) return {};
    for (size_t n=0; n<pending->boundary.size(); ++n) for (size_t w=0; w<12; ++w) {
      const auto actual=read(uint64_t(source)+n*48+w*4);
      if (actual != pending->boundary[n][w]) {
        changed=true;
        if (difference) *difference={n,w,pending->boundary[n][w],actual};
        return {};
      }
    }
    Validated result; result.late=pending->late;
    result.layer.channels=std::move(pending->channels);
    for (const auto &record : pending->boundary) result.layer.boundary.push_back({record[0],record[1]});
    return result;
  }
  template<class ReadWord>
  std::optional<std::vector<NativeJointChannels>> Take(uint32_t visual, uint32_t graph, uint64_t generation,
      uint32_t source, ReadWord &&read, bool &changed) {
    auto result=TakeLayer(visual,graph,generation,source,read,changed);
    if (!result) return {};
    return std::move(result->layer.channels);
  }
  void Retire(uint32_t visual) { if (pending_ && pending_->visual == visual) pending_.reset(); }
  void Clear() { pending_.reset(); }
 private:
  std::optional<Pending> pending_;
};
} // namespace bd::gpu::scene::animation_source
