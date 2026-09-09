/**
 * @brief Bounded native instance identities and immutable, lane-specific poses.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_transform.h"
#include "gpu/scene/native_pose_interpolation.h"
#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_material_uv.h"
#include "gpu/scene/native_material_uv_program.h"
#include "gpu/scene/native_material_images.h"
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
  NativeModelRenderHandle model;
  std::vector<RenderMatrix> transforms;
};
struct NativeInstanceStats {
  uint64_t created = 0, retired = 0, published = 0, reused = 0, refused = 0;
  uint64_t render_reads = 0, render_blends = 0, render_reused = 0, render_snaps = 0, render_refused = 0;
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
  static constexpr size_t kEntryBytes = 2048; // includes fixed image-source comparison bindings
  explicit NativeInstanceRegistry(size_t max_bytes = kMaxBytes,
                                  size_t max_instances = kMaxInstances)
      : max_bytes_(max_bytes), max_instances_(max_instances) {}
  ~NativeInstanceRegistry() {
    accounting_->bytes.fetch_sub(entries_.size() * kEntryBytes);
  }

  NativeInstanceId Create(uint64_t model_generation, NativeModelRenderHandle model = {}) {
    std::lock_guard lock(mutex_);
    if (!model_generation || (model && model->Generation() != model_generation) ||
        next_ == UINT64_MAX || entries_.size() >= max_instances_ ||
        !Fits(kEntryBytes)) { ++stats_.refused; return 0; }
    const auto id = next_++;
    entries_.emplace(id, Entry{model_generation, std::move(model), {}});
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
    if (!valid) {
      slot.reset();
      if (lane == 1) it->second.render = {};
      ++stats_.refused; return false;
    }
    if (slot && slot->transforms.size() == transforms.size() &&
        std::memcmp(slot->transforms.data(), transforms.data(), transforms.size_bytes()) == 0) {
      ++stats_.reused;
      return true;
    }
    // Clear stale publication even when backpressure refuses its replacement.
    slot.reset();
    if (lane == 1) it->second.render = {};
    slot = OwnPose(id, it->second, transforms);
    if (!slot) { ++stats_.refused; return false; }
    ++stats_.published;
    return true;
  }

  // Only the completed producer handoff advances temporal history. Repeated
  // publications in one logic tick replace the current endpoint, not previous.
  // Cuts/disabled interpolation/skipped ticks cannot borrow an older endpoint.
  bool ObserveRenderTick(NativeInstanceId id, uint64_t tick, bool active) {
    std::lock_guard lock(mutex_);
    const auto it = entries_.find(id);
    if (it == entries_.end()) return false;
    auto &entry = it->second;
    auto &render = entry.render;
    const auto &current = entry.poses[1];
    if (!current) { render = {}; return false; }
    if (!active || !render.active || tick < render.tick ||
        (tick > render.tick && tick - render.tick != 1)) {
      render = {};
    } else if (tick > render.tick) {
      render.previous = render.current;
    }
    if (render.current != current || render.tick != tick || render.active != active) {
      render.cached.reset(); render.cached_valid = false;
    }
    render.current = current; render.tick = tick; render.active = active;
    return true;
  }

  // Native culling and scene/shadow packets request the SAME immutable pose.
  // The source-comparison reader above remains unblended. Allocation and pinned
  // history/results share this registry's existing aggregate ownership budget.
  std::shared_ptr<const NativeInstancePose> ReadRender(
      const std::shared_ptr<const NativeInstancePose> &completed, NativePosePhase phase) {
    std::lock_guard lock(mutex_);
    ++stats_.render_reads;
    const auto it = completed ? entries_.find(completed->instance) : entries_.end();
    if (it == entries_.end() || it->second.poses[1] != completed ||
        completed->model_generation != it->second.model_generation) {
      ++stats_.render_refused; return {};
    }
    auto &entry = it->second;
    auto &render = entry.render;
    if (phase.interpolate && (!std::isfinite(phase.alpha) || phase.alpha < 0 || phase.alpha > 1)) {
      ++stats_.render_refused; return {};
    }
    if (render.cached_valid && render.current == completed &&
        (phase.frame < render.phase.frame || (phase.frame == render.phase.frame && phase != render.phase))) {
      ++stats_.render_refused; return {};
    }
    if (render.cached_valid && render.phase == phase && render.current == completed) {
      ++stats_.render_reused; return render.cached;
    }
    render.cached.reset(); render.cached_valid = false;
    const bool blend = phase.interpolate && render.active && phase.tick == render.tick &&
        phase.alpha > 0 && phase.alpha < 1 && render.current == completed &&
        render.previous && render.previous != completed &&
        NativePoseCanBlend(render.previous->transforms, completed->transforms);
    if (blend) {
      render.cached = OwnPose(completed->instance, entry, completed->transforms,
                             render.previous->transforms, phase.alpha);
      if (!render.cached) { ++stats_.render_refused; return {}; }
      ++stats_.render_blends;
    } else {
      render.cached = completed; ++stats_.render_snaps;
    }
    render.phase = phase; render.cached_valid = true;
    return render.cached;
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
      if (to == 1) it->second.render = {};
      return false;
    }
    // Immutable handoff costs no matrix copy or additional residency. A later
    // producer publication cannot change the render-side snapshot or its leases.
    destination = source;
    return true;
  }
  void Invalidate(NativeInstanceId id, uint32_t lane) {
    std::lock_guard lock(mutex_);
    if (const auto it = entries_.find(id); it != entries_.end() && lane < 2) {
      it->second.poses[lane].reset();
      if (lane == 1) it->second.render = {};
    }
  }
  void Retire(NativeInstanceId id) {
    std::lock_guard lock(mutex_);
    if (entries_.erase(id)) { accounting_->bytes.fetch_sub(kEntryBytes); ++stats_.retired; }
  }
  // All animated material writers share one immutable, budgeted publication.
  // A late eye patch can preserve other controller slots without a second owner.
  bool PublishMaterialUVs(NativeInstanceId id, uint64_t generation, const NativeMaterialUVs &material) {
    std::lock_guard lock(mutex_);
    const auto it=entries_.find(id);
    if (it == entries_.end() || it->second.model_generation != generation) return false;
    auto &slot=it->second.material_uv;
    if (material.Valid() && slot && slot->Same(material)) return true;
    slot.reset(); // failure cannot expose the preceding writer
    if (!material.Valid() || !Fits(sizeof(MaterialOwner)+128+material.entries.size()*sizeof(NativeMaterialUVs::Entry))) return false;
    auto owner=std::make_shared<MaterialOwner>();
    owner->material=material;
    const size_t bytes=sizeof(MaterialOwner)+128+owner->material.entries.capacity()*sizeof(NativeMaterialUVs::Entry);
    if (!Fits(bytes)) return false;
    owner->bytes=bytes; owner->accounting=accounting_; accounting_->bytes.fetch_add(bytes);
    slot=std::shared_ptr<const NativeMaterialUVs>(owner,&owner->material);
    return true;
  }
  std::shared_ptr<const NativeMaterialUVs> ReadMaterialUVs(NativeInstanceId id, uint64_t generation) const {
    std::lock_guard lock(mutex_);
    const auto it=entries_.find(id);
    return it != entries_.end() && it->second.model_generation == generation ? it->second.material_uv : nullptr;
  }
  void InvalidateMaterialUVs(NativeInstanceId id) {
    std::lock_guard lock(mutex_);
    if (const auto it=entries_.find(id); it != entries_.end()) it->second.material_uv.reset();
  }
  bool PublishMaterialUVProgram(NativeInstanceId id, uint64_t generation, const NativeMaterialUVProgram &program) {
    std::lock_guard lock(mutex_);
    const auto it=entries_.find(id);
    if (it == entries_.end() || it->second.model_generation != generation) return false;
    auto &entry=it->second;
    if (program.Valid() && entry.material_program && entry.material_program->Same(program)) return true;
    entry.material_program.reset(); entry.material_uv.reset(); entry.material_images.reset();
    if (!program.Valid() || !Fits(sizeof(ProgramOwner)+128+program.slots.size()*sizeof(NativeMaterialUVProgram::Slot))) return false;
    auto owner=std::make_shared<ProgramOwner>(); owner->program=program;
    const size_t bytes=sizeof(ProgramOwner)+128+owner->program.slots.capacity()*sizeof(NativeMaterialUVProgram::Slot);
    if (!Fits(bytes)) return false;
    owner->bytes=bytes; owner->accounting=accounting_; accounting_->bytes.fetch_add(bytes);
    entry.material_program=std::shared_ptr<const NativeMaterialUVProgram>(owner,&owner->program);
    return true;
  }
  std::shared_ptr<const NativeMaterialUVProgram> ReadMaterialUVProgram(NativeInstanceId id, uint64_t generation) const {
    std::lock_guard lock(mutex_);
    const auto it=entries_.find(id);
    return it != entries_.end() && it->second.model_generation == generation ? it->second.material_program : nullptr;
  }
  void InvalidateMaterialUVProgram(NativeInstanceId id) {
    std::lock_guard lock(mutex_);
    if (const auto it=entries_.find(id); it != entries_.end()) {
      it->second.material_program.reset(); it->second.material_uv.reset(); it->second.material_images.reset();
    }
  }
  bool PublishMaterialImages(NativeInstanceId id, uint64_t generation, const NativeMaterialImages &images) {
    std::lock_guard lock(mutex_);
    const auto it=entries_.find(id);
    if (it == entries_.end() || it->second.model_generation != generation) return false;
    auto &slot=it->second.material_images;
    if (images.Valid() && slot && slot->Same(images)) return true;
    slot.reset();
    if (!images.Valid() || !Fits(sizeof(ImageOwner)+128+images.entries.size()*sizeof(NativeMaterialImages::Entry))) return false;
    auto owner=std::make_shared<ImageOwner>(); owner->images=images;
    const size_t bytes=sizeof(ImageOwner)+128+owner->images.entries.capacity()*sizeof(NativeMaterialImages::Entry);
    if (!Fits(bytes)) return false;
    owner->bytes=bytes; owner->accounting=accounting_; accounting_->bytes.fetch_add(bytes);
    slot=std::shared_ptr<const NativeMaterialImages>(owner,&owner->images);
    return true;
  }
  std::shared_ptr<const NativeMaterialImages> ReadMaterialImages(NativeInstanceId id, uint64_t generation) const {
    std::lock_guard lock(mutex_);
    const auto it=entries_.find(id);
    return it != entries_.end() && it->second.model_generation == generation ? it->second.material_images : nullptr;
  }
  void InvalidateMaterialImages(NativeInstanceId id) {
    std::lock_guard lock(mutex_);
    if (const auto it=entries_.find(id); it != entries_.end()) it->second.material_images.reset();
  }
  NativeInstanceStats Stats() const {
    std::lock_guard lock(mutex_);
    auto result = stats_;
    result.indexed = entries_.size(); result.bytes = accounting_->bytes.load();
    return result;
  }
private:
  struct Accounting { std::atomic<size_t> bytes{0}; };
  struct ImageOwner {
    NativeMaterialImages images;
    std::shared_ptr<Accounting> accounting;
    size_t bytes=0;
    ~ImageOwner() { if (accounting) accounting->bytes.fetch_sub(bytes); }
  };
  struct ProgramOwner {
    NativeMaterialUVProgram program;
    std::shared_ptr<Accounting> accounting;
    size_t bytes=0;
    ~ProgramOwner() { if (accounting) accounting->bytes.fetch_sub(bytes); }
  };
  struct MaterialOwner {
    NativeMaterialUVs material;
    std::shared_ptr<Accounting> accounting;
    size_t bytes=0;
    ~MaterialOwner() { if (accounting) accounting->bytes.fetch_sub(bytes); }
  };
  struct PoseOwner {
    NativeInstancePose pose;
    std::shared_ptr<Accounting> accounting;
    size_t bytes = 0;
    ~PoseOwner() { if (accounting) accounting->bytes.fetch_sub(bytes); }
  };
  struct Entry {
    uint64_t model_generation;
    NativeModelRenderHandle model;
    std::array<std::shared_ptr<const NativeInstancePose>, 2> poses;
    struct RenderHistory {
      std::shared_ptr<const NativeInstancePose> previous, current, cached;
      uint64_t tick = 0;
      NativePosePhase phase{};
      bool active = false, cached_valid = false;
    } render;
    std::shared_ptr<const NativeMaterialUVs> material_uv;
    std::shared_ptr<const NativeMaterialUVProgram> material_program;
    std::shared_ptr<const NativeMaterialImages> material_images;
  };
  static_assert(sizeof(Entry) <= kEntryBytes);
  std::shared_ptr<const NativeInstancePose> OwnPose(NativeInstanceId id, const Entry &entry,
      std::span<const RenderMatrix> transforms, std::span<const RenderMatrix> previous = {}, float alpha = 0) {
    const size_t bytes = sizeof(PoseOwner) + 128 + transforms.size_bytes();
    if (!Fits(bytes)) return {};
    auto owner = std::make_shared<PoseOwner>();
    owner->pose.instance = id;
    owner->pose.model_generation = entry.model_generation;
    owner->pose.model = entry.model;
    owner->pose.transforms.assign(transforms.begin(), transforms.end());
    if (!previous.empty()) for (size_t n = 0; n < transforms.size(); ++n)
      for (size_t c = 0; c < 16; ++c)
        owner->pose.transforms[n][c] = previous[n][c] + (transforms[n][c] - previous[n][c]) * alpha;
    const size_t retained = sizeof(PoseOwner) + 128 + owner->pose.transforms.capacity() * sizeof(RenderMatrix);
    if (!Fits(retained)) return {};
    owner->bytes = retained;
    accounting_->bytes.fetch_add(retained);
    owner->accounting = accounting_;
    return std::shared_ptr<const NativeInstancePose>(owner, &owner->pose);
  }
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

// Borrowed only while the pose lease lives. No source graph, mesh/buffer key,
// palette address or registry lookup is needed to select the owned primitives.
inline const NativeModelMaterialProgram *FindNativeInstanceNode(
    const NativeInstancePose &pose, uint32_t matrix_index) {
  if (!pose.model || pose.model_generation != pose.model->Generation() ||
      matrix_index >= pose.transforms.size()) return nullptr;
  return pose.model->FindNode(matrix_index);
}
} // namespace bd::gpu::scene
