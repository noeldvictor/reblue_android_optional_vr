/**
 * @file    gpu/scene/native_material.cpp
 * @brief   Import fixed material properties without recording shader constants.
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_model_materials.h"
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
#include "gpu/scene/guest_scene.h"

REXCVAR_DECLARE(bool, bd_native_materials_verify);

namespace bd::gpu::scene {
namespace {
ModelMaterialRegistry &Models() {
  static ModelMaterialRegistry models;
  return models;
}
std::atomic<uint64_t> model_builds{0}, model_failures{0}, unsupported_meshes{0};
thread_local uint32_t checked[3]{}, wrong[3]{}, last_report = 0;
thread_local uint32_t composed[3]{};

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
      result.valid = DecodeMeshMaterials(words, result.ranges);
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
    if (!program.valid)
      unsupported_meshes.fetch_add(1, std::memory_order_relaxed);
    meshes.push_back({mesh_va, std::move(program)});
    if (!word_budget || ModelMaterialRegistry::RetainedBytes(meshes, meshes.capacity()) >
                            ModelMaterialRegistry::kMaxBytes)
      return false;
  }
  return Models().Publish(graph, std::move(meshes));
}

std::shared_ptr<const NativeModelMaterialProgram> FindCommands(const NodeTag &tag) {
  // Technique 11 selects visual-specific colours on texture tokens. Phase 1
  // rewrites colour/shininess commands. Their native recipes remain pending.
  if (tag.from_list || !tag.ctx_va || !tag.mesh_va || tag.tech == 11 ||
      bd::mem::try_load<uint32_t>(tag.ctx_va + 16, uint32_t(-1)) != 0)
    return {};
  return Models().Find(bd::mem::try_load<uint32_t>(tag.ctx_va + 4), tag.mesh_va);
}

bool Matches(const NativeMaterialRange &range, uint32_t ib, uint32_t vb,
             uint32_t index_va, uint32_t stream_va,
             uint32_t first_index, uint32_t index_count) {
  return range.first_index == first_index && range.index_count == index_count &&
      range.stream == 0 && range.index_record != 0xffff &&
      range.vertex_record != 0xffff &&
      bd::mem::try_load<uint32_t>(ib + range.index_record * 8 + 4) == index_va &&
      bd::mem::try_load<uint32_t>(vb + 4 + range.vertex_record * 12 + 8) == stream_va;
}
} // namespace

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
  const uint32_t ib = bd::mem::try_load<uint32_t>(tag.mesh_va + 8);
  const uint32_t vb = bd::mem::try_load<uint32_t>(tag.mesh_va + 16);
  if (!ib || !vb)
    return {};
  std::optional<NativeReflectionRecipe> found;
  for (const auto &range : commands->ranges) {
    if (!Matches(range, ib, vb, index_va, stream_va, first_index, index_count))
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
  const uint32_t ib = bd::mem::try_load<uint32_t>(tag.mesh_va + 8);
  const uint32_t vb = bd::mem::try_load<uint32_t>(tag.mesh_va + 16);
  if (!ib || !vb)
    return {};
  std::optional<NativeSkinBinding> found;
  for (const auto &range : commands->ranges) {
    if (!Matches(range, ib, vb, index_va, stream_va, first_index, index_count))
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
  const uint32_t ib = bd::mem::try_load<uint32_t>(tag.mesh_va + 8);
  const uint32_t vb = bd::mem::try_load<uint32_t>(tag.mesh_va + 16);
  if (!ib || !vb)
    return {};
  NativeMaterialHandle found;
  for (size_t i = 0; i < commands->ranges.size(); ++i) {
    if (!Matches(commands->ranges[i], ib, vb, index_va, stream_va, first_index, index_count))
      continue;
    const auto &material = commands->materials[i];
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
  if (!commands)
    return {};
  const uint32_t ib = bd::mem::try_load<uint32_t>(tag.mesh_va + 8);
  const uint32_t vb = bd::mem::try_load<uint32_t>(tag.mesh_va + 16);
  const uint32_t graph = bd::mem::try_load<uint32_t>(tag.ctx_va + 4);
  const auto *table_ptr = graph ? bd::mem::try_at<const be_u32>(graph + 8) : nullptr;
  if (!ib || !vb || !table_ptr)
    return {};
  const uint32_t table = uint32_t(*table_ptr);
  std::optional<bool> found;
  for (const auto &range : commands->ranges) {
    if (!Matches(range, ib, vb, index_va, stream_va, first_index, index_count))
      continue;
    bool disables = false;
    if (table && range.control_record != 0xffff) {
      // 0x822813CC: E000 selects a 16-byte control record. sub_8228AB40
      // dispatches sub_8228AAB0 for bit 0; its fourth output byte is the
      // shadow-disable flag (payload bit 3). An absent table is a no-op.
      const uint64_t address = uint64_t(table) + uint64_t(range.control_record) * 16;
      if (address + 4 > std::numeric_limits<uint32_t>::max())
        return {};
      const auto *mask = bd::mem::try_at<const be_u32>(uint32_t(address));
      const auto *flags = bd::mem::try_at<const be_u32>(uint32_t(address + 4));
      if (!mask || !flags)
        return {};
      disables = MaterialControlDisablesShadow(uint32_t(*mask), uint32_t(*flags));
    }
    if (found && *found != disables)
      return {}; // ambiguous geometry under different feature policies
    found = disables;
  }
  return found;
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
    const auto assets = Library().Stats();
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
