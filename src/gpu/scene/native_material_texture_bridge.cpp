/**
 * @brief Publish live object image/UV inputs before visiting its primitives.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_material_texture_bridge.h"
#include "gpu/scene/native_material_texture_source.h"
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_texture_table_bridge.h"
#include "gpu/scene/native_texture_binding_bridge.h"
#include "gpu/native_texture_mirror.h"
#include "core/memory_helpers.h"
#include "core/logging.h"
#include "core/settings.h"
#include <unordered_map>
#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(bd_native_material_textures, true, kCvarGroup,
    "Object-published native material images and UV offsets for primitive submission.");
namespace bd::gpu::scene {
struct NativeObjectTextureState {
  uint32_t context = 0, visual = 0, graph = 0, table_offset = 0;
  uint64_t generation = 0;
  NativeTextureTableHandle table;
  MaterialImageSelection<NativeTextureBinding> fallback;
  MaterialTextureInputs<NativeTextureBinding> inputs;
  struct Mesh {
    std::shared_ptr<const ModelMaterialImport> owner;
    std::vector<NativeMaterialTextureValues> values;
  };
  std::unordered_map<uint32_t, Mesh> meshes;
  size_t bytes = sizeof(NativeObjectTextureState);
};
namespace {
constexpr size_t kScopeBytes = 4u << 20, kScopeDepth = 4;
constexpr uint32_t kSelection = (uint32_t(-32036) << 16) - 7864;
thread_local NativeObjectTextureState *current = nullptr;
thread_local size_t depth = 0;
struct Stats {
  uint64_t scopes = 0, unsupported = 0, refused = 0, override_scopes = 0;
  uint64_t meshes = 0, reads = 0, missing = 0, checked = 0, wrong = 0;
  uint64_t draws = 0, images = 0, uv = 0;
  size_t peak_bytes = 0;
};
thread_local Stats stats;
std::optional<uint32_t> Word(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX - 3) return {};
  const auto *word = bd::mem::try_at<const be_u32>(uint32_t(address));
  return word ? std::optional(uint32_t(*word)) : std::nullopt;
}
MaterialImageSelection<NativeTextureBinding> Capture(uint32_t source) {
  if (!source) return {MaterialImageAction::Keep};
  auto binding = CaptureNativeTexture(ResolveGuestTexture(source));
  return binding.primary ? MaterialImageSelection<NativeTextureBinding>{MaterialImageAction::Bind, std::move(binding)}
                         : MaterialImageSelection<NativeTextureBinding>{};
}
}

NativeObjectTextureScope::NativeObjectTextureScope(uint32_t context) : previous_(current) {
  current = nullptr;
  ++depth;
  if (!REXCVAR_GET(bd_native_material_textures)) return;
  if (depth > kScopeDepth) { ++stats.refused; return; }
  try {
    const auto visual = Word(context), graph = Word(uint64_t(context) + 4);
    const auto table = Word(uint64_t(context) + 12), phase = Word(uint64_t(context) + 16);
    const auto selected = Word(kSelection + 4), offset = Word(kSelection), fallback = Word(kSelection + 32);
    if (!visual || !graph || !*graph || !table || !phase || *phase ||
        !selected || *selected != *table || !offset || !fallback) { ++stats.unsupported; return; }
    auto inputs = ReadMaterialTextureInputs<NativeTextureBinding>(*visual, Word, Capture);
    if (!inputs) { ++stats.unsupported; return; }
    auto publication = std::make_unique<NativeObjectTextureState>();
    publication->generation = LoadedNativeModelGeneration(*graph);
    publication->table = *table ? FindLoadedNativeTextureTable(*table) : nullptr;
    if (!publication->generation || (*table && !publication->table)) { ++stats.unsupported; return; }
    publication->context = context; publication->visual = *visual; publication->graph = *graph;
    publication->table_offset = *offset; publication->fallback = Capture(*fallback);
    publication->inputs = std::move(*inputs);
    publication->bytes += (publication->inputs.overrides.capacity() + publication->inputs.late_images.capacity()) *
        sizeof(MaterialTextureOverride<NativeTextureBinding>);
    if (publication->bytes > kScopeBytes) { ++stats.refused; return; }
    stats.override_scopes += !publication->inputs.overrides.empty() || !publication->inputs.late_images.empty();
    owned_ = std::move(publication); current = owned_.get();
    ++stats.scopes;
  } catch (const std::exception &error) {
    ++stats.refused;
    if (stats.refused <= 3) BD_WARN("[native-material-textures] publication refused: {}", error.what());
  }
}
NativeObjectTextureScope::~NativeObjectTextureScope() { current = previous_; --depth; }

const NativeMaterialTextureValues *FindNativeMaterialTextures(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count) {
  auto *scope = current;
  if (!scope || tag.from_list || tag.ctx_va != scope->context || tag.visual_va != scope->visual ||
      scope->generation != LoadedNativeModelGeneration(scope->graph)) { ++stats.missing; return nullptr; }
  auto it = scope->meshes.find(tag.mesh_va);
  if (it == scope->meshes.end()) {
    NativeObjectTextureState::Mesh mesh;
    mesh.owner = FindLoadedNativeModelMaterials(scope->graph, tag.mesh_va);
    if (!mesh.owner) { ++stats.missing; return nullptr; }
    const auto &program = mesh.owner->program;
    constexpr size_t overhead = 256;
    if (scope->bytes > kScopeBytes - overhead || program.ranges.size() >
        (kScopeBytes - scope->bytes - overhead) / sizeof(NativeMaterialTextureValues)) {
      ++stats.refused; return nullptr;
    }
    auto lookup = [&](uint8_t selector) -> MaterialImageSelection<NativeTextureBinding> {
      if (!scope->table) return {MaterialImageAction::Keep};
      const uint32_t slot = scope->table_offset + uint32_t(selector);
      if (slot >= scope->table->slots.size()) return scope->fallback;
      const auto &value = scope->table->slots[slot];
      if (!value.available) return {};
      return {value.image.primary ? MaterialImageAction::Bind : MaterialImageAction::Keep, value.image};
    };
    if (!ComposeMaterialTextures(std::span(program.texture_assignments), std::span(program.ranges),
        scope->inputs, lookup, mesh.values)) { ++stats.refused; return nullptr; }
    scope->bytes += overhead + mesh.values.capacity() * sizeof(NativeMaterialTextureValues);
    stats.peak_bytes = std::max(stats.peak_bytes, scope->bytes);
    it = scope->meshes.emplace(tag.mesh_va, std::move(mesh)).first;
    ++stats.meshes;
  }
  const auto &mesh = it->second;
  const NativeMaterialTextureValues *found = nullptr;
  for (size_t i = 0; i < mesh.owner->program.ranges.size(); ++i) {
    if (!ModelPrimitiveMatches(mesh.owner->program.ranges[i], mesh.owner->source_bindings[i],
                               index, vertex, first, count)) continue;
    const auto &value = mesh.values[i];
    if (found && *found != value) { ++stats.missing; return nullptr; }
    found = &value;
  }
  ++(found ? stats.reads : stats.missing);
  return found;
}
void NativeMaterialTextureCheck(bool same, uint32_t channel, uint32_t visual) {
  ++stats.checked;
  if (!same && ++stats.wrong <= 4)
    BD_WARN("[native-material-texture-mismatch] visual {:08X} channel {}", visual, channel);
}
void NativeMaterialTextureNoteDraw(uint32_t image_mask, bool uv) {
  ++stats.draws; stats.images += std::popcount(image_mask); stats.uv += uv;
}
void NativeMaterialTextureReport() {
  BD_INFO("[native-material-textures] {} object publications {} with overrides; {} unsupported {} refused; "
          "{} meshes prepared, peak {} bytes; {} reads {} unavailable; {} checks wrong {}; "
          "{} draws {} image slots {} UV blocks; source object/pass setup and shader ABI remain",
      stats.scopes, stats.override_scopes, stats.unsupported, stats.refused, stats.meshes, stats.peak_bytes,
      stats.reads, stats.missing, stats.checked, stats.wrong, stats.draws, stats.images, stats.uv);
}
} // namespace bd::gpu::scene
