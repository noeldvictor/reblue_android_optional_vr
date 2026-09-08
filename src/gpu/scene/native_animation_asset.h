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
// Stable authored name keys are import-time asset associations, not addresses.
// The numeric track ordinal selects the same immutable clip sampler used by
// model-bound clips. Models may load before or after these shared tracks.
class NativeAnimationAsset {
public:
  struct Target { uint32_t name_key, track; };
  static std::optional<NativeAnimationAsset> Create(NativeAnimationClip clip,
      std::vector<uint32_t> names, size_t maximum_bytes) {
    if (names.size() != clip.JointCount() || names.size() != clip.Tracks().size()) return {};
    for (size_t n=0; n<names.size(); ++n) if (clip.Tracks()[n].pose_index != n) return {};
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
    return NativeAnimationAsset(std::move(clip),std::move(targets),bytes);
  }
  size_t RetainedBytes() const { return bytes_; }
  float Duration() const { return clip_.Duration(); }
  const NativeAnimationClip &Clip() const { return clip_; }
  const NativeAnimationTrack *FindTrack(uint32_t name_key) const {
    const auto target = std::lower_bound(targets_.begin(),targets_.end(),name_key,
        [](const auto &a, uint32_t key){return a.name_key < key;});
    if (target == targets_.end() || target->name_key != name_key) return nullptr;
    // Import creates exactly one track per authored descriptor, in ordinal order.
    return &clip_.Tracks()[target->track];
  }
private:
  NativeAnimationAsset(NativeAnimationClip clip, std::vector<Target> targets, size_t bytes)
      : clip_(std::move(clip)), targets_(std::move(targets)), bytes_(bytes) {}
  NativeAnimationClip clip_;
  std::vector<Target> targets_;
  size_t bytes_;
};

// Only this temporary boundary index knows loader/clip keys. There is no lazy
// sample-time import, disk cache or second instance/pose owner. The bridge
// serializes access. Retired but leased assets still debit the aggregate cap.
class NativeAnimationResidency {
public:
  static constexpr size_t kMaxBytes = 8u << 20, kMaxClips = 4096, kEntryBytes = 256;
  explicit NativeAnimationResidency(size_t maximum_bytes = kMaxBytes, size_t maximum_clips = kMaxClips)
      : maximum_bytes_(maximum_bytes), maximum_clips_(maximum_clips) {}
  NativeAnimationResidency(const NativeAnimationResidency &) = delete;
  NativeAnimationResidency &operator=(const NativeAnimationResidency &) = delete;
  size_t Bytes() const { return accounting_->bytes.load(); }
  size_t Size() const { return entries_.size(); }
  size_t AvailableAssetBytes() const {
    if (entries_.size() >= maximum_clips_) return 0;
    const auto bytes = Bytes();
    return bytes >= maximum_bytes_ || maximum_bytes_-bytes < kEntryBytes ? 0 : maximum_bytes_-bytes-kEntryBytes;
  }
  void Retire(uint32_t owner) {
    std::erase_if(entries_,[&](const auto &entry){return entry.second.owner == owner;});
  }
  void Invalidate(uint32_t source) { entries_.erase(source); }
  bool Publish(uint32_t owner, uint32_t source, NativeAnimationAsset asset) {
    Invalidate(source);
    if (!owner || !source || entries_.size() >= maximum_clips_ ||
        asset.RetainedBytes() > AvailableAssetBytes()) return false;
    const size_t bytes = asset.RetainedBytes()+kEntryBytes;
    auto resident = std::make_shared<Resident>(std::move(asset),accounting_,bytes);
    accounting_->bytes.fetch_add(bytes);
    resident->charged = true;
    entries_.emplace(source,Entry{owner,std::move(resident)});
    return true;
  }
  std::shared_ptr<const NativeAnimationAsset> Find(uint32_t source) const {
    const auto entry = entries_.find(source);
    return entry == entries_.end() ? nullptr :
        std::shared_ptr<const NativeAnimationAsset>(entry->second.resident,&entry->second.resident->asset);
  }
private:
  struct Accounting { std::atomic<size_t> bytes{0}; };
  struct Resident {
    NativeAnimationAsset asset;
    std::shared_ptr<Accounting> accounting;
    size_t bytes;
    bool charged = false;
    Resident(NativeAnimationAsset value, std::shared_ptr<Accounting> budget, size_t count)
        : asset(std::move(value)), accounting(std::move(budget)), bytes(count) {}
    ~Resident() { if (charged) accounting->bytes.fetch_sub(bytes); }
  };
  struct Entry { uint32_t owner; std::shared_ptr<const Resident> resident; };
  size_t maximum_bytes_, maximum_clips_;
  std::shared_ptr<Accounting> accounting_ = std::make_shared<Accounting>();
  std::unordered_map<uint32_t,Entry> entries_;
};
} // namespace bd::gpu::scene
