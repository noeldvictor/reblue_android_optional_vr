/**
 * @brief Publish live object image/UV inputs before visiting its primitives.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_material_texture_bridge.h"
#include "gpu/scene/native_material_texture_source.h"
#include "gpu/scene/native_material_alpha_source.h"
#include "gpu/scene/native_alpha_bridge.h"
#include "gpu/scene/native_blend_bridge.h"
#include "gpu/scene/native_rigid_draw.h"
#include "gpu/scene/native_primitive_policy_source.h"
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_lighting_bridge.h"
#include "gpu/scene/native_sampler_bridge.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/native_rigid_scene.h"
#include "gpu/scene/deferred_consumer.h"
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
#include <cstring>
#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(bd_native_material_textures, true, kCvarGroup,
    "Object-published native material images and UV offsets for primitive submission.");
REXCVAR_DEFINE_BOOL(bd_native_primitive_policies, true, kCvarGroup,
    "Load-owned primitive winding and live compound draw-participation policy.");
namespace bd::gpu::scene {
struct NativeObjectTextureState {
  uint32_t context = 0, visual = 0, graph = 0, table_offset = 0;
  uint32_t render_view = 0;
  bool shadow_phase = false;
  uint32_t stack = 0;
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
  std::optional<NativeMaterialAlphaInputs> alpha_inputs;
  std::optional<NativeRigidCutoutInputs> cutout_pass;
  std::optional<NativeRigidDeferredInputs> deferred_inputs;
  std::optional<NativeMaterialShadowInputs> shadow_inputs;
  std::optional<NativeSamplerFilterPass> shadow_filters;
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
std::optional<NativeRigidCutoutInputs> CaptureCutoutPass() {
  const auto alpha = FindNativeAlphaIntent();
  const auto blend = FindNativeEnabledBlendIntent();
  if (!alpha || !blend) return {};
  uint32_t comparison;
  switch (alpha->compare) {
  case AlphaCompare::GreaterEqual: comparison = RigidCutoutGE; break;
  case AlphaCompare::Never: comparison = RigidCutoutNever; break;
  case AlphaCompare::Less: comparison = RigidCutoutLess; break;
  case AlphaCompare::Equal: comparison = RigidCutoutEqual; break;
  case AlphaCompare::LessEqual: comparison = RigidCutoutLE; break;
  case AlphaCompare::Greater: comparison = RigidCutoutGreater; break;
  case AlphaCompare::NotEqual: comparison = RigidCutoutNE; break;
  case AlphaCompare::Always: comparison = RigidCutoutAlways; break;
  default: return {};
  }
  return NativeRigidCutoutInputs{0, comparison, alpha->alpha_to_coverage, *blend};
}
}

NativeObjectTextureScope::NativeObjectTextureScope(uint32_t context,
    std::shared_ptr<const NativeInstancePose> pose, uint32_t stack) : previous_(current) {
  current = nullptr;
  ++depth;
  if (!REXCVAR_GET(bd_native_material_textures)) return;
  if (depth > kScopeDepth) { ++stats.refused; return; }
  try {
    const auto visual = Word(context), graph = Word(uint64_t(context) + 4);
    const auto table = Word(uint64_t(context) + 12), phase = Word(uint64_t(context) + 16);
    const auto render_view = Word(kRenderViewIdVa);
    const auto selected = Word(kSelection + 4), offset = Word(kSelection), fallback = Word(kSelection + 32);
    if (!visual || !graph || !*graph || !table || !phase || *phase > 1 ||
        !selected || *selected != *table || !offset || !fallback || !render_view) { ++stats.unsupported; return; }
    if (*phase == 1 && (!NativeRigidShadowEnabled() || *render_view != 1)) return;
    auto inputs = ReadMaterialTextureInputs<NativeTextureBinding>(*visual, Word, Capture);
    if (!inputs) { ++stats.unsupported; return; }
    auto publication = std::make_unique<NativeObjectTextureState>();
    publication->model = FindLoadedNativeModel(*graph);
    publication->generation = publication->model ? publication->model->Generation() : 0;
    if (pose && pose->model == publication->model) publication->pose = std::move(pose);
    if (*phase == 0) {
      publication->object = ReadMaterialObjectInputs(*visual, Word);
      publication->fog = FindNativeFogLayers();
      publication->fog_revision = NativeFogRevision();
      // Per-node light selections execute later than this scope. Do not snapshot
      // another node's lights as object-wide data; the direct path must own those updates.
      if (const auto per_node = Word(uint64_t(*visual)+3380); per_node && !*per_node)
        publication->lights = FindNativeSelectedLights(*visual+3132);
    }
    publication->table = *table ? FindLoadedNativeTextureTable(*table) : nullptr;
    if (!publication->generation || (*table && !publication->table)) { ++stats.unsupported; return; }
    publication->context = context; publication->visual = *visual; publication->graph = *graph;
    publication->render_view = *render_view;
    publication->shadow_phase = *phase == 1;
    publication->stack = stack;
    if (REXCVAR_GET(bd_native_primitive_policies))
      publication->policy_inputs = ReadPrimitivePolicyInputs(context, *visual, Word);
    if (NativeRigidSceneEnabled() && *render_view == 3) {
      publication->alpha_inputs = ReadMaterialAlphaInputs(*visual, Word);
      publication->cutout_pass = CaptureCutoutPass();
      if (NativeRigidDeferredEnabled()) {
        // bdSceneNodeDrawSingle, 82280C3C..82280C6C: shadow_allowed
        // gates the live sort-disabled word passed to the depth helper.
        const auto fixed = Word((uint32_t(-32035) << 16) - 26168);
        const auto depth_word = Word((uint32_t(-32251) << 16) + 20912);
        if (fixed && depth_word)
          publication->deferred_inputs = NativeRigidDeferredInputs{*fixed != 0, std::bit_cast<float>(*depth_word)};
      }
    }
    if (publication->shadow_phase) {
      publication->shadow_inputs = ReadMaterialShadowInputs(*visual, Word);
      publication->shadow_filters = FindNativeSamplerFilters(*render_view);
    }
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
  if (!scope || scope->shadow_phase || !tag.valid || tag.from_list || tag.ctx_va != scope->context ||
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
  const auto admission = PrepareNativeRigidSceneAdmission(*program, scope->policy_inputs, NativeRigidDeferredEnabled());
  if (admission.route != NativeRigidCasterRoute::Native) return {};
  const bool cutouts = std::any_of(admission.policies.begin(), admission.policies.end(),
      [](const auto &policy) { return policy.alpha_test; });
  std::vector<uint32_t> references;
  if (cutouts) {
    refusal = "owned whole-node cutout references or pass unavailable";
    if (!scope->alpha_inputs || !scope->cutout_pass ||
        !ComposeMaterialAlphaReferences(program->ranges, admission.policies, *scope->alpha_inputs, references)) return {};
  }
  refusal = "fresh completed primary shadow or receiver colour unavailable";
  const auto receiver = FindNativePrimaryReceiver({pose.instance, pose.model_generation},scope->render_view);
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
    const auto lights = CaptureNativeSceneLights(pose.instance, pose.model_generation, node, packet->lighting->inputs);
    if (!lights) return {};
    packet->lights.reset(); // no walk-time/inherited shader values in a pending packet
    auto cutout = cutouts ? scope->cutout_pass : std::nullopt;
    if (cutout) cutout->reference = references[primitive];
    refusal = "whole-node scene shader resources unavailable";
    auto plan = PrepareNativeRigidScene(*program, *packet,
        {receiver->image, receiver->world_to_shadow, receiver->colour, *visibility}, &refusal, cutout, scope->deferred_inputs, lights);
    if (!plan) {
      // Failure-only context for the exact next producer decision. No probe
      // loop, frame dump, weakened admission or partial sibling replacement.
      BD_ERROR("[native-scene-packet] instance {} generation {} node {} primitive {} geometry {:016X} material {:016X} material mask {} layers {} image mask {}; {}",
          pose.instance,pose.model_generation,node,primitive,packet->geometry->id,packet->material->id,
          packet->material_mask,packet->shader.texture_layers,packet->textures.image_mask,refusal);
      return {};
    }
    result.push_back(std::move(*plan));
  }
  return result;
}
bool StageNativeRigidSceneDeferredForObject(NativeRigidSceneSubmission &submission) {
  if (std::none_of(submission.plans.begin(), submission.plans.end(),
      [](const auto &plan) { return plan.deferred; })) return true;
  const auto *scope = current;
  return scope && scope->pose && scope->pose->instance == submission.instance &&
      scope->pose->model_generation == submission.model_generation &&
      StageNativeDeferredScene(scope->visual, submission);
}
void ReportNativeMaterialUvMismatch(const NodeTag &tag,
    const NativeMaterialTextureValues &values, const void *actual) {
  if (!current || !actual || stats.wrong > 4) return;
  // These checked reads are diagnostic only. Distinguish an import/recipe
  // disagreement from a later staging/flush writer without seeding ownership
  // from whichever legacy value happened to survive.
  constexpr uint32_t staging = (uint32_t(-32034) << 16) - 32552;
  constexpr uint32_t scratch = (uint32_t(-32034) << 16) - 22068;
  std::array<uint32_t,4> observed;
  std::memcpy(observed.data(),actual,sizeof(observed));
  BD_WARN("[native-uv-provenance] visual {:08X} node {} mesh {:08X} view {} tech {} generation {} overrides {}",
      tag.visual_va,tag.node_index,tag.mesh_va,tag.render_view,tag.tech,current->generation,current->inputs.overrides.size());
  for (uint32_t n=0;n<4;++n) {
    const auto source=Word(uint64_t(tag.visual_va)+3444+n*4);
    const auto staged=Word(staging+32+n*4), working=Word(scratch+n*4);
    BD_WARN("[native-uv-lane] {} owned {:08X} actual {:08X} initial {:08X} reset {:08X} live {:08X} staged {:08X} scratch {:08X} readable {}{}{}",
        n,std::bit_cast<uint32_t>(values.uv[n]),observed[n],
        std::bit_cast<uint32_t>(current->inputs.initial_uv[n]),std::bit_cast<uint32_t>(current->inputs.reset_uv[n]),
        source.value_or(0),staged.value_or(0),working.value_or(0),source.has_value(),staged.has_value(),working.has_value());
  }
  for (size_t n=0;n<std::min<size_t>(8,current->inputs.overrides.size());++n) {
    const auto &entry=current->inputs.overrides[n];
    BD_WARN("[native-uv-override] {} selector {} channel {} uv {} {:08X} {:08X} image {}",
        n,entry.selector,entry.channel,entry.uv.has_value(),entry.uv ? std::bit_cast<uint32_t>((*entry.uv)[0]):0,
        entry.uv ? std::bit_cast<uint32_t>((*entry.uv)[1]):0,entry.replaces_image);
  }
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
        scope->inputs, lookup, mesh.values, 4096, scope->shadow_phase)) { ++stats.refused; return nullptr; }
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
  if (!scope || scope->shadow_phase || tag.from_list || tag.ctx_va != scope->context || tag.visual_va != scope->visual ||
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

std::optional<std::vector<NativeRigidShadowPlan>> PrepareNativeRigidShadowForObject(
    const NativeInstancePose &pose, uint32_t node, const RenderCamera &camera, const char *&refusal) {
  const auto *scope = current;
  refusal = "shadow object scope/pose/phase unavailable";
  if (!scope || !scope->shadow_phase || scope->render_view != 1 || scope->pose.get() != &pose ||
      scope->model != pose.model || !scope->policy_inputs || node >= pose.transforms.size()) return {};
  refusal = "shadow object texture mode unavailable";
  if (!scope->shadow_inputs) return {};
  refusal = "shadow owned node/admission unavailable";
  const auto *program = FindNativeInstanceNode(pose, node);
  if (!program) return {};
  const auto admission = PrepareNativeRigidShadowAdmission(*program, scope->policy_inputs);
  if (admission.route != NativeRigidCasterRoute::Native) return {};
  const auto *mesh = PrepareMaterialMesh(*program);
  refusal = "shadow owned texture recipe unavailable";
  if (!mesh || mesh->values.size() != program->ranges.size()) return {};
  std::vector<NativeRigidShadowCutout> cutouts(program->ranges.size());
  for (size_t n = 0; n < cutouts.size(); ++n) if (admission.policies[n].alpha_test) {
    auto &cutout = cutouts[n];
    const auto &range = program->ranges[n];
    const uint32_t layers = range.shadow_uses_texture ? scope->shadow_inputs->texture_layers : 0;
    // The direct phase1 callback selects shadownull; the list callback selects
    // shadowmap. Neither consumes scene colour, vertex alpha or cutoff state.
    cutout.textured = layers != 0 && admission.policies[n].deferred;
    cutout.uv = mesh->values[n].uv; cutout.owns_uv = mesh->values[n].owns_uv;
    if (cutout.textured) {
      refusal = "shadow base image assignment unavailable";
      if (!(mesh->values[n].image_mask & 1)) return {};
      refusal = "shadow pass sampler filters unavailable";
      if (!scope->shadow_filters || !(*scope->shadow_filters)[0]) return {};
      cutout.image = mesh->values[n].images[0];
      cutout.sampler = NativeMaterialSampler2D{*(*scope->shadow_filters)[0],
          MaterialSampleAddress::Wrap, MaterialSampleAddress::Wrap};
    }
  }
  refusal = "shadow canonical geometry, matrices or sampled image contract unavailable";
  auto plans = PrepareNativeRigidShadow(*program, pose.transforms[node], *scope->policy_inputs, camera, cutouts);
  if (!plans) for (size_t n=0;n<std::min<size_t>(8,cutouts.size());++n) {
    const auto &geometry = program->geometries[n];
    const auto &cutout = cutouts[n];
    BD_ERROR("[native-shadow-input] primitive {} geometry {:016X} canonical {} input {} stream mask {} stride {} count {}; textured {} image {} UV {} sampler {}",
        n, geometry ? geometry->id : 0, geometry && geometry->canonical_vertices, geometry && geometry->rigid_vertex_input,
        geometry ? geometry->stream_mask : 0, geometry ? geometry->strides[0] : 0, geometry ? geometry->count : 0,
        cutout.textured, bool(cutout.image.primary), cutout.owns_uv, cutout.sampler.has_value());
  }
  return plans;
}

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
  if (!scope || scope->shadow_phase || tag.from_list || tag.ctx_va != scope->context || tag.visual_va != scope->visual) return {};
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
