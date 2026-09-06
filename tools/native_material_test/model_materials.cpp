#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_model_geometry_source.h"
#include <barrier>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

using namespace bd::gpu::scene;
namespace {
void Require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
ModelMaterialImport Mesh(uint32_t key, uint8_t power = 12) {
  ModelMaterialImport mesh;
  mesh.source_mesh = key;
  auto &program = mesh.program;
  const uint16_t commands[]{uint16_t(0x0400 | power), 0x0200,
      0x4000, 0, 0x5000, 0x1000, 1, 0, 0xff};
  program.valid = DecodeMeshMaterials(commands, program.ranges);
  Require(program.valid && program.ranges.size() == 1, "fixture decode");
  // Native immutable assets only; this test does not touch disk or the game.
  NativeMaterialAsset asset{program.ranges[0].material};
  std::vector<uint8_t> encoded;
  Require(EncodeNativeMaterial(asset, encoded), "fixture material encoding");
  program.materials.push_back(std::make_shared<const NativeMaterial>(
      NativeMaterial{NativeMaterialContentId(encoded), asset}));
  program.geometries.resize(1);
  mesh.source_bindings.push_back({100, 200, 300, 32});
  return mesh;
}
}

void TestNativeModelMaterials() {
  ModelMaterialRegistry registry;
  Require(!registry.Find(1, 10) && registry.Stats().indexed == 0,
          "lookup must never discover or create a model");
  Require(registry.Publish(1, {Mesh(20), Mesh(10)}), "preload publication");
  const auto first_generation = registry.Generation(1);
  Require(first_generation && !registry.Generation(999), "only published native model generations exist");
  auto first = registry.Find(1, 10);
  auto sibling = registry.Find(1, 20);
  Require(first && sibling && first->program.materials[0]->id == sibling->program.materials[0]->id,
          "native identities do not depend on source keys");
  Require(first->program.ranges[0].skin && first->program.ranges[0].skin->count == 0,
          "explicit unskinned recipe survives publication");
  const auto old_id = first->program.materials[0]->id;
  const auto old_bytes = registry.Stats().bytes;
  registry.Retire(1);
  Require(!registry.Find(1, 10) && registry.Stats().indexed == 0 &&
          registry.Stats().live == 1 && registry.Stats().bytes == old_bytes,
          "retired leases remain owned and charged");
  Require(registry.Publish(1, {Mesh(10, 24)}), "source address reuse");
  Require(registry.Generation(1) > first_generation, "source reuse gets a fresh native generation");
  auto replacement = registry.Find(1, 10);
  Require(replacement && replacement->program.materials[0]->id != old_id &&
          first->program.materials[0]->id == old_id && registry.Stats().live == 2,
          "old and new generations must not alias");
  first.reset();
  Require(registry.Stats().live == 2, "second mesh lease pins whole model");
  sibling.reset();
  Require(registry.Stats().live == 1, "last old lease releases accounting");
  registry.Retire(1);
  replacement.reset();
  Require(registry.Stats().bytes == 0 && registry.Stats().live == 0,
          "full retirement releases recipes");

  Require(registry.Publish(3, {Mesh(10)}), "valid initial publication");
  Require(!registry.Publish(3, {Mesh(10), Mesh(10)}) && !registry.Find(3, 10),
          "failed reused source cannot expose stale generation");
  Require(!registry.Publish(0, {}) && !registry.Publish(4, {Mesh(0)}),
          "null source identities refused");
  auto malformed = Mesh(10);
  malformed.program.materials.clear();
  Require(!registry.Publish(4, {malformed}), "range/material count mismatch");
  malformed = Mesh(10);
  malformed.program.geometries.clear();
  Require(!registry.Publish(4, {malformed}), "range/geometry count mismatch");
  malformed = Mesh(10);
  malformed.source_bindings.clear();
  Require(!registry.Publish(4, {malformed}), "range/source association count mismatch");
  malformed = Mesh(10);
  malformed.program.valid = false;
  Require(!registry.Publish(4, {malformed}), "invalid program cannot carry ranges");
  ModelMaterialImport unsupported;
  unsupported.source_mesh = 11;
  Require(registry.Publish(4, {Mesh(10), unsupported}) &&
          !registry.Find(4, 11) && registry.Find(4, 10),
          "unsupported mesh does not erase supported siblings");
  registry.Retire(4);

  std::vector<ModelMaterialImport> one{Mesh(10)};
  const auto bytes = ModelMaterialRegistry::RetainedBytes(one, one.capacity());
  ModelMaterialRegistry tight(bytes, 1);
  Require(tight.Publish(5, std::move(one)), "exact budget allowed");
  auto pinned = tight.Find(5, 10);
  tight.Retire(5);
  Require(!tight.Publish(6, {Mesh(10)}) && tight.Stats().bytes == bytes,
          "retirement does not bypass pinned budget");
  pinned.reset();
  Require(tight.Publish(6, {Mesh(10)}), "freed capacity reusable");
  tight.Retire(6);
  ModelMaterialRegistry short_budget(bytes - 1);
  Require(!short_budget.Publish(7, {Mesh(10)}), "byte ceiling enforced");
  Require(ModelMaterialRegistry::RetainedBytes({}, std::numeric_limits<size_t>::max()) ==
              std::numeric_limits<size_t>::max(), "accounting overflow saturates");
  std::vector<ModelMaterialImport> too_many(ModelMaterialRegistry::kMaxMeshes + 1);
  Require(!registry.Publish(8, std::move(too_many)), "mesh count bound");

  std::shared_ptr<const ModelMaterialImport> surviving;
  {
    ModelMaterialRegistry temporary;
    Require(temporary.Publish(9, {Mesh(10)}), "temporary owner");
    surviving = temporary.Find(9, 10);
  }
  Require(surviving && surviving->program.materials[0]->id == old_id,
          "lease outlives registry without dangling accounting");
  surviving.reset();

  // Force a render lease to span a loader's retirement/republication.
  Require(registry.Publish(1, {Mesh(10)}), "concurrent initial model");
  std::barrier rendezvous(2);
  bool valid = false;
  std::thread reader_thread([&] {
    auto lease = registry.Find(1, 10);
    rendezvous.arrive_and_wait();
    rendezvous.arrive_and_wait();
    auto fresh = registry.Find(1, 10);
    valid = lease && fresh && lease->program.materials[0]->id == old_id &&
            fresh->program.materials[0]->id != old_id;
  });
  rendezvous.arrive_and_wait();
  const bool published = registry.Publish(1, {Mesh(10, 24)});
  rendezvous.arrive_and_wait();
  reader_thread.join();
  Require(published && valid, "loader/render ownership overlap");
  registry.Retire(1);
  Require(registry.Stats().live == 0 && registry.Stats().bytes == 0,
          "concurrent lease accounting balanced");

  std::unordered_map<uint32_t, ModelMaterialSourceNode> tree{
      {1, {2, 3, 20, true}}, {2, {0, 0, 10, true}},
      {3, {4, 0, 20, true}}, {4, {0, 0, 99, false}}};
  auto read = [&](uint32_t key) -> std::optional<ModelMaterialSourceNode> {
    const auto it = tree.find(key);
    return it == tree.end() ? std::nullopt : std::optional(it->second);
  };
  std::vector<uint32_t> sources{42};
  Require(CollectModelMaterialSources(1, read, sources) &&
          sources == std::vector<uint32_t>{20, 10}, "complete tree and shared meshes");
  const auto before = sources;
  Require(!CollectModelMaterialSources(1, read, sources, 3) && sources == before,
          "node budget is transactional");
  tree[4].child = 1;
  Require(!CollectModelMaterialSources(1, read, sources) && sources == before,
          "cycles refused transactionally");
  tree[4].child = 5;
  Require(!CollectModelMaterialSources(1, read, sources) && sources == before,
          "missing source refused transactionally");
  tree[4].child = 2;
  Require(!CollectModelMaterialSources(1, read, sources), "aliased tree node refused");
  Require(CollectModelMaterialSources(0, read, sources) && sources.empty(),
          "empty model is valid");

  // Read the source table once, then destroy it. Native associations and
  // material selection remain usable without a source reader or GPU/runtime.
  std::unordered_map<uint32_t, uint32_t> words{
      {104, 2}, {108, 200}, {116, 300}, {300, 2},
      {212, 1000}, {316, 12}, {320, 400}, {324, 2000}};
  auto reader = [&](uint32_t address) -> std::optional<uint32_t> {
    const auto it = words.find(address);
    return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  auto imported = Mesh(10);
  auto &range = imported.program.ranges[0];
  range.index_record = range.vertex_record = 1;
  const auto decoded = ReadModelGeometrySource(100, range, reader);
  Require(decoded && decoded->binding.index_buffer == 1000 &&
          decoded->binding.vertex_buffer == 2000 && decoded->vertex_count == 12 &&
          decoded->declaration_slot == 400, "load-time index/vertex/decl association");
  auto bad_range = range;
  bad_range.vertex_record = 2;
  Require(!ReadModelGeometrySource(100, bad_range, reader), "vertex table bound");
  bad_range = range;
  bad_range.index_record = 2;
  Require(!ReadModelGeometrySource(100, bad_range, reader), "index table bound");
  bad_range = range;
  bad_range.stream = 1;
  Require(!ReadModelGeometrySource(100, bad_range, reader), "unconverted stream explicit");
  Require(!ReadModelGeometrySource(UINT32_MAX - 4, range, reader), "source offset overflow");
  words[108] = UINT32_MAX - 3;
  Require(!ReadModelGeometrySource(100, range, reader), "index record address overflow");
  words[108] = 200;
  words[316] = 0;
  Require(!ReadModelGeometrySource(100, range, reader), "zero vertex count");
  words[316] = 12;
  words.erase(324);
  Require(!ReadModelGeometrySource(100, range, reader), "missing buffer word");
  imported.source_bindings[0] = decoded->binding;
  auto second_primitive = Mesh(10, 24);
  // Identical geometry may have different materials; keep primitive ordinals,
  // not a map that overwrites one material under the shared geometry key.
  imported.program.ranges.push_back(range);
  imported.program.materials.push_back(second_primitive.program.materials[0]);
  imported.program.geometries.resize(2);
  imported.source_bindings.push_back(decoded->binding);
  Require(registry.Publish(50, {std::move(imported)}), "native primitive publication");
  words.clear();
  const auto owned = registry.Find(50, 10);
  Require(owned && ModelPrimitiveMatches(owned->program.ranges[0], owned->source_bindings[0],
      1000, 2000, 0, 3), "source-free primitive lookup after source destruction");
  Require(owned->program.materials[0]->id != owned->program.materials[1]->id,
          "reused geometry preserves distinct materials");
  Require(!ModelPrimitiveMatches(owned->program.ranges[0], owned->source_bindings[0],
      1000, 2000, 1, 3), "draw range is part of association");
  Require(!ModelPrimitiveMatches(owned->program.ranges[0], {}, 0, 0, 0, 3),
          "unknown source is not a binding");
  registry.Retire(50);
  Require(registry.Publish(50, {Mesh(10)}), "primitive source reuse");
  Require(owned->source_bindings[0].index_buffer == 1000 &&
          registry.Find(50, 10)->source_bindings[0].index_buffer == 100,
          "retired primitive association cannot be repointed by source reuse");
  registry.Retire(50);
  std::cout << "native model material ownership, budgets, reload and concurrent leases passed\n";
}
