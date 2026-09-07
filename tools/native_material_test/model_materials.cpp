#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_model_geometry_source.h"
#include "gpu/scene/native_model_shadow_source.h"
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_object_primitive.h"
#include "gpu/scene/lighting_shader_bridge.h"
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
  program.shadow_policies.push_back(NativeShadowPolicy::Receive);
  mesh.source_bindings.push_back({100, 200, 300, 32});
  return mesh;
}
}

void TestNativeModelMaterials() {
  {
    // Load-time control ordering; ranges must not retain a command interpreter.
    const uint16_t commands[]{0x1000, 1, 0, 0x0301, 0x040c, 0x0500, 0x1000, 1, 3,
        0xe000, 0x040c, 0x1000, 1, 6, 0x040d, 0x1000, 1, 9,
        0x0300, 0x05ff, 0x1000, 1, 12, 0xff};
    for (bool applies : {false, true}) for (bool disable : {false, true}) {
      std::vector<NativeMaterialRange> ranges;
      Require(DecodeMeshMaterials(commands, ranges, nullptr, nullptr,
          [&](uint16_t index) -> std::optional<NativeMaterialControl> {
            Require(index == 0, "control index imported at decode");
            return NativeMaterialControl{applies, disable, disable, disable};
          }) && ranges.size() == 5, "ordered feature recipe decode");
      for (unsigned flags = 0; flags < 64; ++flags) {
        NativeMaterialObjectInputs object{};
        object.diffuse_enabled = flags & 1; object.writes_shininess = flags & 2;
        NativeLightingInputs pass{};
        pass.specular_enabled = flags & 4; pass.normal_mapping = flags & 8; pass.fog_enabled = flags & 16;
        const bool reflection = flags & 32;
        for (size_t i = 0; i < ranges.size(); ++i) {
          const auto value = ComposeNativeMaterialFeatures(ranges[i].features, object, pass, reflection);
          const bool diffuse = i == 4 ? false : i == 0 ? object.diffuse_enabled :
              (i >= 2 && applies) ? !disable && object.diffuse_enabled : true;
          const bool specular = (i == 1 || i >= 3 || (i == 2 && !applies)) &&
              object.writes_shininess && pass.specular_enabled;
          Require(value && value->diffuse == diffuse && value->specular == specular &&
              value->normal_mapping == (i > 0 && i < 4 && pass.normal_mapping) &&
              value->reflection == reflection && value->fog == bool(pass.fog_enabled),
              "live object/pass gates, restore/default and repeated power ordering");
          for (uint32_t initial : {0u, UINT32_MAX, 0xa5a5a5a5u}) {
            auto ps = initial; ApplyNativeMaterialFeatures(*value, ps);
            Require((ps & ~kNativeMaterialFeatureMask) == (initial & ~kNativeMaterialFeatureMask) &&
                (ps & kNativeMaterialFeatureMask) == PackNativeMaterialFeatures(*value),
                "feature ABI adapter preserves all unowned bits");
          }
        }
      }
    }
    std::vector<NativeMaterialRange> ranges;
    Require(DecodeMeshMaterials(commands, ranges), "unknown control preserves other owned data");
    Require(!ComposeNativeMaterialFeatures(ranges[2].features, {}, {}, false) &&
        !ComposeNativeMaterialFeatures(ranges[3].features, {}, {}, false) &&
        ComposeNativeMaterialFeatures(ranges[4].features, {}, {}, false).has_value(),
        "unknown controls refuse consumption until both switches are explicitly known");
    const auto before = ranges[4].features;
    Require(!DecodeMeshMaterials(std::span(commands).first(std::size(commands) - 1), ranges) &&
        ranges[4].features == before, "truncated feature import cannot replace retained ranges");
  }
  {
    std::unordered_map<uint32_t, uint32_t> words{{400, 0x12340756}, {408, 0x0001abcd}};
    unsigned reads = 0;
    const auto read = [&](uint32_t address) -> std::optional<uint32_t> {
      ++reads;
      const auto it = words.find(address);
      return it == words.end() ? std::nullopt : std::optional(it->second);
    };
    NativePrimitiveShaderInputs inputs;
    Require(!PackPrimitiveShaderBits(inputs), "unknown colour is not disabled");
    Require(ReadModelVertexShaderInputs(400, inputs, read) && inputs.vertex_bones == 7 &&
            inputs.vertex_colour == true && inputs.texture_layers == 1, "BE declaration fields imported once");
    const auto before = inputs;
    reads = 0;
    Require(!ReadModelVertexShaderInputs(0, inputs, read) &&
            !ReadModelVertexShaderInputs(401, inputs, read) &&
            !ReadModelVertexShaderInputs(UINT32_MAX - 3, inputs, read) && !reads,
            "null, unaligned and overflow declaration refused without reads");
    words.erase(408);
    Require(!ReadModelVertexShaderInputs(400, inputs, read) && inputs == before, "partial import is transactional");
    words[400] = 0; words[408] = 0xfffeffff;
    Require(ReadModelVertexShaderInputs(400, inputs, read) && inputs.vertex_bones == 0 &&
            inputs.vertex_colour == false, "zero bones and disabled colour are explicitly known");
    words.clear();
    for (uint8_t layers = 0; layers <= 3; ++layers) {
      inputs.texture_layers = layers;
      for (bool colour : {false, true}) {
        inputs.vertex_colour = colour;
        const auto bits = PackPrimitiveShaderBits(inputs);
        for (uint32_t initial : {0u, UINT32_MAX, 0xa5a5a5a5u}) {
          uint32_t vs = initial, ps = initial;
          Require(bits.has_value(), "owned shader bits pack after source destruction");
          ApplyPrimitiveShaderBits(*bits, vs, ps);
          Require((vs & 16u) == (colour ? 16u : 0u) && (ps & 7u) == ((1u << layers) - 1) &&
                  (vs & ~16u) == (initial & ~16u) && (ps & ~7u) == (initial & ~7u),
                  "compatibility packing changes only the four owned bits");
        }
      }
    }
    inputs.texture_layers = 4;
    Require(!PackPrimitiveShaderBits(inputs), "invalid native layer count refused");
  }
  {
    ModelMaterialRegistry models;
    NativeInstanceRegistry instances;
    auto source = Mesh(10);
    source.program.ranges[0].shader = {2, true, 0};
    // Opaque GPU-handle lifetime only: this core must never inspect backend
    // geometry bytes. Production geometry/layout pixels have separate fixtures.
    auto gpu_owner = std::make_shared<const uint32_t>(91);
    std::weak_ptr<const uint32_t> gpu_lifetime = gpu_owner;
    source.program.geometries[0] = std::shared_ptr<const NativeGeometry>(gpu_owner,
        static_cast<const NativeGeometry *>(static_cast<const void *>(gpu_owner.get())));
    const ModelNodeSourceBinding node{0, 10};
    Require(models.Publish(100, {source}, {&node, 1}), "packet model publication");
    auto model = models.FindModel(100);
    const auto id = instances.Create(model->Generation(), model);
    RenderMatrix world{};
    world[0] = world[5] = world[10] = world[15] = 1; world[12] = 7;
    Require(instances.Publish(id, 0, {&world, 1}), "packet pose publication");
    auto pose = instances.Read(id, 0);
    using Image = std::shared_ptr<const uint32_t>;
    MaterialTextureValues<Image> textures;
    textures.images[0] = std::make_shared<const uint32_t>(73);
    std::weak_ptr<const uint32_t> image_lifetime = textures.images[0];
    textures.image_mask = 1; textures.owns_uv = true; textures.uv = {1, 2, 3, 4};
    NativePrimitivePolicy policy;
    policy.routing_known = policy.direct = true;
    NativeMaterialObjectInputs object{{.5f, .25f, .75f, 1}, true};
    NativeSelectedLights lights{}; lights[0].kind = LitDirectional; lights[0].colour.x = .75f;
    NativeLightingPass lighting;
    lighting.inputs.ambient = {.125f, .25f, .5f, 1};
    lighting.inputs.specular_enabled = 1;
    auto packet = BuildNativeObjectPrimitive(pose, 0, 0, object, textures, policy, lights, {}, lighting);
    Require(packet && packet->geometry && packet->world[12] == 7 && packet->policy.direct &&
            packet->receiver_shadow == NativeShadowPolicy::Receive && (packet->material_mask & kNativeDiffuse) &&
            packet->material_values[0] == object.colour, "owned pose/material/geometry/texture/policy packet");
    Require(!BuildNativeObjectPrimitive(pose, 1, 0, object, textures, policy) &&
            !BuildNativeObjectPrimitive(pose, 0, 1, object, textures, policy) &&
            !BuildNativeObjectPrimitive({}, 0, 0, object, textures, policy), "unknown node/primitive/pose refused");
    NativeInstancePose stale = *pose; ++stale.model_generation;
    Require(!BuildNativeObjectPrimitive(std::make_shared<const NativeInstancePose>(stale), 0, 0, object, textures, policy),
            "mismatched model generation cannot assemble a packet");
    object.colour[0] = std::numeric_limits<float>::infinity();
    Require(!BuildNativeObjectPrimitive(pose, 0, 0, object, textures, policy), "nonfinite object input refused");
    textures = {}; object = {}; source = {}; lights = {}; lighting = {}; gpu_owner.reset();
    models.Retire(100); instances.Retire(id); pose.reset(); model.reset(); stale = {};
    Require(!gpu_lifetime.expired() && !image_lifetime.expired() && *packet->textures.images[0] == 73 &&
            packet->textures.uv[3] == 4 && packet->world[12] == 7 && packet->material_values[0][0] == .5f &&
            packet->lights && (*packet->lights)[0].colour.x == .75f &&
            packet->lighting && packet->lighting->inputs.ambient[0] == .125f &&
            packet->features == NativeMaterialFeatures{true, true, false, false, false} &&
            packet->shader == NativePrimitiveShaderInputs{2, true, 0},
            "queued packet survives object scope, source and model/instance retirement");
    Require(models.Publish(100, {Mesh(20, 42)}), "same source key may be reused");
    Require(packet->material->asset.properties.shininess != 42, "replacement model cannot repoint packet");
    packet.reset();
    Require(gpu_lifetime.expired() && image_lifetime.expired(), "last packet releases owned resources");
  }
  {
    ModelMaterialRegistry models;
    auto a = Mesh(100), b = Mesh(200, 24);
    a.program.bounds = std::array<float, 4>{1, 2, 3, 4};
    std::vector<ModelNodeSourceBinding> bindings{{2, 200}, {0, 100}, {1, 100}};
    Require(models.Publish(10, {a, b}, bindings), "load-owned node association");
    auto model = models.FindModel(10);
    const auto generation = model->Generation();
    Require(model->Nodes() == 3 && model->FindNode(0) == model->FindNode(1) &&
            model->FindNode(2) != model->FindNode(0) && !model->FindNode(3), "node order and shared primitives");
    NativeInstanceRegistry instances;
    Require(!instances.Create(generation + 1, model), "model/instance generation mismatch refused");
    const auto instance = instances.Create(generation, model);
    std::array<RenderMatrix, 3> matrices{};
    Require(instance && instances.Publish(instance, 1, matrices), "pose retains native model");
    auto pose = instances.Read(instance, 1);
    a = {}; b = {}; bindings.clear(); bindings.shrink_to_fit();
    models.Retire(10); model.reset();
    Require(!models.FindModel(10) && models.Stats().live == 1 &&
            FindNativeInstanceNode(*pose, 0)->bounds->at(3) == 4 && !FindNativeInstanceNode(*pose, 3),
            "native primitive/bounds selection after import storage destruction");
    const ModelNodeSourceBinding replacement{0, 300};
    Require(models.Publish(10, {Mesh(300, 42)}, {&replacement, 1}) && models.Generation(10) != generation &&
            FindNativeInstanceNode(*pose, 0)->ranges[0].material.shininess != 42, "reload cannot repoint leased model");
    instances.Retire(instance);
    Require(models.Stats().live == 2, "GPU-facing pose still pins retired model");
    pose.reset();
    Require(models.Stats().live == 1, "last pose releases model accounting");
    models.Retire(10);
    const ModelNodeSourceBinding duplicate[]{{0, 100}, {0, 200}, {2, 200}};
    Require(models.Publish(20, {Mesh(100), Mesh(200)}, duplicate), "ambiguous association preserves other nodes");
    model = models.FindModel(20);
    Require(!model->FindNode(0) && model->FindNode(2), "duplicate matrix index cannot choose wrong primitive");
    model.reset(); models.Retire(20);
    const ModelNodeSourceBinding missing{0, 999}, oversized{4096, 100};
    Require(!models.Publish(20, {Mesh(100)}, {&missing, 1}) &&
            !models.Publish(20, {Mesh(100)}, {&oversized, 1}), "unknown mesh and out-of-range node refused");
    std::vector<ModelMaterialImport> mesh{Mesh(100)};
    const auto old_size = ModelMaterialRegistry::RetainedBytes(mesh, mesh.capacity());
    ModelMaterialRegistry tight(old_size);
    const ModelNodeSourceBinding node{0, 100};
    Require(!tight.Publish(1, mesh, {&node, 1}), "node storage participates in model byte budget");
  }
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
  malformed.program.shadow_policies.clear();
  Require(!registry.Publish(4, {malformed}), "range/shadow policy count mismatch");
  malformed = Mesh(10);
  malformed.source_bindings.clear();
  Require(!registry.Publish(4, {malformed}), "range/source association count mismatch");
  malformed = Mesh(10);
  malformed.program.valid = false;
  Require(!registry.Publish(4, {malformed}), "invalid program cannot carry ranges");
  malformed = Mesh(10);
  malformed.program.ranges[0].policy_step_end = 1;
  Require(!registry.Publish(4, {malformed}), "missing primitive policy steps refused before publication");
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
  tree[1].matrix_index = 4; tree[2].matrix_index = 6; tree[3].matrix_index = 8;
  std::vector<ModelNodeSourceBinding> node_sources;
  Require(CollectModelMaterialSources(1, read, sources, 4096, &node_sources) && node_sources.size() == 3 &&
          node_sources[0].matrix_index == 4 && node_sources[1].matrix_index == 6 && node_sources[2].matrix_index == 8,
          "load traversal preserves every node including shared meshes");
  const auto previous_nodes = node_sources.size();
  Require(!CollectModelMaterialSources(1, read, sources, 1, &node_sources) && node_sources.size() == previous_nodes,
          "node publication failure leaves prior result untouched");
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
  imported.program.shadow_policies.resize(2, NativeShadowPolicy::Receive);
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

  // Asset control import is bounded and host-endian. Missing metadata differs
  // from an explicitly absent table; neither case may trigger source discovery.
  words = {{1000, 1}, {1004, 8}, {1016, 0}, {1020, 8}};
  size_t control_reads = 0;
  const auto controls = [&](uint32_t address) -> std::optional<uint32_t> {
    ++control_reads;
    return reader(address);
  };
  Require(ReadModelShadowPolicy({}, 0, controls) == NativeShadowPolicy::Unknown &&
          ReadModelShadowPolicy(0, 0, controls) == NativeShadowPolicy::Receive &&
          ReadModelShadowPolicy(1000, 0xffff, controls) == NativeShadowPolicy::Receive &&
          control_reads == 0, "unknown, null and omitted controls do not read source");
  Require(ReadModelShadowPolicy(1000, 0x1000, controls) == NativeShadowPolicy::Unknown &&
          ReadModelShadowPolicy(UINT32_MAX - 6, 0, controls) == NativeShadowPolicy::Unknown &&
          ReadModelShadowPolicy(UINT32_MAX - 16, 1, controls) == NativeShadowPolicy::Unknown &&
          control_reads == 0, "invalid control records and full-word overflow refused before reading");
  Require(ReadModelShadowPolicy(1000, 0, controls) == NativeShadowPolicy::Disabled &&
          ReadModelShadowPolicy(1000, 1, controls) == NativeShadowPolicy::Receive,
          "shadow disable requires both present and feature bits");
  Require(!ReadModelMaterialControl({}, 0, controls) &&
          !ReadModelMaterialControl(0, 0, controls)->applies &&
          ReadModelMaterialControl(1000, 1, controls)->applies &&
          !ReadModelMaterialControl(1000, 1, controls)->disable_shadow,
          "missing table, null no-op and absent-present-bit default restoration differ");
  for (uint32_t present = 0; present < 4; ++present) for (uint32_t flags = 0; flags < 16; ++flags) {
    words[1000] = present; words[1004] = flags;
    const auto control = ReadModelMaterialControl(1000, 0, controls);
    Require(control && control->applies && control->disable_diffuse == bool((present & 1) && (flags & 1)) &&
        control->disable_specular == bool((present & 1) && (flags & 2)) &&
        control->disable_shadow == bool((present & 1) && (flags & 8)), "independent present and feature bits");
  }
  words[1000] = 1;
  for (uint32_t flags = 0; flags < 16; ++flags) {
    words[1004] = flags;
    Require(ReadModelShadowPolicy(1000, 0, controls) ==
                ((flags & 8) ? NativeShadowPolicy::Disabled : NativeShadowPolicy::Receive),
            "unrelated material feature bits do not change shadow policy");
  }
  words.erase(1004);
  Require(ReadModelShadowPolicy(1000, 0, controls) == NativeShadowPolicy::Unknown,
          "missing payload is not an enabled policy");
  words[1004] = 8;
  words.erase(1000);
  Require(ReadModelShadowPolicy(1000, 0, controls) == NativeShadowPolicy::Unknown,
          "missing present mask is unknown");
  words[1000] = 1;
  auto shadow_mesh = Mesh(10);
  shadow_mesh.program.shadow_policies[0] = ReadModelShadowPolicy(1000, 0, controls);
  Require(registry.Publish(60, {shadow_mesh}), "load-owned shadow policy publication");
  const auto old_shadow = registry.Find(60, 10);
  const auto shadow_generation = registry.Generation(60);
  words.clear();
  const auto read_count = control_reads;
  Require(old_shadow && FindModelShadowPolicy(*old_shadow, 100, 200, 0, 3) == true &&
          control_reads == read_count, "native consumer survives destroyed control source");
  Require(!FindModelShadowPolicy(*old_shadow, 100, 200, 1, 3),
          "shadow lookup does not accept a different draw range");
  shadow_mesh.program.ranges.push_back(shadow_mesh.program.ranges[0]);
  shadow_mesh.source_bindings.push_back(shadow_mesh.source_bindings[0]);
  shadow_mesh.program.shadow_policies.push_back(NativeShadowPolicy::Disabled);
  Require(FindModelShadowPolicy(shadow_mesh, 100, 200, 0, 3) == true,
          "identical repeated policies are unambiguous");
  shadow_mesh.program.shadow_policies[1] = NativeShadowPolicy::Receive;
  Require(!FindModelShadowPolicy(shadow_mesh, 100, 200, 0, 3),
          "conflicting policy for shared geometry refuses both orders");
  std::swap(shadow_mesh.program.shadow_policies[0], shadow_mesh.program.shadow_policies[1]);
  Require(!FindModelShadowPolicy(shadow_mesh, 100, 200, 0, 3), "reverse conflict refused");
  shadow_mesh.program.shadow_policies[1] = NativeShadowPolicy::Unknown;
  Require(!FindModelShadowPolicy(shadow_mesh, 100, 200, 0, 3), "unknown sibling cannot inherit known policy");
  shadow_mesh.program.shadow_policies.pop_back();
  Require(!FindModelShadowPolicy(shadow_mesh, 100, 200, 0, 3), "malformed owner is not indexed out of bounds");
  Require(registry.Publish(60, {Mesh(10)}) && registry.Generation(60) > shadow_generation,
          "policy reload publishes a new generation");
  Require(FindModelShadowPolicy(*registry.Find(60, 10), 100, 200, 0, 3) == false &&
          FindModelShadowPolicy(*old_shadow, 100, 200, 0, 3) == true,
          "reload cannot repoint a retired primitive policy");
  registry.Retire(60);
  std::cout << "native model material ownership, budgets, reload and concurrent leases passed\n";
}
