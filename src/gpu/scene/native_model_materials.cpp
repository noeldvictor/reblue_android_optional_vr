/**
 * @file    native_model_materials.cpp
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_model_materials.h"
#include <algorithm>
#include <limits>

namespace bd::gpu::scene {

const NativeModelMaterialProgram *NativeModelRenderData::FindNode(uint32_t matrix_index) const {
  const auto found = std::lower_bound(nodes_.begin(), nodes_.end(), matrix_index,
      [](const Node &node, uint32_t index) { return node.matrix_index < index; });
  if (found == nodes_.end() || found->matrix_index != matrix_index ||
      (found + 1 != nodes_.end() && (found + 1)->matrix_index == matrix_index)) return nullptr;
  return found->program;
}

std::optional<bool> FindModelShadowPolicy(
    const ModelMaterialImport &mesh, uint32_t index_buffer, uint32_t vertex_buffer,
    uint32_t first_index, uint32_t index_count) {
  const auto &program = mesh.program;
  if (!program.valid || program.ranges.size() != program.shadow_policies.size() ||
      program.ranges.size() != mesh.source_bindings.size())
    return {};
  std::optional<bool> found;
  for (size_t i = 0; i < program.ranges.size(); ++i) {
    if (!ModelPrimitiveMatches(program.ranges[i], mesh.source_bindings[i],
                               index_buffer, vertex_buffer, first_index, index_count))
      continue;
    const auto policy = program.shadow_policies[i];
    if (policy != NativeShadowPolicy::Receive && policy != NativeShadowPolicy::Disabled)
      return {};
    const bool disabled = policy == NativeShadowPolicy::Disabled;
    if (found && *found != disabled)
      return {};
    found = disabled;
  }
  return found;
}

ModelMaterialRegistry::ModelMaterialRegistry(size_t max_bytes, size_t max_models)
    : max_bytes_(max_bytes), max_models_(max_models) {}

ModelMaterialRegistry::Model::~Model() {
  if (accounting) {
    accounting->bytes.fetch_sub(bytes);
    accounting->live.fetch_sub(1);
  }
}

size_t ModelMaterialRegistry::RetainedBytes(
    std::span<const ModelMaterialImport> meshes, size_t mesh_capacity, size_t node_capacity, size_t joint_capacity,
    size_t target_capacity) {
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
  add(node_capacity, sizeof(NativeModelRenderData::Node));
  add(joint_capacity, sizeof(NativeSkeletonJoint));
  add(target_capacity, sizeof(uint32_t));
  for (const auto &mesh : meshes) {
    add(mesh.program.ranges.capacity(), sizeof(NativeMaterialRange));
    add(mesh.program.materials.capacity(), sizeof(NativeMaterialHandle));
    add(mesh.program.geometries.capacity(), sizeof(std::shared_ptr<const NativeGeometry>));
    add(mesh.program.skin_geometries.capacity(), sizeof(std::shared_ptr<const NativeGeometry>));
    add(mesh.program.shadow_policies.capacity(), sizeof(NativeShadowPolicy));
    add(mesh.program.texture_assignments.capacity(), sizeof(MaterialImageAssignment));
    add(mesh.program.policy_steps.capacity(), sizeof(PrimitivePolicyStep));
    add(mesh.source_bindings.capacity(), sizeof(ModelPrimitiveSourceBinding));
  }
  return bytes;
}

bool ModelMaterialRegistry::Publish(uint32_t source_model,
                                     std::vector<ModelMaterialImport> meshes,
                                     std::span<const ModelNodeSourceBinding> nodes,
                                     std::vector<NativeSkeletonJoint> skeleton,
                                     std::vector<uint32_t> animation_targets) {
  std::lock_guard lock(mutex_);
  // A failed new load must not leave a previous allocation's recipes visible.
  // Existing leases remain valid, but cannot be found through a reused key.
  if (models_.erase(source_model))
    ++stats_.retired;
  size_t bytes = RetainedBytes(meshes, meshes.capacity(), nodes.size(), skeleton.capacity(), animation_targets.capacity());
  if (!source_model || stats_.published == UINT64_MAX || meshes.size() > kMaxMeshes || nodes.size() > kMaxMeshes || bytes > max_bytes_ ||
      accounting_->bytes.load() > max_bytes_ - bytes ||
      accounting_->live.load() >= max_models_ || (!skeleton.empty() && !ValidNativeSkeleton(skeleton)) ||
      (!animation_targets.empty() && animation_targets.size() != skeleton.size())) {
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
        mesh.program.ranges.size() != mesh.program.geometries.size() ||
        (!mesh.program.skin_geometries.empty() && mesh.program.ranges.size() != mesh.program.skin_geometries.size()) ||
        mesh.program.ranges.size() != mesh.program.shadow_policies.size() ||
        mesh.program.ranges.size() != mesh.source_bindings.size() ||
        (!mesh.program.valid && !mesh.program.ranges.empty())) {
      ++stats_.refused;
      return false;
    }
    previous = mesh.source_mesh;
    uint32_t end = 0, policy_end = 0;
    for (const auto &range : mesh.program.ranges) {
      if (range.texture_assignment_end < end ||
          range.texture_assignment_end > mesh.program.texture_assignments.size() ||
          range.policy_step_end < policy_end || range.policy_step_end > mesh.program.policy_steps.size()) {
        ++stats_.refused;
        return false;
      }
      end = range.texture_assignment_end;
      policy_end = range.policy_step_end;
    }
  }
  auto model = std::make_shared<Model>();
  model->generation = stats_.published + 1;
  model->meshes = std::move(meshes);
  model->render.generation_ = model->generation;
  model->render.skeleton_ = std::move(skeleton);
  model->render.animation_targets_ = std::move(animation_targets);
  model->render.nodes_.reserve(nodes.size());
  for (const auto &node : nodes) {
    const auto found = std::lower_bound(model->meshes.begin(), model->meshes.end(), node.source_mesh,
        [](const auto &mesh, uint32_t key) { return mesh.source_mesh < key; });
    if (node.matrix_index >= kMaxMeshes || found == model->meshes.end() || found->source_mesh != node.source_mesh) {
      ++stats_.refused; return false;
    }
    model->render.nodes_.push_back({node.matrix_index, found->program.valid ? &found->program : nullptr});
  }
  std::sort(model->render.nodes_.begin(), model->render.nodes_.end(),
      [](const auto &a, const auto &b) { return a.matrix_index < b.matrix_index; });
  bytes = RetainedBytes(model->meshes, model->meshes.capacity(), model->render.nodes_.capacity(),
                        model->render.skeleton_.capacity(),model->render.animation_targets_.capacity());
  if (bytes > max_bytes_ || accounting_->bytes.load() > max_bytes_ - bytes) { ++stats_.refused; return false; }
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

std::shared_ptr<const ModelMaterialImport> ModelMaterialRegistry::Find(
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
      return {it->second, &*mesh};
    }
  }
  ++stats_.misses;
  return {};
}

std::shared_ptr<const ModelMaterialImport> ModelMaterialRegistry::FindNodeImport(
    uint32_t source_model, uint32_t node) {
  std::lock_guard lock(mutex_);
  const auto found = models_.find(source_model);
  if (found == models_.end()) return {};
  const auto *program = found->second->render.FindNode(node);
  if (!program) return {};
  for (const auto &mesh : found->second->meshes)
    if (&mesh.program == program) return {found->second, &mesh};
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

uint64_t ModelMaterialRegistry::Generation(uint32_t source_model) const {
  std::lock_guard lock(mutex_);
  const auto it = models_.find(source_model);
  return it == models_.end() ? 0 : it->second->generation;
}

NativeModelRenderHandle ModelMaterialRegistry::FindModel(uint32_t source_model) const {
  std::lock_guard lock(mutex_);
  const auto found = models_.find(source_model);
  return found == models_.end() ? NativeModelRenderHandle{} :
      NativeModelRenderHandle(found->second, &found->second->render);
}

} // namespace bd::gpu::scene
