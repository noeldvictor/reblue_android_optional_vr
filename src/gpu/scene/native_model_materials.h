/**
 * @file    native_model_materials.h
 * @brief   Load-owned immutable primitive recipes and their temporary source index.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once

#include "gpu/scene/native_material_library.h"
#include <atomic>
#include <unordered_set>

namespace bd::gpu::scene {

struct NativeGeometry;

// Asset policy only. Pass availability and per-instance visibility remain live
// inputs; Unknown must not be treated as permission to receive a shadow.
enum class NativeShadowPolicy : uint8_t { Unknown, Receive, Disabled };

// No captured register state or source-memory pointers. The record indices in
// NativeMaterialRange are import recipes, not the finished geometry/texture API.
// Materials already have persistent content identities in NativeMaterialLibrary.
struct NativeModelMaterialProgram {
  std::vector<NativeMaterialRange> ranges;
  std::vector<NativeMaterialHandle> materials;
  // The same primitive ordinal selects its material and owned GPU geometry.
  // Null is explicitly unconverted, never permission to discover it in this core.
  std::vector<std::shared_ptr<const NativeGeometry>> geometries;
  std::vector<NativeShadowPolicy> shadow_policies;
  std::vector<MaterialImageAssignment> texture_assignments;
  std::vector<PrimitivePolicyStep> policy_steps;
  bool valid = false;
};

// Temporary replay-to-primitive lookup only. Resolve these associations once
// during model loading; never re-read source buffer tables at submission.
// Keep source identities outside the native program, and delete this adapter
// when native model/instance handles reach the direct submission path.
struct ModelPrimitiveSourceBinding {
  uint32_t index_buffer = 0, vertex_buffer = 0;
  uint64_t layout = 0;
  uint32_t stride = 0;
};

inline bool ModelPrimitiveMatches(const NativeMaterialRange &range,
                                 const ModelPrimitiveSourceBinding &source,
                                 uint32_t index_buffer, uint32_t vertex_buffer,
                                 uint32_t first_index, uint32_t index_count) {
  return source.index_buffer && source.vertex_buffer &&
      source.index_buffer == index_buffer && source.vertex_buffer == vertex_buffer &&
      range.first_index == first_index && range.index_count == index_count;
}

// Only the temporary loader/consumer index uses source keys. They are never
// persisted or used as native asset identities. Remove this index when native
// model/instance handles reach all of its consumers.
struct ModelMaterialImport {
  uint32_t source_mesh = 0;
  NativeModelMaterialProgram program;
  std::vector<ModelPrimitiveSourceBinding> source_bindings;
};

// Temporary source association selects an owned primitive policy. Conflicting
// policies for identical geometry are ambiguous, not last-writer-wins.
std::optional<bool> FindModelShadowPolicy(
    const ModelMaterialImport &mesh, uint32_t index_buffer, uint32_t vertex_buffer,
    uint32_t first_index, uint32_t index_count);

struct ModelMaterialRegistryStats {
  uint64_t published = 0, retired = 0, refused = 0;
  uint64_t hits = 0, misses = 0;
  size_t indexed = 0, live = 0, bytes = 0;
};

struct ModelMaterialSourceNode {
  uint32_t child = 0, sibling = 0, mesh = 0;
  bool has_geometry = false;
};

// Bounded import traversal, independent of source memory and visibility flags.
// Shared meshes are imported once. Cyclic/aliased nodes are malformed trees.
// Reader returns optional<ModelMaterialSourceNode>; failure leaves out unchanged.
template <typename Reader>
bool CollectModelMaterialSources(uint32_t root, Reader &&read,
                                 std::vector<uint32_t> &out,
                                 size_t max_nodes = 4096) {
  std::vector<uint32_t> pending, sources;
  std::unordered_set<uint32_t> visited, meshes;
  if (root)
    pending.push_back(root);
  while (!pending.empty()) {
    const auto key = pending.back();
    pending.pop_back();
    if (visited.size() >= max_nodes || !visited.insert(key).second)
      return false;
    const auto node = read(key);
    if (!node)
      return false;
    if (node->sibling)
      pending.push_back(node->sibling);
    if (node->child)
      pending.push_back(node->child);
    if (node->has_geometry && node->mesh && meshes.insert(node->mesh).second)
      sources.push_back(node->mesh);
  }
  out = std::move(sources);
  return true;
}

// Immutable load-time publication, never a first-draw cache. Aliasing leases
// pin the entire model across retirement. Retired-but-pinned models still count
// against both limits, including after source address reuse. Thread safe across
// loader and rendering threads; no GPU/runtime/guest-memory dependency.
class ModelMaterialRegistry {
public:
  static constexpr size_t kMaxBytes = 8u << 20;
  static constexpr size_t kMaxMeshes = 4096;
  explicit ModelMaterialRegistry(size_t max_bytes = kMaxBytes,
                                 size_t max_models = 4096);
  bool Publish(uint32_t source_model, std::vector<ModelMaterialImport> meshes);
  void Retire(uint32_t source_model);
  std::shared_ptr<const ModelMaterialImport> Find(
      uint32_t source_model, uint32_t source_mesh);
  ModelMaterialRegistryStats Stats() const;
  uint64_t Generation(uint32_t source_model) const;
  // Logical retained vector storage plus a conservative per-model bookkeeping
  // allowance. Shared material assets and geometry have their own library/GPU
  // arena budgets; retired geometry currently remains in the bounded GPU cache.
  static size_t RetainedBytes(std::span<const ModelMaterialImport> meshes,
                              size_t mesh_capacity);

private:
  struct Accounting {
    std::atomic<size_t> live{0}, bytes{0};
  };
  struct Model {
    uint64_t generation = 0;
    std::vector<ModelMaterialImport> meshes;
    std::shared_ptr<Accounting> accounting;
    size_t bytes = 0;
    ~Model();
  };
  const size_t max_bytes_, max_models_;
  std::shared_ptr<Accounting> accounting_ = std::make_shared<Accounting>();
  mutable std::mutex mutex_;
  std::unordered_map<uint32_t, std::shared_ptr<const Model>> models_;
  ModelMaterialRegistryStats stats_;
};

} // namespace bd::gpu::scene
