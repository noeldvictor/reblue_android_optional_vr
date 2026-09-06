/**
 * @brief Bounded native instance identities and immutable, lane-specific poses.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_transform.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>

namespace bd::gpu::scene {
using NativeInstanceId = uint64_t;
struct NativeInstancePose {
  NativeInstanceId instance = 0;
  uint64_t model_generation = 0;
  std::vector<RenderMatrix> transforms;
};
struct NativeInstanceStats {
  uint64_t created = 0, retired = 0, published = 0, reused = 0, refused = 0;
  size_t indexed = 0, bytes = 0;
};

// No source addresses, renderer state, resource wrappers or disk IO. Callers
// supply native model generations and native matrices at the update boundary.
// Readers pin immutable poses; retirement/replacement cannot repoint a lease.
class NativeInstanceRegistry {
public:
  static constexpr size_t kMaxTransforms = 4096, kMaxInstances = 4096;
  static constexpr size_t kMaxBytes = 16u << 20;
  // Includes conservative bookkeeping for the bridge's separately bounded index.
  static constexpr size_t kEntryBytes = 512;
  explicit NativeInstanceRegistry(size_t max_bytes = kMaxBytes,
                                  size_t max_instances = kMaxInstances)
      : max_bytes_(max_bytes), max_instances_(max_instances) {}
  ~NativeInstanceRegistry() {
    accounting_->bytes.fetch_sub(entries_.size() * kEntryBytes);
  }

  NativeInstanceId Create(uint64_t model_generation) {
    std::lock_guard lock(mutex_);
    if (!model_generation || next_ == UINT64_MAX || entries_.size() >= max_instances_ ||
        !Fits(kEntryBytes)) { ++stats_.refused; return 0; }
    const auto id = next_++;
    entries_.emplace(id, Entry{model_generation, {}});
    accounting_->bytes.fetch_add(kEntryBytes);
    ++stats_.created;
    return id;
  }

  bool Publish(NativeInstanceId id, uint32_t lane, std::span<const RenderMatrix> transforms) {
    std::lock_guard lock(mutex_);
    const auto it = entries_.find(id);
    if (it == entries_.end() || lane >= 2) { ++stats_.refused; return false; }
    auto &slot = it->second.poses[lane];
    bool valid = !transforms.empty() && transforms.size() <= kMaxTransforms;
    if (valid) for (const auto &matrix : transforms)
      for (float value : matrix) valid &= std::isfinite(value);
    if (!valid) { slot.reset(); ++stats_.refused; return false; }
    if (slot && slot->transforms.size() == transforms.size() &&
        std::memcmp(slot->transforms.data(), transforms.data(), transforms.size_bytes()) == 0) {
      ++stats_.reused;
      return true;
    }
    // Clear stale publication even when backpressure refuses its replacement.
    slot.reset();
    const size_t bytes = sizeof(PoseOwner) + 128 + transforms.size_bytes();
    if (!Fits(bytes)) { ++stats_.refused; return false; }
    auto owner = std::make_shared<PoseOwner>();
    owner->pose.instance = id;
    owner->pose.model_generation = it->second.model_generation;
    owner->pose.transforms.assign(transforms.begin(), transforms.end());
    const size_t retained = sizeof(PoseOwner) + 128 +
        owner->pose.transforms.capacity() * sizeof(RenderMatrix);
    if (!Fits(retained)) { ++stats_.refused; return false; }
    owner->bytes = retained;
    accounting_->bytes.fetch_add(retained);
    owner->accounting = accounting_;
    slot = std::shared_ptr<const NativeInstancePose>(owner, &owner->pose);
    ++stats_.published;
    return true;
  }

  std::shared_ptr<const NativeInstancePose> Read(NativeInstanceId id, uint32_t lane) const {
    std::lock_guard lock(mutex_);
    const auto it = entries_.find(id);
    return it != entries_.end() && lane < 2 ? it->second.poses[lane] : nullptr;
  }
  bool Transfer(NativeInstanceId id, uint32_t from, uint32_t to, size_t count) {
    std::lock_guard lock(mutex_);
    const auto it = entries_.find(id);
    if (it == entries_.end() || from >= 2 || to >= 2) return false;
    const auto &source = it->second.poses[from];
    auto &destination = it->second.poses[to];
    if (!source || source->transforms.size() != count) {
      destination.reset();
      return false;
    }
    // Immutable handoff costs no matrix copy or additional residency. A later
    // producer publication cannot change the render-side snapshot or its leases.
    destination = source;
    return true;
  }
  void Invalidate(NativeInstanceId id, uint32_t lane) {
    std::lock_guard lock(mutex_);
    if (const auto it = entries_.find(id); it != entries_.end() && lane < 2)
      it->second.poses[lane].reset();
  }
  void Retire(NativeInstanceId id) {
    std::lock_guard lock(mutex_);
    if (entries_.erase(id)) { accounting_->bytes.fetch_sub(kEntryBytes); ++stats_.retired; }
  }
  NativeInstanceStats Stats() const {
    std::lock_guard lock(mutex_);
    auto result = stats_;
    result.indexed = entries_.size(); result.bytes = accounting_->bytes.load();
    return result;
  }
private:
  struct Accounting { std::atomic<size_t> bytes{0}; };
  struct PoseOwner {
    NativeInstancePose pose;
    std::shared_ptr<Accounting> accounting;
    size_t bytes = 0;
    ~PoseOwner() { if (accounting) accounting->bytes.fetch_sub(bytes); }
  };
  struct Entry {
    uint64_t model_generation;
    std::array<std::shared_ptr<const NativeInstancePose>, 2> poses;
  };
  bool Fits(size_t bytes) const {
    return bytes <= max_bytes_ && accounting_->bytes.load() <= max_bytes_ - bytes;
  }
  const size_t max_bytes_, max_instances_;
  NativeInstanceId next_ = 1;
  std::shared_ptr<Accounting> accounting_ = std::make_shared<Accounting>();
  mutable std::mutex mutex_;
  std::unordered_map<NativeInstanceId, Entry> entries_;
  NativeInstanceStats stats_;
};
} // namespace bd::gpu::scene
