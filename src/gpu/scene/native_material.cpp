/**
 * @file    gpu/scene/native_material.cpp
 * @brief   Import fixed material properties without recording shader constants.
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_model_geometry_source.h"
#include "gpu/scene/native_model_shadow_source.h"
#include "gpu/scene/native_material_texture_bridge.h"
#include "gpu/scene/native_mesh.h"
#include "gpu/scene/native_shadow.h"
#include "gpu/scene/reflection_texture_import.h"
#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "engine/game.h"
#include "gpu/frame_stats.h"
#include "gpu/host_resource_heap.h"
#include "gpu/physical_buffers.h"
#include "gpu/scene/guest_scene.h"

REXCVAR_DECLARE(bool, bd_native_materials_verify);
REXCVAR_DECLARE(bool, bd_native_meshes);

namespace bd::gpu::scene {
namespace {
ModelMaterialRegistry &Models() {
  static ModelMaterialRegistry models;
  return models;
}
std::atomic<uint64_t> model_builds{0}, model_failures{0}, unsupported_meshes{0};
std::atomic<uint64_t> geometry_loaded{0}, geometry_unconverted{0};
std::atomic<uint64_t> geometry_hits{0}, geometry_misses{0};
std::atomic<uint64_t> shadow_policy_loaded{0}, shadow_policy_disabled{0}, shadow_policy_unknown{0};
std::atomic<uint64_t> shadow_policy_hits{0}, shadow_policy_misses{0};
thread_local uint64_t geometry_draws = 0, geometry_checked = 0, geometry_wrong = 0;
thread_local uint32_t checked[3]{}, wrong[3]{}, last_report = 0;
thread_local uint32_t composed[3]{};
thread_local uint64_t native_lit_queued = 0;

NativeMaterialLibrary &Library() {
  static NativeMaterialLibrary library([] {
    std::filesystem::path root;
    if (auto *runtime = rex::Runtime::instance())
      root = runtime->cache_root();
    if (root.empty())
      root = std::filesystem::current_path();
    return root / "native_materials" / "v1";
  }());
  return library;
}

NativeModelMaterialProgram ReadCommands(uint32_t source, size_t &word_budget) {
  NativeModelMaterialProgram result;
  std::vector<uint16_t> words;
  if (!source)
    return result;
  while (words.size() < 65536) {
    auto read = [&]() {
      if (!word_budget)
        return false;
      const uint64_t address = uint64_t(source) + words.size() * 2;
      if (address > std::numeric_limits<uint32_t>::max())
        return false;
      const auto *word = bd::mem::try_at<const be_u16>(uint32_t(address));
      if (!word)
        return false;
      words.push_back(uint16_t(*word));
      --word_budget;
      return true;
    };
    if (!read())
      return result;
    const uint16_t command = words.back();
    const int operands = MeshCommandOperands(command);
    if (operands < 0 || words.size() + size_t(operands) > 65536)
      return result;
    for (int i = 0; i < operands; ++i)
      if (!read())
        return result;
    if (command == 0xff) {
      result.valid = DecodeMeshMaterials(words, result.ranges, &result.texture_assignments, &result.policy_steps);
      if (result.valid) {
        result.materials.reserve(result.ranges.size());
        for (const auto &range : result.ranges)
          result.materials.push_back(Library().Resolve({range.material}));
      }
      return result;
    }
  }
  return result;
}

void LoadModelGeometry(ModelMaterialImport &mesh) {
  auto &program = mesh.program;
  program.geometries.resize(program.ranges.size());
  mesh.source_bindings.resize(program.ranges.size());
  // bdSceneGraphNodeProcess: mesh+4 counts 8-byte index records at +8;
  // mesh+16 points to {count, 12-byte vertex records}. Each vertex record is
  // {vertex count, declaration-cache slot, buffer}. The slot's declaration is
  // at +12. Physical registration records the exact expanded byte length, so
  // length / vertex count is the stride computed by the original loader.
  const auto word = [](uint64_t address) -> std::optional<uint32_t> {
    if (!address || address > UINT32_MAX - 3)
      return {};
    const auto *value = bd::mem::try_at<const be_u32>(uint32_t(address));
    return value ? std::optional(uint32_t(*value)) : std::nullopt;
  };
  for (size_t i = 0; i < program.ranges.size(); ++i) {
    const auto &range = program.ranges[i];
    const auto source = ReadModelGeometrySource(mesh.source_mesh, range, word);
    if (!source) {
      ++geometry_unconverted;
      continue;
    }
    auto &binding = mesh.source_bindings[i];
    binding = source->binding;
    const auto declaration = word(uint64_t(source->declaration_slot) + 12);
    ResourceType type;
    const auto *decl = declaration && HostResourceHeap::GetType(*declaration, &type) &&
        type == ResourceType::VertexDeclaration
        ? HostResourceHeap::FromGuest<GuestVertexDeclaration>(*declaration) : nullptr;
    const auto buffer = [](uint32_t key, ResourceType expected) -> GuestBuffer * {
      ResourceType type;
      if (HostResourceHeap::GetType(key, &type))
        return type == expected ? HostResourceHeap::FromGuest<GuestBuffer>(key) : nullptr;
      auto *found = FindPhysicalBufferByStruct(key);
      return found && found->type == expected ? found : nullptr;
    };
    const auto *index = buffer(binding.index_buffer, ResourceType::IndexBuffer);
    const auto *vertex = buffer(binding.vertex_buffer, ResourceType::VertexBuffer);
    bool one_stream = decl && decl->vertexStreams[0];
    for (size_t stream = 1; decl && stream < 16; ++stream)
      one_stream &= !decl->vertexStreams[stream];
    if (!one_stream || !index || !vertex || index->ownsMirror || vertex->ownsMirror ||
        vertex->dataSize % source->vertex_count || !REXCVAR_GET(bd_native_meshes)) {
      ++geometry_unconverted;
      continue;
    }
    binding.layout = decl->hash;
    binding.stride = vertex->dataSize / source->vertex_count;
    NativeMeshImport request;
    request.persist = !REXCVAR_GET(bd_native_materials_verify);
    request.declaration = decl;
    request.index = index;
    request.streams[0] = vertex;
    request.strides[0] = binding.stride;
    request.start_index = range.first_index;
    request.count = range.index_count;
    // Complete bdSceneNodeDrawIndexed: base vertex 0, triangle strip (6),
    // StartIndex from operand 2 and count from operand 1 plus two.
    request.primitive_type = 6;
    program.geometries[i] = ImportNativeMesh(request);
    ++(program.geometries[i] ? geometry_loaded : geometry_unconverted);
  }
}

bool PublishModelMaterials(uint32_t graph) {
  // Called once after the graph builder completes, on the loader thread, with
  // all nodes and buffer/declaration tables ready. No optional PSO gate and no
  // first-draw snapshot. The original builder remains a temporary load adapter.
  Models().Retire(graph);
  if (!graph || graph > UINT32_MAX - 19)
    return false;
  const auto *root = bd::mem::try_at<const be_u32>(graph + 16);
  if (!root)
    return false;
  const auto *control_table = bd::mem::try_at<const be_u32>(graph + 8);
  const auto table = control_table ? std::optional(uint32_t(*control_table)) : std::nullopt;
  std::vector<uint32_t> mesh_keys;
  if (!CollectModelMaterialSources(uint32_t(*root), [](uint32_t node_va)
      -> std::optional<ModelMaterialSourceNode> {
        if (!node_va || node_va > UINT32_MAX - sizeof(GuestDrawNode) + 1 ||
            !bd::mem::try_at<const uint8_t>(node_va + sizeof(GuestDrawNode) - 1))
          return {};
        const auto *node = bd::mem::try_at<const GuestDrawNode>(node_va);
        if (!node)
          return {};
        // Visibility is live instance state. Include initially hidden/pruned
        // nodes too, rather than freezing visibility into material ownership.
        return ModelMaterialSourceNode{uint32_t(node->child), uint32_t(node->sibling),
            uint32_t(node->mesh), bool(uint32_t(node->flags) & kNodeHasGeometry)};
      }, mesh_keys, ModelMaterialRegistry::kMaxMeshes))
    return false;
  std::vector<ModelMaterialImport> meshes;
  meshes.reserve(mesh_keys.size());
  size_t word_budget = 1u << 20; // aggregate source work, not per mesh
  for (uint32_t mesh_va : mesh_keys) {
    const auto *commands = bd::mem::try_at<const be_u32>(mesh_va);
    if (!commands)
      return false;
    auto program = ReadCommands(uint32_t(*commands), word_budget);
    // The completed graph builder has relocated the asset control table. Own
    // its per-primitive result now, under the same generation as the material
    // and geometry. Visual overrides and pass/visibility inputs are not frozen.
    program.shadow_policies.reserve(program.ranges.size());
    for (const auto &range : program.ranges) {
      const auto policy = ReadModelShadowPolicy(table, range.control_record,
          [](uint32_t address) -> std::optional<uint32_t> {
            const auto *word = bd::mem::try_at<const be_u32>(address);
            return word ? std::optional(uint32_t(*word)) : std::nullopt;
          });
      program.shadow_policies.push_back(policy);
      ++(policy == NativeShadowPolicy::Unknown ? shadow_policy_unknown : shadow_policy_loaded);
      if (policy == NativeShadowPolicy::Disabled)
        ++shadow_policy_disabled;
    }
    if (!program.valid)
      unsupported_meshes.fetch_add(1, std::memory_order_relaxed);
    meshes.push_back({mesh_va, std::move(program)});
    LoadModelGeometry(meshes.back());
    if (!word_budget || ModelMaterialRegistry::RetainedBytes(meshes, meshes.capacity()) >
                            ModelMaterialRegistry::kMaxBytes)
      return false;
  }
  return Models().Publish(graph, std::move(meshes));
}

std::shared_ptr<const ModelMaterialImport> FindCommands(const NodeTag &tag) {
  // Technique 11 selects visual-specific colours on texture tokens. Phase 1
  // rewrites colour/shininess commands. Their native recipes remain pending.
  if (tag.from_list || !tag.ctx_va || !tag.mesh_va || tag.tech == 11 ||
      bd::mem::try_load<uint32_t>(tag.ctx_va + 16, uint32_t(-1)) != 0)
    return {};
  return Models().Find(bd::mem::try_load<uint32_t>(tag.ctx_va + 4), tag.mesh_va);
}

} // namespace

uint64_t LoadedNativeModelGeneration(uint32_t source_model) {
  return Models().Generation(source_model);
}

std::shared_ptr<const ModelMaterialImport> FindLoadedNativeModelMaterials(
    uint32_t source_model, uint32_t source_mesh) {
  return Models().Find(source_model, source_mesh);
}

std::shared_ptr<const NativeGeometry> FindLoadedNativeGeometry(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count, uint64_t layout, uint32_t stride) {
  const auto model = FindCommands(tag);
  std::shared_ptr<const NativeGeometry> found;
  if (model) {
    for (size_t i = 0; i < model->program.ranges.size(); ++i) {
      const auto &binding = model->source_bindings[i];
      if (!ModelPrimitiveMatches(model->program.ranges[i], binding, index_va,
                                 stream_va, first_index, index_count))
        continue;
      const auto &geometry = model->program.geometries[i];
      if (!geometry || binding.layout != layout || binding.stride != stride ||
          (found && found != geometry)) {
        ++geometry_misses;
        return {};
      }
      found = geometry;
    }
  }
  ++(found ? geometry_hits : geometry_misses);
  return found;
}

void NativeModelGeometryCheck(bool same) {
  ++geometry_checked;
  geometry_wrong += !same;
}

void NativeModelGeometryNoteDraw(bool load_owned) {
  geometry_draws += load_owned;
}

bool ModelOwnsReflectionBinding(const NodeTag &tag) {
  const uint32_t vtable = tag.visual_va
      ? bd::mem::try_load<uint32_t>(tag.visual_va) : 0;
  const auto *begin = vtable && vtable <= UINT32_MAX - 35
      ? bd::mem::try_at<const be_u32>(vtable + 32) : nullptr;
  // sub_8221DB00 calls visual vf08 after model commands. This material
  // callback binds current/next scene targets to slots 5/10 through
  // sub_8221E618, overriding the model recipe. Its own native pass recipe
  // (and placement between sub-draws) is a separate conversion boundary.
  return begin && ModelReflectionCallbackSupported(uint32_t(*begin));
}

std::optional<NativeReflectionRecipe> ImportNativeReflectionRecipe(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count) {
  if (!ModelOwnsReflectionBinding(tag))
    return {};
  const auto commands = FindCommands(tag);
  if (!commands)
    return {};
  std::optional<NativeReflectionRecipe> found;
  for (size_t i = 0; i < commands->program.ranges.size(); ++i) {
    const auto &range = commands->program.ranges[i];
    if (!ModelPrimitiveMatches(range, commands->source_bindings[i], index_va,
                               stream_va, first_index, index_count))
      continue;
    if (range.reflection.source == ReflectionTextureSource::Unknown ||
        (found && *found != range.reflection))
      return {}; // ambiguous geometry or an unconverted texture override
    found = range.reflection;
  }
  return found;
}

std::optional<NativeSkinBinding> ImportNativeSkinBinding(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count) {
  if (tag.from_list) {
    if (!tag.bone_table_va || tag.bone_count > NativeSkinBinding::kCapacity)
      return {};
    std::array<uint16_t, NativeSkinBinding::kCapacity> joints{};
    for (uint32_t i = 0; i < tag.bone_count; ++i) {
      const uint64_t address = uint64_t(tag.bone_table_va) + i * 4;
      if (address > UINT32_MAX - 3)
        return {};
      const auto *joint = bd::mem::try_at<const be_u32>(uint32_t(address));
      if (!joint || uint32_t(*joint) > UINT16_MAX)
        return {};
      joints[i] = uint16_t(uint32_t(*joint));
    }
    return DecodeNativeSkinBinding(std::span(joints).first(tag.bone_count));
  }
  const auto commands = FindCommands(tag);
  if (!commands)
    return {};
  std::optional<NativeSkinBinding> found;
  for (size_t i = 0; i < commands->program.ranges.size(); ++i) {
    const auto &range = commands->program.ranges[i];
    if (!ModelPrimitiveMatches(range, commands->source_bindings[i], index_va,
                               stream_va, first_index, index_count))
      continue;
    if (!range.skin || (found && *found != *range.skin))
      return {}; // geometry reused under different joint bindings is ambiguous
    found = range.skin;
  }
  return found;
}

NativeMaterialHandle ImportNativeMaterial(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count) {
  const auto commands = FindCommands(tag);
  if (!commands)
    return {};
  NativeMaterialHandle found;
  for (size_t i = 0; i < commands->program.ranges.size(); ++i) {
    if (!ModelPrimitiveMatches(commands->program.ranges[i], commands->source_bindings[i],
                               index_va, stream_va, first_index, index_count))
      continue;
    const auto &material = commands->program.materials[i];
    if (!material || (found && found->id != material->id))
      return {}; // repeated geometry under different materials is ambiguous
    found = material;
  }
  return found;
}

std::optional<bool> ImportMaterialDisablesShadow(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count) {
  const auto commands = FindCommands(tag);
  const auto policy = commands ? FindModelShadowPolicy(
      *commands, index_va, stream_va, first_index, index_count) : std::nullopt;
  ++(policy ? shadow_policy_hits : shadow_policy_misses);
  return policy;
}

uint32_t EvaluateNativeMaterial(const NodeTag &tag,
                               const NativeMaterialAsset &material,
                               std::array<float, 4> values[3]) {
  if (!tag.visual_va || tag.from_list || !tag.ctx_va || tag.tech == 11 ||
      bd::mem::try_load<uint32_t>(tag.ctx_va + 16, uint32_t(-1)) != 0)
    return 0;
  std::array<float, 4> object_colour;
  for (uint32_t i = 0; i < 4; ++i) {
    const auto *component = bd::mem::try_at<const be_f32>(
        tag.visual_va + kVisualMaterialColor + i * 4);
    if (!component)
      return 0;
    object_colour[i] = float(*component);
  }
  return ComposeNativeMaterialAsset(material, object_colour,
      bd::mem::try_load<uint32_t>(tag.visual_va + 3044) != 0,
      values);
}

void NativeMaterialCheck(uint32_t mask, const std::array<float, 4> values[3],
                         const uint8_t *pixel_constants) {
  for (uint32_t field = 0; field < 3; ++field) {
    if (!(mask & (1u << field)))
      continue;
    ++checked[field];
    bool equal = true;
    for (uint32_t i = 0; i < 4; ++i) {
      float actual;
      std::memcpy(&actual, pixel_constants + (field + 3) * 16 + i * 4, 4);
      equal &= std::isfinite(actual) &&
          std::fabs(actual - values[field][i]) <= 1e-6f * (1 + std::fabs(actual));
    }
    if (!equal)
      ++wrong[field];
  }
  NativeMaterialNoteReplay(0);
}

void NoteNativeLitQueuedDraw() { ++native_lit_queued; }
void NativeMaterialNoteReplay(uint32_t mask) {
  for (uint32_t field = 0; field < 3; ++field)
    if (mask & (1u << field))
      ++composed[field];
  const uint32_t frame = FrameStatFrameCount();
  if (frame - last_report >= 300) {
    const auto models = Models().Stats();
    if (REXCVAR_GET(bd_native_materials_verify)) {
      // A water/update count also fires in the opening cinematic. Report the
      // existing semantic state readers so checks can identify their scenario.
      const auto &game = bd::engine::Game::Get();
      BD_INFO("[native-material-context] frame {} mode {} field-state {} stage {} "
              "player {} event {} movie {} loader-busy {} icon-visible {}",
              frame, bd::engine::ToString(game.Mode()), game.FieldState(),
              game.Stage().Name(), game.Field().HasPlayer() ? 1 : 0,
              bd::engine::EventScenePlaying() ? 1 : 0,
              bd::engine::SofdecMoviePlaying() ? 1 : 0,
              game.IsLoading() ? 1 : 0, game.LoadingScreenUp() ? 1 : 0);
    }
    BD_INFO("[native-material] source checks (wrong/checked): diffuse {}/{}, "
            "specular {}/{}, reflection {}/{}; {} load-owned models; "
            "replay fields composed {}/{}/{}",
            wrong[0], checked[0], wrong[1], checked[1], wrong[2], checked[2],
            models.indexed, composed[0], composed[1], composed[2]);
    BD_INFO("[native-model-materials] {} builds, {} published, {} retired, {} live / {} bytes; "
            "{} load failures, {} unsupported meshes, {} budget/input refusals; "
            "{} lookups hit / {} missing; no draw-time command discovery",
            model_builds.load(), models.published, models.retired, models.live, models.bytes,
            model_failures.load(), unsupported_meshes.load(), models.refused,
            models.hits, models.misses);
    BD_INFO("[native-model-shadow] {} load-owned policies ({} disabled), {} unknown; "
            "{} draw lookups hit / {} unavailable; no draw-time control reads",
            shadow_policy_loaded.load(), shadow_policy_disabled.load(), shadow_policy_unknown.load(),
            shadow_policy_hits.load(), shadow_policy_misses.load());
    BD_INFO("[native-model-geometry] {} load-owned primitives, {} unconverted; "
            "{} replay lookups hit / {} unavailable; {} load-owned draws; "
            "{} source checks wrong {}; buffer associations resolved at load",
            geometry_loaded.load(), geometry_unconverted.load(),
            geometry_hits.load(), geometry_misses.load(), geometry_draws,
            geometry_checked, geometry_wrong);
    const auto assets = Library().Stats();
    NativeMaterialTextureReport();
    BD_INFO("[native-lit-shading] {} normal-lit queued draws; named light/fog evaluator; "
            "source shader selection, binding ABI and templates remain", native_lit_queued);
    BD_INFO("[native-material-assets] {} cooked, {} loaded, {} resident, {} memory hits; "
            "{} invalid, {} write failures, {} budget refusals; {} model recipe bytes",
            assets.cooked, assets.loaded, assets.resident, assets.memory_hits,
            assets.invalid, assets.write_failures, assets.budget_refusals, models.bytes);
    BD_INFO("[native-material-disk] {} budget refusals; last write inventory {} files / {} logical bytes, complete {}; "
            "disk-full keeps native resident data, never evicts files",
            assets.disk_budget_refusals, assets.disk_files, assets.disk_bytes,
            assets.disk_inventory_complete);
    last_report = frame;
  }
}
} // namespace bd::gpu::scene

REX_EXTERN(__imp__bdSceneGraphBuild);
REX_EXTERN(__imp__sub_8227EBE8);

REX_HOOK_RAW(bdSceneGraphBuild) {
  __imp__bdSceneGraphBuild(ctx, base);
  using namespace bd::gpu::scene;
  if (!ctx.r3.u32)
    return;
  model_builds.fetch_add(1, std::memory_order_relaxed);
  try {
    if (!PublishModelMaterials(ctx.r3.u32))
      model_failures.fetch_add(1, std::memory_order_relaxed);
  } catch (const std::exception &error) {
    model_failures.fetch_add(1, std::memory_order_relaxed);
    BD_WARN("[native-model-materials] load import failed: {}", error.what());
  }
}

REX_HOOK_RAW(sub_8227EBE8) {
  // Complete destructor entry, before node/declaration release. Unlike the
  // physical-block free hook, this also covers graphs with no physical block.
  bd::gpu::scene::Models().Retire(ctx.r3.u32);
  __imp__sub_8227EBE8(ctx, base);
}
