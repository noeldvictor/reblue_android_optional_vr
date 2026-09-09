/**
 * @brief Load-owned named animation tracks, shared across model instances.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_animation_clip.h"
#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace bd::gpu::scene {
namespace material_image_source { struct LoadedAnimation; }
enum class NativeAnimationBinding { Name, Joint };
// Stable authored name keys are import-time asset associations, not addresses.
// The numeric track ordinal selects the same immutable clip sampler used by
// model-bound clips. Models may load before or after these shared tracks.
class NativeAnimationAsset {
public:
  struct Target { uint32_t name_key, track; };
  static std::optional<NativeAnimationAsset> Create(NativeAnimationClip clip,
      std::vector<uint32_t> names, size_t maximum_bytes,
      NativeAnimationBinding binding = NativeAnimationBinding::Name, uint32_t channel_mask = 7) {
    if (clip.JointCount() != clip.Tracks().size() ||
        (binding == NativeAnimationBinding::Name ? names.size() != clip.JointCount() || channel_mask != 7 :
         binding != NativeAnimationBinding::Joint || !names.empty() || (channel_mask != 3 && channel_mask != 7))) return {};
    for (size_t n=0; n<clip.Tracks().size(); ++n) {
      const auto &track=clip.Tracks()[n];
      if (track.pose_index != n || (!(channel_mask&4) &&
          (!track.scale.empty() || (track.splines && track.splines->scale.active)))) return {};
    }
    std::vector<Target> targets;
    if (maximum_bytes < sizeof(NativeAnimationAsset) + clip.RetainedBytes() ||
        names.size() > (maximum_bytes-sizeof(NativeAnimationAsset)-clip.RetainedBytes())/sizeof(Target)) return {};
    targets.reserve(names.size());
    for (uint32_t n=0; n<names.size(); ++n) targets.push_back({names[n],n});
    std::sort(targets.begin(),targets.end(),[](auto a, auto b){return a.name_key < b.name_key;});
    for (size_t n=1; n<targets.size(); ++n)
      if (targets[n-1].name_key == targets[n].name_key) return {};
    const auto bytes = sizeof(NativeAnimationAsset)+clip.RetainedBytes()+targets.capacity()*sizeof(Target);
    if (bytes > maximum_bytes) return {};
    return NativeAnimationAsset(std::move(clip),std::move(targets),bytes,binding,channel_mask);
  }
  size_t RetainedBytes() const { return bytes_; }
  float Duration() const { return clip_.Duration(); }
  const NativeAnimationClip &Clip() const { return clip_; }
  bool Indexed() const { return binding_ == NativeAnimationBinding::Joint; }
  uint32_t ChannelMask() const { return channel_mask_; }
  const NativeAnimationTrack *FindTrack(uint32_t name_key, uint32_t joint = UINT32_MAX) const {
    if (Indexed()) return joint < clip_.Tracks().size() ? &clip_.Tracks()[joint] : nullptr;
    const auto target = std::lower_bound(targets_.begin(),targets_.end(),name_key,
        [](const auto &a, uint32_t key){return a.name_key < key;});
    if (target == targets_.end() || target->name_key != name_key) return nullptr;
    // Import creates one canonical first-match track per authored name.
    return &clip_.Tracks()[target->track];
  }
  bool SampleTarget(uint32_t name_key, uint32_t joint, float seconds, NativeJointChannels &out) const {
    const auto *track=FindTrack(name_key,joint);
    return track && clip_.SampleTrack(size_t(track-clip_.Tracks().data()),seconds,out);
  }
private:
  NativeAnimationAsset(NativeAnimationClip clip, std::vector<Target> targets, size_t bytes,
      NativeAnimationBinding binding, uint32_t channel_mask)
      : clip_(std::move(clip)), targets_(std::move(targets)), bytes_(bytes), binding_(binding), channel_mask_(channel_mask) {}
  NativeAnimationClip clip_;
  std::vector<Target> targets_;
  size_t bytes_;
  NativeAnimationBinding binding_;
  uint32_t channel_mask_;
};

// Only this temporary boundary index knows loader/clip keys. Completed loads
// register backing lifetime; selected slots prepare immutable assets BEFORE
// sampling. Dormant pack entries do not consume the working set's curve budget.
// The bridge serializes access; retired but leased assets remain charged.
class NativeAnimationResidency {
public:
  static constexpr size_t kMaxBytes = 8u << 20, kMaxClips = 4096, kEntryBytes = 256;
  explicit NativeAnimationResidency(size_t maximum_bytes = kMaxBytes, size_t maximum_clips = kMaxClips)
      : maximum_bytes_(maximum_bytes), maximum_clips_(maximum_clips) {}
  NativeAnimationResidency(const NativeAnimationResidency &) = delete;
  NativeAnimationResidency &operator=(const NativeAnimationResidency &) = delete;
  size_t Bytes() const { return accounting_->bytes.load(); }
  size_t Size() const { return entries_.size(); }
  size_t ResidentCount() const {
    return std::ranges::count_if(entries_,[](const auto &entry){return bool(entry.second.resident);});
  }
  size_t AvailableAssetBytes() const {
    if (entries_.size() >= maximum_clips_) return 0;
    const auto bytes = Bytes();
    return bytes >= maximum_bytes_ || maximum_bytes_-bytes < kEntryBytes ? 0 : maximum_bytes_-bytes-kEntryBytes;
  }
  void Retire(uint32_t owner) {
    std::erase_if(entries_,[&](const auto &entry){return entry.second.owner == owner;});
  }
  template<class Asset = NativeAnimationAsset>
  void Invalidate(uint32_t source) { entries_.erase(Key<Asset>(source)); }
  template<class Asset = NativeAnimationAsset>
  bool Register(uint32_t owner, uint32_t source) {
    Invalidate<Asset>(source);
    if (!owner || !source || entries_.size() >= maximum_clips_ || RemainingBytes() < kEntryBytes) return false;
    entries_.emplace(Key<Asset>(source),Entry{owner,std::make_shared<Charge>(accounting_,kEntryBytes)});
    return true;
  }
  template<class Asset = NativeAnimationAsset>
  bool Publish(uint32_t owner, uint32_t source, Asset asset) {
    Invalidate<Asset>(source);
    if (!owner || !source || entries_.size() >= maximum_clips_ ||
        asset.RetainedBytes() > AvailableAssetBytes()) return false;
    if (!Register<Asset>(owner,source)) return false;
    auto &entry = entries_.at(Key<Asset>(source));
    entry.resident = std::make_shared<Resident>(std::move(asset),accounting_,entry.charge);
    return true;
  }
  template <class Asset = NativeAnimationAsset, class Import>
  bool Prepare(uint32_t source, uint32_t frame, Import &&import) {
    const auto found = entries_.find(Key<Asset>(source));
    if (found == entries_.end()) return false; // never resurrect retired/in-flight backing
    auto &entry = found->second;
    entry.last_frame = frame;
    if (entry.resident) return true; // no packed-key reads on steady-state slot updates
    auto attempt = [&] {
      const auto budget = RemainingBytes();
      // A failed generation is retried only when its available budget improves.
      // Malformed/oversized assets cannot cause a per-tick decode/allocation loop.
      if (!budget || (entry.attempted && budget <= entry.failed_budget)) return false;
      entry.attempted = true; entry.failed_budget = budget;
      auto asset = import(source,budget);
      if (!asset || asset->RetainedBytes() > budget) return false;
      entry.resident = std::make_shared<Resident>(std::move(*asset),accounting_,entry.charge);
      entry.attempted = false;
      return true;
    };
    if (attempt()) return true;
    // Keep this and the two preceding frames' selections, plus every lease.
    // Evict only dormant payloads; their loader registration remains bounded.
    for (auto &[key,candidate] : entries_) {
      if (key != Key<Asset>(source) && candidate.resident && candidate.resident.use_count() == 1 &&
          uint32_t(frame-candidate.last_frame) > 2) candidate.resident.reset();
    }
    return attempt();
  }
  template<class Asset = NativeAnimationAsset>
  std::shared_ptr<const Asset> Find(uint32_t source) const {
    const auto entry = entries_.find(Key<Asset>(source));
    return entry == entries_.end() || !entry->second.resident ? nullptr :
        std::shared_ptr<const Asset>(entry->second.resident,static_cast<const Asset *>(entry->second.resident->asset.get()));
  }
private:
  // One existing index and budget; disjoint typed keys prevent a motion source
  // and an image-loader object at the same numeric address from aliasing.
  template<class Asset> static uint64_t Key(uint32_t source) {
    static_assert(std::is_same_v<Asset,NativeAnimationAsset> || std::is_same_v<Asset,material_image_source::LoadedAnimation>);
    return uint64_t(!std::is_same_v<Asset,NativeAnimationAsset>)<<32 | source;
  }
  struct Accounting { std::atomic<size_t> bytes{0}; };
  struct Charge {
    std::shared_ptr<Accounting> accounting;
    size_t bytes;
    Charge(std::shared_ptr<Accounting> budget, size_t count) : accounting(std::move(budget)), bytes(count) {
      accounting->bytes.fetch_add(bytes);
    }
    ~Charge() { accounting->bytes.fetch_sub(bytes); }
    Charge(const Charge &) = delete;
    Charge &operator=(const Charge &) = delete;
  };
  struct Resident {
    std::shared_ptr<const void> asset;
    std::shared_ptr<Charge> entry_charge;
    Charge payload_charge;
    template<class Asset>
    Resident(Asset value, std::shared_ptr<Accounting> budget, std::shared_ptr<Charge> entry)
        : asset(std::make_shared<const Asset>(std::move(value))), entry_charge(std::move(entry)),
          payload_charge(std::move(budget),static_cast<const Asset *>(asset.get())->RetainedBytes()) {}
  };
  struct Entry {
    uint32_t owner;
    std::shared_ptr<Charge> charge;
    std::shared_ptr<const Resident> resident;
    uint32_t last_frame = 0;
    size_t failed_budget = 0;
    bool attempted = false;
  };
  size_t RemainingBytes() const { const auto bytes=Bytes(); return bytes >= maximum_bytes_ ? 0 : maximum_bytes_-bytes; }
  size_t maximum_bytes_, maximum_clips_;
  std::shared_ptr<Accounting> accounting_ = std::make_shared<Accounting>();
  std::unordered_map<uint64_t,Entry> entries_;
};
} // namespace bd::gpu::scene
