/**
 * @file    native_model_materials.cpp
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_model_materials.h"
#include <algorithm>
#include <limits>

namespace bd::gpu::scene {

ModelMaterialRegistry::ModelMaterialRegistry(size_t max_bytes, size_t max_models)
    : max_bytes_(max_bytes), max_models_(max_models) {}

ModelMaterialRegistry::Model::~Model() {
  if (accounting) {
    accounting->bytes.fetch_sub(bytes);
    accounting->live.fetch_sub(1);
  }
}

size_t ModelMaterialRegistry::RetainedBytes(
    std::span<const ModelMaterialImport> meshes, size_t mesh_capacity) {
  constexpr size_t limit = std::numeric_limits<size_t>::max();
  size_t bytes = sizeof(Model) + 256;
  auto add = [&](size_t count, size_t stride) {
    if (count > (limit - bytes) / stride) {
      bytes = limit;
      return;
    }
    bytes += count * stride;
  };
  add(mesh_capacity, sizeof(ModelMaterialImport));
  for (const auto &mesh : meshes) {
    add(mesh.program.ranges.capacity(), sizeof(NativeMaterialRange));
    add(mesh.program.materials.capacity(), sizeof(NativeMaterialHandle));
  }
  return bytes;
}

bool ModelMaterialRegistry::Publish(uint32_t source_model,
                                     std::vector<ModelMaterialImport> meshes) {
  std::lock_guard lock(mutex_);
  // A failed new load must not leave a previous allocation's recipes visible.
  // Existing leases remain valid, but cannot be found through a reused key.
  if (models_.erase(source_model))
    ++stats_.retired;
  const size_t bytes = RetainedBytes(meshes, meshes.capacity());
  if (!source_model || meshes.size() > kMaxMeshes || bytes > max_bytes_ ||
      accounting_->bytes.load() > max_bytes_ - bytes ||
      accounting_->live.load() >= max_models_) {
    ++stats_.refused;
    return false;
  }
  std::sort(meshes.begin(), meshes.end(), [](const auto &a, const auto &b) {
    return a.source_mesh < b.source_mesh;
  });
  uint32_t previous = 0;
  for (const auto &mesh : meshes) {
    if (!mesh.source_mesh || mesh.source_mesh == previous ||
        mesh.program.ranges.size() != mesh.program.materials.size() ||
        (!mesh.program.valid && !mesh.program.ranges.empty())) {
      ++stats_.refused;
      return false;
    }
    previous = mesh.source_mesh;
  }
  auto model = std::make_shared<Model>();
  model->meshes = std::move(meshes);
  model->bytes = bytes;
  accounting_->bytes.fetch_add(bytes);
  accounting_->live.fetch_add(1);
  model->accounting = accounting_;
  models_.emplace(source_model, std::move(model));
  ++stats_.published;
  return true;
}

void ModelMaterialRegistry::Retire(uint32_t source_model) {
  std::lock_guard lock(mutex_);
  if (models_.erase(source_model))
    ++stats_.retired;
}

std::shared_ptr<const NativeModelMaterialProgram> ModelMaterialRegistry::Find(
    uint32_t source_model, uint32_t source_mesh) {
  std::lock_guard lock(mutex_);
  const auto it = models_.find(source_model);
  if (it != models_.end()) {
    const auto &meshes = it->second->meshes;
    const auto mesh = std::lower_bound(meshes.begin(), meshes.end(), source_mesh,
        [](const auto &entry, uint32_t key) { return entry.source_mesh < key; });
    if (mesh != meshes.end() && mesh->source_mesh == source_mesh &&
        mesh->program.valid) {
      ++stats_.hits;
      return {it->second, &mesh->program};
    }
  }
  ++stats_.misses;
  return {};
}

ModelMaterialRegistryStats ModelMaterialRegistry::Stats() const {
  std::lock_guard lock(mutex_);
  auto stats = stats_;
  stats.indexed = models_.size();
  stats.live = accounting_->live.load();
  stats.bytes = accounting_->bytes.load();
  return stats;
}

} // namespace bd::gpu::scene
