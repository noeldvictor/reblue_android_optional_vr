/**
 * @brief Publish live object image/UV inputs before visiting its primitives.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_material_texture_bridge.h"
#include "gpu/scene/native_material_texture_source.h"
#include "gpu/scene/native_primitive_policy_source.h"
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_lighting_bridge.h"
#include "gpu/scene/native_sampler_bridge.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/native_rigid_scene.h"
#include "gpu/scene/native_shadow_receiver_bridge.h"
#include "gpu/scene/guest_scene.h"
#include "gpu/scene/native_fog_bridge.h"
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
REXCVAR_DEFINE_BOOL(bd_native_primitive_policies, true, kCvarGroup,
    "Load-owned primitive winding and live compound draw-participation policy.");
namespace bd::gpu::scene {
struct NativeObjectTextureState {
  uint32_t context = 0, visual = 0, graph = 0, table_offset = 0;
  uint32_t render_view = 0;
  uint64_t generation = 0;
  NativeModelRenderHandle model;
  std::shared_ptr<const NativeInstancePose> pose;
  std::optional<NativeMaterialObjectInputs> object;
  std::optional<NativeSelectedLights> lights;
  std::optional<NativeNodeSelectedLights> node_lights;
  std::optional<NativeFogLayers> fog;
  uint64_t fog_revision = 0;
  NativeTextureTableHandle table;
  MaterialImageSelection<NativeTextureBinding> fallback;
  MaterialTextureInputs<NativeTextureBinding> inputs;
  std::optional<PrimitivePolicyInputs> policy_inputs;
  struct Mesh {
    std::shared_ptr<const ModelMaterialImport> owner;
    const NativeModelMaterialProgram *program = nullptr;
    std::vector<NativeMaterialTextureValues> values;
    std::vector<NativePrimitivePolicy> policies;
    std::optional<NativePrimitivePlan> plan;
  };
  std::unordered_map<const NativeModelMaterialProgram *, Mesh> meshes;
  // Only legacy replay ordinal matching needs this bounded source-key index.
  // Both routes share the same prepared values, never two material caches.
  std::unordered_map<uint32_t, Mesh *> source_meshes;
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
struct PolicyStats {
  uint64_t plans = 0, known = 0, unknown = 0, direct = 0, deferred = 0, suppressed = 0;
  uint64_t reads = 0, missing = 0, checked = 0, wrong = 0, draws = 0, changed = 0, refreshes = 0;
};
thread_local PolicyStats policy_stats;
struct ObjectStats { uint64_t publications = 0, reads = 0, missing = 0, checked = 0, wrong = 0, packets = 0; };
thread_local ObjectStats object_stats;
thread_local uint64_t shader_checked = 0, shader_wrong = 0, shader_draws = 0;
thread_local uint64_t feature_checked = 0, feature_wrong = 0, feature_draws = 0;
thread_local uint64_t sampler_checked = 0, sampler_wrong = 0, sampler_draws = 0;
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

NativeObjectTextureScope::NativeObjectTextureScope(uint32_t context,
    std::shared_ptr<const NativeInstancePose> pose) : previous_(current) {
  current = nullptr;
  ++depth;
  if (!REXCVAR_GET(bd_native_material_textures)) return;
  if (depth > kScopeDepth) { ++stats.refused; return; }
  try {
    const auto visual = Word(context), graph = Word(uint64_t(context) + 4);
    const auto table = Word(uint64_t(context) + 12), phase = Word(uint64_t(context) + 16);
    const auto render_view = Word(kRenderViewIdVa);
    const auto selected = Word(kSelection + 4), offset = Word(kSelection), fallback = Word(kSelection + 32);
    if (!visual || !graph || !*graph || !table || !phase || *phase ||
        !selected || *selected != *table || !offset || !fallback || !render_view) { ++stats.unsupported; return; }
    auto inputs = ReadMaterialTextureInputs<NativeTextureBinding>(*visual, Word, Capture);
    if (!inputs) { ++stats.unsupported; return; }
    auto publication = std::make_unique<NativeObjectTextureState>();
    publication->model = FindLoadedNativeModel(*graph);
    publication->generation = publication->model ? publication->model->Generation() : 0;
    if (pose && pose->model == publication->model) publication->pose = std::move(pose);
    publication->object = ReadMaterialObjectInputs(*visual, Word);
    publication->fog = FindNativeFogLayers();
    publication->fog_revision = NativeFogRevision();
    // Per-node light selections execute later than this scope. Do not snapshot
    // another node's lights as object-wide data; the direct path must own those updates.
    if (const auto per_node = Word(uint64_t(*visual)+3380); per_node && !*per_node)
      publication->lights = FindNativeSelectedLights(*visual+3132);
    publication->table = *table ? FindLoadedNativeTextureTable(*table) : nullptr;
    if (!publication->generation || (*table && !publication->table)) { ++stats.unsupported; return; }
    publication->context = context; publication->visual = *visual; publication->graph = *graph;
    publication->render_view = *render_view;
    if (REXCVAR_GET(bd_native_primitive_policies))
      publication->policy_inputs = ReadPrimitivePolicyInputs(context, *visual, Word);
    publication->table_offset = *offset; publication->fallback = Capture(*fallback);
    publication->inputs = std::move(*inputs);
    publication->bytes += (publication->inputs.overrides.capacity() + publication->inputs.late_images.capacity()) *
        sizeof(MaterialTextureOverride<NativeTextureBinding>);
    if (publication->bytes > kScopeBytes) { ++stats.refused; return; }
    stats.override_scopes += !publication->inputs.overrides.empty() || !publication->inputs.late_images.empty();
    owned_ = std::move(publication); current = owned_.get();
    object_stats.publications += current->object.has_value();
    ++stats.scopes;
  } catch (const std::exception &error) {
    ++stats.refused;
    if (stats.refused <= 3) BD_WARN("[native-material-textures] publication refused: {}", error.what());
  }
}
NativeObjectTextureScope::~NativeObjectTextureScope() { current = previous_; --depth; }

void InvalidateNativeMaterialLights() {
  if (current) { current->lights.reset(); current->node_lights.reset(); }
}

bool PublishNativeMaterialLights(uint32_t selection, const NativeSelectedLights &lights) {
  auto *scope = current;
  const auto &tag = CurrentNodeTag();
  if (!scope || !tag.valid || tag.from_list || tag.ctx_va != scope->context ||
      tag.visual_va != scope->visual || !scope->pose ||
      !FindNativeInstanceNode(*scope->pose, tag.node_index)) return false;
  scope->node_lights.reset();
  // sub_82142C58: a non-null per-node table entry replaces visual+3132.
  // Resolve that source binding where the authored publisher executes, never
  // from a register file or a direct packet consumer. Null/inherited nodes are
  // still unowned; another node's publication cannot supply their packet.
  const auto per_node = Word(uint64_t(scope->visual)+3380);
  if (!per_node) return false;
  uint64_t expected = uint64_t(scope->visual)+3132;
  if (*per_node) {
    const auto table = Word(uint64_t(scope->visual)+3376);
    const auto entry = table && *table ? Word(uint64_t(*table)+uint64_t(tag.node_index)*4) : std::nullopt;
    if (!entry || !*entry) return false;
    expected = *entry;
  }
  if (expected != selection) return false;
  scope->node_lights = NativeNodeSelectedLights{tag.node_index, lights};
  return true;
}

std::optional<std::vector<NativeRigidScenePlan>> PrepareNativeRigidSceneForObject(
    const NativeInstancePose &pose, uint32_t node, const char *&refusal) {
  auto *scope = current;
  refusal = "fresh ordinary object scope unavailable";
  if (!scope || scope->pose.get() != &pose || scope->model != pose.model ||
      scope->render_view != 3 || !scope->policy_inputs || scope->policy_inputs->phase != 0 ||
      scope->policy_inputs->technique != 0) return {};
  const auto *program = FindNativeInstanceNode(pose, node);
  if (!program) return {};
  refusal = "whole-node ordinary scene family unavailable";
  if (PrepareNativeRigidSceneAdmission(*program, scope->policy_inputs).route != NativeRigidCasterRoute::Native) return {};
  refusal = "fresh completed primary shadow or receiver colour unavailable";
  const auto receiver = FindNativePrimaryReceiver(scope->visual,scope->render_view);
  if (!receiver) return {};
  // This is the temporary source boundary, not a tag-based native draw API.
  NodeTag tag; tag.valid = true; tag.visual_va = scope->visual; tag.ctx_va = scope->context;
  tag.node_index = node; tag.render_view = scope->render_view;
  refusal = "live receiver visibility unavailable";
  const auto visibility = ImportNodeShadowInputs(tag);
  if (!visibility) return {};
  std::vector<NativeRigidScenePlan> result;
  result.reserve(program->ranges.size());
  for (uint32_t primitive = 0; primitive < program->ranges.size(); ++primitive) {
    refusal = "owned ordinary scene packet or lighting pass unavailable";
    auto packet = FindNativeObjectPrimitive(pose, node, primitive);
    if (!packet || !packet->lighting) return {};
    refusal = "owned scene/object lighting publication unavailable";
    const auto lights = FindNativeSceneLights(pose.instance, pose.model_generation, node, packet->lighting->inputs);
    if (!lights) return {};
    packet->lights = lights; // values survive publication/source/GPU retirement
    refusal = "whole-node scene shader resources unavailable";
    auto plan = PrepareNativeRigidScene(*program, *packet,
        {receiver->image, receiver->world_to_shadow, receiver->colour, *visibility});
    if (!plan) return {}; // No partial replacement of a multi-primitive node.
    result.push_back(std::move(*plan));
  }
  return result;
}

namespace {
NativeObjectTextureState::Mesh *PrepareMaterialMesh(const NativeModelMaterialProgram &program) {
  auto *scope = current;
  if (!scope) return nullptr;
  auto it = scope->meshes.find(&program);
  if (it == scope->meshes.end()) {
    NativeObjectTextureState::Mesh mesh;
    mesh.program = &program; // the scope's immutable model lease pins it
    constexpr size_t overhead = 256;
    if (scope->bytes > kScopeBytes - overhead || program.ranges.size() >
        (kScopeBytes - scope->bytes - overhead) /
            (sizeof(NativeMaterialTextureValues) + sizeof(NativePrimitivePolicy))) {
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
    if (scope->policy_inputs) {
      auto classify = [&](const PrimitivePolicyStep &step) {
        bool early_image = false;
        if (!scope->inputs.skip_overrides) {
          for (const auto &entry : scope->inputs.overrides) {
            if (entry.selector != step.value ||
                (entry.channel && entry.channel != uint32_t(step.channel) + 1)) continue;
            if (entry.uv) break;
            early_image |= entry.replaces_image;
          }
        }
        if (early_image) return PrimitiveTextureClass::Unchanged;
        // The original effect routing uses the base table selection, not the
        // final early/special/late image that the material samples.
        const auto image = lookup(step.value);
        if (image.action == MaterialImageAction::Keep) return PrimitiveTextureClass::Ordinary;
        if (image.action != MaterialImageAction::Bind || !image.image.primary)
          return PrimitiveTextureClass::Unknown;
        return image.image.primary->dimension == plume::RenderTextureViewDimension::TEXTURE_3D
            ? PrimitiveTextureClass::Volume : PrimitiveTextureClass::Ordinary;
      };
      if (!ComposePrimitivePolicies(std::span(program.policy_steps), std::span(program.ranges),
          *scope->policy_inputs, classify, mesh.policies)) { ++stats.refused; return nullptr; }
      mesh.plan = SummarizePrimitivePlan(mesh.policies);
      ++policy_stats.plans;
      ++(mesh.plan->known ? policy_stats.known : policy_stats.unknown);
      policy_stats.direct += mesh.plan->direct; policy_stats.deferred += mesh.plan->deferred;
      policy_stats.suppressed += mesh.plan->suppressed;
    }
    const size_t retained = overhead + mesh.values.capacity() * sizeof(NativeMaterialTextureValues) +
        mesh.policies.capacity() * sizeof(NativePrimitivePolicy);
    if (retained > kScopeBytes - scope->bytes) { ++stats.refused; return nullptr; }
    scope->bytes += retained;
    stats.peak_bytes = std::max(stats.peak_bytes, scope->bytes);
    it = scope->meshes.emplace(&program, std::move(mesh)).first;
    ++stats.meshes;
  }
  return &it->second;
}

NativeObjectTextureState::Mesh *PrepareReplayMaterialMesh(const NodeTag &tag) {
  auto *scope = current;
  if (!scope || tag.from_list || tag.ctx_va != scope->context || tag.visual_va != scope->visual ||
      scope->generation != LoadedNativeModelGeneration(scope->graph)) return nullptr;
  if (auto it = scope->source_meshes.find(tag.mesh_va); it != scope->source_meshes.end()) return it->second;
  constexpr size_t alias_bytes = 128;
  if (scope->source_meshes.size() >= 4096 || scope->bytes > kScopeBytes - alias_bytes) {
    ++stats.refused; return nullptr;
  }
  const auto owner = FindLoadedNativeModelMaterials(scope->graph, tag.mesh_va);
  if (!owner || owner.owner_before(scope->model) || scope->model.owner_before(owner)) return nullptr;
  auto *mesh = PrepareMaterialMesh(owner->program);
  if (!mesh || scope->bytes > kScopeBytes - alias_bytes) { ++stats.refused; return nullptr; }
  mesh->owner = owner;
  scope->source_meshes.emplace(tag.mesh_va, mesh);
  scope->bytes += alias_bytes;
  stats.peak_bytes = std::max(stats.peak_bytes, scope->bytes);
  return mesh;
}
} // namespace

std::optional<NativeObjectPrimitiveInputs> FindNativeObjectPrimitive(
    const NativeInstancePose &pose, uint32_t node, uint32_t primitive) {
  auto *scope = current;
  // Exact publication identity prevents another instance of the same model, or
  // a different lane/update, from borrowing this object's color and textures.
  if (!scope || scope->pose.get() != &pose || !scope->object || scope->model != pose.model) return {};
  const auto *program = FindNativeInstanceNode(pose, node);
  const auto *mesh = program ? PrepareMaterialMesh(*program) : nullptr;
  if (!mesh || primitive >= mesh->values.size() || primitive >= mesh->policies.size()) return {};
  auto result = BuildNativeObjectPrimitive(scope->pose, node, primitive,
      *scope->object, mesh->values[primitive], mesh->policies[primitive],
      SelectNativeObjectLights(scope->lights, scope->node_lights, node),
      NativeFogIsCurrent(scope->fog_revision) ? scope->fog : std::nullopt,
      FindNativeLightingPass(scope->render_view), FindNativeSamplerFilters(scope->render_view),
      FindNativePassCamera(scope->render_view));
  object_stats.packets += result.has_value();
  return result;
}

std::optional<NativeMaterialObjectInputs> FindNativeMaterialObjectInputs(const NodeTag &tag) {
  const auto *scope = current;
  if (!scope || tag.from_list || tag.tech == 11 || tag.ctx_va != scope->context || tag.visual_va != scope->visual || !scope->object) {
    ++object_stats.missing; return {};
  }
  ++object_stats.reads;
  return scope->object;
}
std::optional<NativeSelectedLights> FindNativeMaterialLights(const NodeTag &tag) {
  const auto *scope = current;
  if (!scope || tag.from_list || tag.ctx_va != scope->context || tag.visual_va != scope->visual) return {};
  return SelectNativeObjectLights(scope->lights, scope->node_lights, tag.node_index);
}
void NativeMaterialObjectInputCheck(bool same) {
  ++object_stats.checked;
  if (!same && ++object_stats.wrong <= 4) BD_WARN("[native-object-input-mismatch] object colour/shininess publication");
}
std::optional<NativeFogLayers> FindNativeMaterialFog(const NodeTag &tag) {
  const auto *scope = current;
  // A nested or late publisher invalidates older active scopes too. Do not
  // reimport source data in this consumer or borrow the preceding pass's fog.
  if (!scope || tag.from_list || tag.ctx_va != scope->context || tag.visual_va != scope->visual ||
      !NativeFogIsCurrent(scope->fog_revision)) return {};
  return scope->fog;
}

const NativeMaterialTextureValues *FindNativeMaterialTextures(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count) {
  const auto *prepared = PrepareReplayMaterialMesh(tag);
  if (!prepared) { ++stats.missing; return nullptr; }
  const auto &mesh = *prepared;
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
std::optional<NativePrimitivePolicy> FindNativePrimitivePolicy(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count) {
  if (!REXCVAR_GET(bd_native_primitive_policies)) return {};
  const auto *mesh = PrepareReplayMaterialMesh(tag);
  if (!mesh || !mesh->plan) { ++policy_stats.missing; return {}; }
  std::optional<NativePrimitivePolicy> found;
  for (size_t i = 0; i < mesh->owner->program.ranges.size(); ++i) {
    if (!ModelPrimitiveMatches(mesh->owner->program.ranges[i], mesh->owner->source_bindings[i],
                               index, vertex, first, count)) continue;
    const auto &value = mesh->policies[i];
    if (found && *found != value) { ++policy_stats.missing; return {}; }
    found = value;
  }
  ++(found ? policy_stats.reads : policy_stats.missing);
  return found;
}
std::optional<NativePrimitiveShaderInputs> FindNativePrimitiveShaderInputs(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count) {
  const auto *mesh = PrepareReplayMaterialMesh(tag);
  if (!mesh) return {};
  std::optional<NativePrimitiveShaderInputs> found;
  for (size_t i = 0; i < mesh->program->ranges.size(); ++i) {
    const auto &range = mesh->program->ranges[i];
    if (!ModelPrimitiveMatches(range, mesh->owner->source_bindings[i], index, vertex, first, count)) continue;
    if (found && *found != range.shader) return {};
    found = range.shader;
  }
  return found;
}
void NativePrimitiveShaderCheck(bool same) {
  ++shader_checked;
  if (!same && ++shader_wrong <= 4) BD_WARN("[native-primitive-shader-mismatch] texture layers/vertex colour");
}
void NativePrimitiveShaderNoteDraw() { ++shader_draws; }
std::optional<NativeMaterialFeatures> FindNativeMaterialFeatures(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count) {
  if (tag.tech != 0) return {};
  const auto *mesh = PrepareReplayMaterialMesh(tag);
  const auto lighting = NativeNodeLightingPass(tag);
  if (!mesh || !current->object || !lighting) return {};
  std::optional<NativeMaterialFeatures> found;
  for (size_t i = 0; i < mesh->program->ranges.size(); ++i) {
    const auto &range = mesh->program->ranges[i];
    if (!ModelPrimitiveMatches(range, mesh->owner->source_bindings[i], index, vertex, first, count)) continue;
    const auto value = ComposeNativeMaterialFeatures(range.features, *current->object,
        lighting->inputs, range.reflection.enabled);
    if (!value || (found && *found != *value)) return {};
    found = value;
  }
  return found;
}
void NativeMaterialFeatureCheck(bool same) {
  ++feature_checked;
  if (!same && ++feature_wrong <= 4) BD_WARN("[native-material-feature-mismatch] ordered material/pass switches");
}
void NativeMaterialFeatureNoteDraw() { ++feature_draws; }
std::optional<NativeMaterialSamplers> FindNativeMaterialSamplers(
    const NodeTag &tag, uint32_t index, uint32_t vertex, uint32_t first, uint32_t count) {
  if (tag.tech != 0) return {};
  const auto *mesh = PrepareReplayMaterialMesh(tag);
  const auto filters = FindNativeSamplerFilters(tag.render_view);
  if (!mesh || !filters) return {};
  std::optional<NativeMaterialSamplers> found;
  for (size_t i = 0; i < mesh->program->ranges.size(); ++i) {
    const auto &range = mesh->program->ranges[i];
    if (!ModelPrimitiveMatches(range, mesh->owner->source_bindings[i], index, vertex, first, count)) continue;
    const auto value = ComposeMaterialSamplers(range.sampler_addresses, *filters);
    if (found && *found != value) return {};
    found = value;
  }
  return found;
}
void NativeMaterialSamplerCheck(bool same) {
  ++sampler_checked;
  if (!same && ++sampler_wrong <= 4) BD_WARN("[native-material-sampler-mismatch] owned 2D filtering/addressing");
}
void NativeMaterialSamplerNoteDraw() { ++sampler_draws; }
std::optional<NativePrimitivePlan> FindNativePrimitivePlan(const NodeTag &tag) {
  if (!REXCVAR_GET(bd_native_primitive_policies)) return {};
  const auto *mesh = PrepareReplayMaterialMesh(tag);
  return mesh ? mesh->plan : std::nullopt;
}
void NativePrimitivePolicyCheck(bool same) {
  ++policy_stats.checked;
  if (!same && ++policy_stats.wrong <= 4) BD_WARN("[native-primitive-policy-mismatch] winding/participation");
}
void NativePrimitivePolicyNoteDraw(bool changed) { ++policy_stats.draws; policy_stats.changed += changed; }
void NativePrimitivePolicyRefresh() { ++policy_stats.refreshes; }
void NativeMaterialTextureCheck(bool same, uint32_t channel, uint32_t visual) {
  ++stats.checked;
  if (!same && ++stats.wrong <= 4)
    BD_WARN("[native-material-texture-mismatch] visual {:08X} channel {}", visual, channel);
}
void NativeMaterialTextureNoteDraw(uint32_t image_mask, bool uv) {
  ++stats.draws; stats.images += std::popcount(image_mask); stats.uv += uv;
}
void NativeMaterialTextureReport() {
  BD_INFO("[native-material-sampler] {} checks wrong {}; {} owned-input draws; ordinary 2D recipes; cube/volume/inherited axes and direct submission pending",
      sampler_checked, sampler_wrong, sampler_draws);
  BD_INFO("[native-material-feature] {} checks wrong {}; {} owned-input draws; ordinary material/pass switches; direct submission pending",
      feature_checked, feature_wrong, feature_draws);
  BD_INFO("[native-primitive-shader] {} checks wrong {}; {} owned-input draws; remaining material/pass inputs and direct submission pending",
      shader_checked, shader_wrong, shader_draws);
  NativeFogReport();
  NativeSelectedLightsReport();
  BD_INFO("[native-object-inputs] {} publications {} owned colour reads {} unavailable; {} checks wrong {}; {} owned primitive packets; no direct draw claimed",
          object_stats.publications, object_stats.reads, object_stats.missing,
          object_stats.checked, object_stats.wrong, object_stats.packets);
  BD_INFO("[native-material-textures] {} object publications {} with overrides; {} unsupported {} refused; "
          "{} meshes prepared, peak {} bytes; {} reads {} unavailable; {} checks wrong {}; "
          "{} draws {} image slots {} UV blocks; source object/pass setup and shader ABI remain",
      stats.scopes, stats.override_scopes, stats.unsupported, stats.refused, stats.meshes, stats.peak_bytes,
      stats.reads, stats.missing, stats.checked, stats.wrong, stats.draws, stats.images, stats.uv);
  BD_INFO("[native-primitive-policy] {} plans {} known {} unknown; {} direct {} deferred {} suppressed candidates; "
          "{} reads {} unavailable; {} checks wrong {}; {} draws {} cull changes {} compound refreshes; "
          "source pass setup, volume effects, callbacks and templates remain",
      policy_stats.plans, policy_stats.known, policy_stats.unknown, policy_stats.direct,
      policy_stats.deferred, policy_stats.suppressed, policy_stats.reads, policy_stats.missing,
      policy_stats.checked, policy_stats.wrong, policy_stats.draws, policy_stats.changed, policy_stats.refreshes);
}
} // namespace bd::gpu::scene
