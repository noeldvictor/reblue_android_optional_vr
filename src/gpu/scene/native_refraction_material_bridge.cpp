/**
 * @brief Whole water/refraction setup replacements with counted import adapters.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_refraction_material.h"
#include "gpu/scene/native_instance_bridge.h"
#include "gpu/scene/native_water_material_bridge.h"
#include "gpu/scene/native_water_material_source.h"
#include "gpu/scene/native_deferred_contract.h"
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_material_texture_source.h"
#include "gpu/scene/native_lighting_bridge.h"
#include "gpu/scene/native_fog_bridge.h"
#include "gpu/scene/native_shadow_receiver_bridge.h"
#include "gpu/scene/native_water_bottom.h"
#include "gpu/scene/native_reflection_pass.h"
#include "gpu/scene/native_scene_snapshot.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/native_texture_binding_bridge.h"
#include "gpu/scene/native_alpha_bridge.h"
#include "gpu/scene/native_blend_bridge.h"
#include "gpu/scene/native_rigid_draw.h"
#include "gpu/scene/native_transform_bridge.h"
#include "gpu/scene/deferred_surface.h"
#include "gpu/scene/refraction_material_import.h"
#include "gpu/scene/shader_parameter_import.h"
#include "gpu/native_texture_mirror.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_82454720);
REX_EXTERN(__imp__sub_82455150);
REX_EXTERN(bdShaderConstantFlush);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(bdSetSamplerState);
REX_EXTERN(sub_8221D2C8);
REXCVAR_DECLARE(bool, bd_native_scene_textures);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kPhase = (uint32_t(-32137) << 16) + 16476;
constexpr uint32_t kSettings = (uint32_t(-32035) << 16) - 26552;
constexpr uint32_t kDefaultFactor = (uint32_t(-32251) << 16) + 21040;
constexpr uint32_t kPlanarImage = (uint32_t(-32035) << 16) + 29040 + 12;
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
struct Stats {
  uint64_t water = 0, refraction = 0, compatibility = 0, refused = 0, faults = 0;
  uint64_t parameters = 0, state_adapters = 0, bindings = 0, null_bindings = 0, snapshots = 0, clamped = 0;
  uint64_t debug_bindings = 0;
  uint64_t water_candidates = 0, water_consumed = 0, water_unavailable = 0;
  uint64_t direct_materials = 0, direct_ends = 0;
  uint32_t water_refusal_frame = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-refraction-material] water {} refraction {} compatibility {} refused {} faults {}; "
          "parameter adapters {} state adapters {} image bindings {} null no-ops {} snapshots {} clamps {} debug images {}; "
          "host setup/order, authored fields/descriptors, state shadows, getters and snapshot scope adapters remain",
      stats.water, stats.refraction, stats.compatibility, stats.refused, stats.faults,
      stats.parameters, stats.state_adapters, stats.bindings, stats.null_bindings, stats.snapshots, stats.clamped,
      stats.debug_bindings);
  stats.frame = frame;
  stats.reported = true;
  if (stats.water_candidates)
    BD_INFO("[native-water-admission] candidates {} consumed {} unavailable {}; sorted source/visual adapters remain",
        stats.water_candidates,stats.water_consumed,stats.water_unavailable);
  if (stats.direct_materials)
    BD_INFO("[native-water-material] direct begin {} end {}; no model/resource callbacks or translated shader selection; outgoing state/parameter adapters remain",
        stats.direct_materials,stats.direct_ends);
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address > UINT32_MAX || bytes > UINT32_MAX ||
      address + bytes - 1 > UINT32_MAX || !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) { return !(address & 3) && Range(address, bytes); }
void Check(bool valid) {
  if (!valid) { ++stats.faults; throw std::runtime_error("Native refraction material lost a validated import"); }
}
std::optional<uint32_t> Word(uint64_t address) {
  return Words(address, 4) ? std::optional(bd::mem::load<uint32_t>(uint32_t(address))) : std::nullopt;
}
uint32_t ReadWord(uint64_t address) { const auto value = Word(address); Check(bool(value)); return *value; }
float ReadFloat(uint64_t address) { Check(Words(address, 4)); return bd::mem::load<float>(uint32_t(address)); }
bool Overlap(uint64_t a, uint64_t bytes, uint64_t scratch) { return a < scratch + 1120 && scratch < a + bytes; }
bool DescriptorReady(uint32_t descriptor, uint64_t scratch) {
  if (!Words(descriptor, 16) || Overlap(descriptor, 16, scratch)) return false;
  const auto flags = ReadWord(descriptor) & 3;
  if (!flags) return true;
  const auto first = ReadWord(uint64_t(descriptor) + 4);
  const auto count = ReadWord(uint64_t(descriptor) + 8) - first;
  const auto source = ReadWord(uint64_t(descriptor) + 12);
  return ParameterRangeSupported(first, count) &&
      (!count || (Range(source, uint64_t(count) * 16) && !Overlap(source, uint64_t(count) * 16, scratch)));
}
bool Ready(PPCContext &ctx, bool water) {
  const uint32_t material = ctx.r3.u32;
  const uint64_t scratch = uint64_t(ctx.r1.u32) - 1024;
  if (ctx.r1.u32 < 1024 || (ctx.r1.u32 & 15) || !Words(scratch, 1120) ||
      !Words(material, water ? 5060 : 4972) || Overlap(material, water ? 5060 : 4972, scratch)) return false;
  if (!water) return DescriptorReady(ReadWord(uint64_t(material) + 4968), scratch);
  if (!Word(kPhase) || !Word(kDefaultFactor) || !Word(kPlanarImage) || !Word(kDevice)) return false;
  if (ReadWord(kPhase) == 3) {
    const auto settings = Word(kSettings);
    if (!settings || !*settings || !Word(uint64_t(*settings) + 7020)) return false;
  }
  const auto read = [scratch](uint64_t address) {
    return Overlap(address, 4, scratch) ? std::nullopt : Word(address);
  };
  const auto destination = ReadWaterFactorDestination(material, read);
  return destination && Words(*destination, 4) && !Overlap(*destination, 4, scratch) &&
      DescriptorReady(ReadWord(uint64_t(material) + 4760), scratch) &&
      DescriptorReady(ReadWord(uint64_t(material) + 4952), scratch) && bool(ReadWaterSceneImage(read));
}
struct Adapter {
  PPCContext &ctx;
  uint8_t *base;
  const uint32_t material;
  const uint64_t saved_stack;
  const bool water;
  const bool capture;
  NativeWaterMaterialOutput output;
  Adapter(PPCContext &context, uint8_t *memory, bool is_water, bool capture_output = false)
      : ctx(context), base(memory), material(ctx.r3.u32), saved_stack(ctx.r1.u64), water(is_water), capture(capture_output) {
    ctx.r1.u32 -= 256;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
    ctx.fpscr.disableFlushMode();
  }
  ~Adapter() { ctx.r1.u64 = saved_stack; }
  void PublishSceneFactor() {
    const bool scene = ReadWord(kPhase) == 3;
    const bool force = scene && ReadWord(uint64_t(ReadWord(kSettings)) + 7020) != 0;
    const int32_t authored = int32_t(ReadWord(uint64_t(material) + 4708));
    const auto destination = ReadWaterFactorDestination(material, Word);
    Check(destination && Words(*destination, 4));
    bd::mem::store<float>(*destination, WaterSceneFactor(scene, force, authored, ReadFloat(kDefaultFactor)));
  }
  void Flush(uint32_t offset, bool clamp) {
    const uint32_t descriptor = ReadWord(uint64_t(material) + offset);
    Check(DescriptorReady(descriptor, uint64_t(uint32_t(saved_stack)) - 1024));
    if (clamp && (ReadWord(descriptor) & 2)) {
      const uint32_t first = ReadWord(uint64_t(descriptor) + 4), end = ReadWord(uint64_t(descriptor) + 8);
      const uint32_t data = ReadWord(uint64_t(descriptor) + 12);
      if (first <= 51 && end > 51 && data) {
        const uint64_t address = uint64_t(data) + (51 - first) * 16 + 12;
        const float before = ReadFloat(address), after = ClampWaterHighlight(before);
        if (before > 1.f) { bd::mem::store<float>(uint32_t(address), after); ++stats.clamped; }
      }
    }
    ctx.r3.u64 = descriptor;
    ++stats.parameters;
    bdShaderConstantFlush(ctx, base); // existing host parameter producer / counted descriptor adapter
  }
  void FlushWaterParameters(uint32_t index) { Flush(index ? 4952 : 4760, true); }
  void FlushRefractionParameters() { Flush(4968, false); }
  void State(uint32_t offset, uint32_t value) {
    ctx.r3.u64 = offset; ctx.r4.u64 = value;
    ++stats.state_adapters;
    bdSetRenderState(ctx, base); // existing native intent producer, temporary cache/getter shadow
  }
  void EnableSourceAlphaBlending() { State(60, 1); State(72, 6); State(76, 7); }
  void EnableDepthTest() { State(40, 1); } // no depth-write override, separate alpha or blend-op reset
  GuestTexture *Bind(uint32_t slot, uint32_t address) {
    GuestTexture *image = nullptr;
    if (address) {
      image = ResolveGuestTexture(address); // can wait for IO, never under the video mutex
      if (!image) {
        image = GetOrCreateDebugTexture(); // preserve the existing unsupported-image marker, never replay setup
        ++stats.debug_bindings;
      }
      Check(image != nullptr);
      Video::SetTexture(slot, image);
      ++stats.bindings;
    } else ++stats.null_bindings;
    ctx.r3.u64 = ReadWord(kDevice); // temporary void-callback register convention
    return image;
  }
  void BindPlanarReflection() {
    const auto address = ReadWord(kPlanarImage);
    const auto *outgoing = Bind(7, address);
    if (capture) {
      output.planar = FindCompletedNativeWaterReflection();
      // Late descriptor/state writes can alias the outgoing getter. Validate
      // that mirror, but never select native input from it or guess inheritance.
      Check(output.planar && outgoing && outgoing->nativeImage == output.planar);
    }
  }
  void BindSceneImage() {
    const auto image = ReadWaterSceneImage(Word);
    Check(bool(image)); Bind(12, *image);
    if (capture && *image) output.bump = CaptureNativeTexture(ResolveGuestTexture(*image)).primary;
  }
  bool WantsSnapshot() { return int32_t(ReadWord(uint64_t(material) + 4700)) > 0; }
  void Snapshot() {
    ++stats.snapshots;
    if (water && capture) {
      // Native request -> exact producer result. Authored timing and outgoing
      // getter/slot publication remain inside that producer for mixed consumers.
      Check(ProduceNativeSceneSnapshot(material,output.snapshot) && bool(output.snapshot));
      return;
    }
    ctx.r3.u64 = uint64_t(material) + (water ? 4648 : 4932);
    ctx.r4.u64 = material;
    sub_8221D2C8(ctx, base); // native snapshot producer; unowned scopes remain tracked there
  }
};
void Prepare(PPCContext &ctx, uint8_t *base, bool water) {
  const bool enabled = REXCVAR_GET(bd_native_scene_textures);
  if (!enabled || !Ready(ctx, water)) {
    ++stats.compatibility; stats.refused += enabled;
    if (water) __imp__sub_82454720(ctx, base); else __imp__sub_82455150(ctx, base);
    Report(); return;
  }
  Adapter adapter(ctx, base, water);
  if (water) {
    PrepareWaterMaterial(adapter); ++stats.water;
  }
  else { PrepareRefractionMaterial(adapter); ++stats.refraction; }
  // No fallback/replay after the first material or GPU side effect.
  Report();
}
} // namespace

NativeWaterMaterialScope::NativeWaterMaterialScope(uint32_t entry, uint32_t visual, NativeVisualIdentity identity) {
  if (!entry || !identity || !NativeRigidSceneEnabled() || !IsDeferredWaterResource(visual, Word)) return;
  entry_ = entry; visual_ = visual; identity_ = identity; frame_ = FrameStatFrameCount();
  ++stats.water_candidates;
}
NativeWaterMaterialScope::NativeWaterMaterialScope(NativeWaterDeferred pending) : pending_(std::move(pending)) {
  Check(pending_->Valid(FrameStatFrameCount()) && pending_->bridge);
  identity_ = pending_->Identity(); frame_ = pending_->frame; visual_ = pending_->bridge->visual;
  Check(FindNativeVisualIdentity(visual_) == identity_);
  ++stats.water_candidates;
}
bool NativeWaterMaterialScope::Draw(uint32_t stack, uint8_t *base, bool stencil_pending, int32_t &depth_write) {
  if (!identity_) return false;
  PPCContext context{};
  context.r1.u64 = stack; context.r3.u64 = visual_;
  constexpr uint32_t engine = (uint32_t(-32034) << 16) - 19936;
  const auto technique = pending_ ? Word(uint64_t(visual_)+3000) : Word(uint64_t(entry_)+248);
  const auto unavailable = [&] {
    ++stats.water_unavailable;
    if (stats.water_unavailable == 1 || FrameStatFrameCount()-stats.water_refusal_frame >= 300) {
      BD_INFO("[native-water-admission] unavailable before material: technique {} instance {} generation {} frame {}",
          technique.value_or(~0u),identity_.instance,identity_.model_generation,frame_);
      stats.water_refusal_frame = FrameStatFrameCount();
    }
    return false;
  };
  if (!REXCVAR_GET(bd_native_scene_textures) || stencil_pending || Word(kPhase) != 3 ||
      (!pending_ && !Range(entry_,816)) || !Word(engine+16) || !technique ||
      !CheckNativeWaterModelContract(visual_,*technique,Word) || !Ready(context,true))
    return unavailable();
  const auto lighting = FindNativeLightingPass(3);
  const auto node = pending_ ? pending_->node : ReadWord(uint64_t(entry_)+252);
  const auto recipe = pending_ ? std::optional(pending_->lights) :
      lighting ? CaptureNativeSceneLights(identity_.instance,identity_.model_generation,node,lighting->inputs) : std::nullopt;
  const auto ticket = recipe ? ResolveNativeSceneLights(*recipe) : std::nullopt;
  if (!ticket) return unavailable();
  lights_ = ticket->lights; // copied once, not resolved/committed again after the writer
  struct DirectAdapter : Adapter {
    NativeWaterMaterialScope &scope;
    const NativeSceneLightTicket &ticket;
    int32_t &depth_write;
    DirectAdapter(PPCContext &ctx, uint8_t *memory, NativeWaterMaterialScope &owner,
                  const NativeSceneLightTicket &lights, int32_t &depth)
        : Adapter(ctx,memory,true,true), scope(owner), ticket(lights), depth_write(depth) {}
    bool BeginLights() { return CommitNativeSceneLights(ticket,uint32_t(saved_stack)); }
    void PublishModelFlags() {
      // Re-read after the light exporter, which can alias source parameters.
      Check(CheckNativeWaterModelContract(material,scope.pending_ ? ReadWord(uint64_t(material)+3000) : ReadWord(uint64_t(scope.entry_)+248),Word));
      bd::mem::store<uint32_t>(engine+16,scope.pending_ ? 0 : ReadWord(uint64_t(scope.entry_)+240));
      PPCContext preflight{};
      preflight.r3.u64 = material; preflight.r1.u64 = saved_stack;
      Check(Ready(preflight,true)); // never fall back after the first light publication
    }
    void PublishWaterOutput() {
      const auto values = ReadNativeWaterMaterial(material,Word);
      Check(bool(values)); output.material = *values;
      scope.Publish(material,std::move(output));
      RefreshNativeVisualInputsAfterWriter();
      ++stats.water; ++stats.direct_materials;
    }
    void SubmitWater() {
      if (!scope.Submit(false)) throw std::runtime_error("Native water lost an owned input after material participation");
    }
    void ExportDepthIntent() {
      const bool force_off = ReadWord(uint64_t(material)+3000) == 8;
      if (force_off && !scope.pending_) bd::mem::store<uint8_t>(scope.entry_+295,0);
      const int32_t next = scope.pending_ ? (!force_off && scope.pending_->shadow_allowed) : bd::mem::load<int8_t>(scope.entry_+295);
      if (next != depth_write) { depth_write = next; State(48,uint32_t(next)); }
    }
    void FinishWater() {
      // Original resource end is only these unbindings. Native packets already
      // retain their own leases; registry active bytes/resource stay idle.
      Video::SetTexture(7,nullptr); Video::SetTexture(12,nullptr);
      if (WantsSnapshot()) Video::SetTexture(13,nullptr);
      ++stats.direct_ends;
    }
  } adapter(context,base,*this,*ticket,depth_write);
  if (pending_) {
    Check(pending_->Valid(frame_) && FindNativeVisualIdentity(visual_) == identity_);
    const auto &outgoing = *pending_->bridge;
    for (uint32_t slot=0;slot<6;++slot) if (outgoing.texture_sources[slot]) {
      auto *texture = ResolveGuestTexture(outgoing.texture_sources[slot]);
      Check(texture && CaptureNativeTexture(texture) == outgoing.image_leases[slot]);
      for (uint32_t axis=0;axis<2;++axis) {
        context.r3.u64 = slot; context.r4.u64 = axis*4; context.r5.u64 = outgoing.addresses[slot][axis];
        bdSetSamplerState(context,base); // outgoing state only; native water owns its six samplers
      }
      Video::SetTexture(slot,texture);
    }
    adapter.State(100,pending_->alpha_reference);
    Check(PublishNativeWorld(pending_->pose->transforms[pending_->node]));
    constexpr uint32_t object_mode = (uint32_t(-32036)<<16)-5536;
    const auto cull = pending_->cull;
    bd::mem::store<uint8_t>(object_mode,outgoing.object_mode);
    adapter.State(56,cull == PrimitiveCull::Back ? 6 : cull == PrimitiveCull::Front ? 2 : 0);
  }
  const bool consumed = ConsumeNativeWaterMaterial(adapter);
  if (!consumed) return unavailable();
  Report();
  return true;
}
void NativeWaterMaterialScope::Publish(uint32_t visual, std::optional<NativeWaterMaterialOutput> output) {
  publication_.Reset();
  if (visual != visual_ || frame_ != FrameStatFrameCount() || !output) return;
  // This is the original sorted image producer's explicit slot5 selection,
  // copied after the resource writer's possible aliases. Null means unowned
  // inheritance, not permission to borrow the previous draw's cube.
  const auto source = pending_ ? std::optional<uint32_t>{} : Word(uint64_t(entry_) + 372);
  if (pending_) output->environment = pending_->environment;
  else if (source && *source) {
    const auto image = CaptureNativeTexture(ResolveGuestTexture(*source));
    output->environment = image.cube ? image.cube : image.primary;
  }
  const auto object = ReadMaterialObjectInputs(visual_,Word);
  Check(bool(object)); output->object = *object;
  publication_.Publish(identity_, frame_, std::move(*output));
}
bool NativeWaterMaterialScope::Submit(bool stencil_pending) {
  if (!identity_) return false;
  const auto entry = entry_;
  const auto refuse = [&](const char *reason) {
    ++stats.water_unavailable;
    if (stats.water_unavailable == 1 || FrameStatFrameCount()-stats.water_refusal_frame >= 300) {
      BD_INFO("[native-water-admission] unavailable: {}; instance {} generation {} frame {}",
          reason,identity_.instance,identity_.model_generation,frame_);
      stats.water_refusal_frame = FrameStatFrameCount();
    }
    return false;
  };
  const auto output = publication_.Read(identity_, FrameStatFrameCount());
  if (!output || stencil_pending || Word(kPhase) != 3) return refuse("completed material or scene pass");
  std::shared_ptr<const NativeInstancePose> pose;
  std::shared_ptr<const ModelMaterialImport> mesh;
  const NativeModelMaterialProgram *owned_program = nullptr;
  uint32_t node = 0;
  std::optional<size_t> primitive;
  if (pending_) {
    if (!pending_->Valid(frame_) || pending_->Identity() != identity_) return refuse("retained native water ownership");
    pose = pending_->pose; node = pending_->node; primitive = pending_->primitive;
    owned_program = pending_->Program();
  } else {
  if (entry != entry_ ||
      Word(uint64_t(entry)+244) != visual_ || Word(uint64_t(entry)+272) != visual_ ||
      !Range(entry,816) || bd::mem::load<int8_t>(entry+289) > 0 ||
      bd::mem::load<int8_t>(entry+292) != 0) return refuse("completed material or regular sorted surface");
  const auto graph = Word(uint64_t(visual_)+2620), palette = Word(uint64_t(entry)+268);
  node = ReadWord(uint64_t(entry)+252);
  if (!graph || !*graph || !palette || !*palette) return refuse("model/palette import");
  pose = FindNativeInstancePose(visual_,*graph,*palette);
  mesh = FindLoadedNativeModelNodeImport(*graph,node);
  if (!pose || !mesh || pose->instance != identity_.instance || pose->model_generation != identity_.model_generation ||
      node >= pose->transforms.size() || FindNativeInstanceNode(*pose,node) != &mesh->program) return refuse("native pose/model node owner");
  // The entry is still an import adapter, never the native transform owner.
  // Reject missed pose publication rather than freezing its copied source matrix.
  for (uint32_t i=0; i<16; ++i)
    if (pose->transforms[node][i] != ReadFloat(uint64_t(entry)+16+i*4)) return refuse("sorted world differs from completed native pose");
  const auto &program = mesh->program;
  for (size_t i=0; i<program.ranges.size(); ++i) {
    if (!ModelPrimitiveMatches(program.ranges[i],mesh->source_bindings[i],ReadWord(uint64_t(entry)+384),
        ReadWord(uint64_t(entry)+380),bd::mem::load<uint16_t>(entry+284),uint32_t(bd::mem::load<uint16_t>(entry+280))+2)) continue;
    if (primitive) return refuse("ambiguous sorted primitive");
    primitive = i;
  }
  if (!primitive) return refuse("load-owned primitive association");
  owned_program = &mesh->program;
  }
  const auto &program = *owned_program;
  const auto &geometry = program.geometries[*primitive];
  if (!geometry || !geometry->id || !geometry->canonical_vertices || !geometry->water_vertex_input ||
      geometry->stream_mask != 1 || !geometry->strides[0] || geometry->strides[0] > 255 ||
      !geometry->streams[0].buffer.ref || !geometry->index.buffer.ref || !geometry->count ||
      program.ranges[*primitive].skin) return refuse("canonical water geometry/tangent or skin");
  const auto camera = FindNativePassCamera(3);
  const auto lighting = FindNativeLightingPass(3);
  const auto fog = FindNativeFogLayers();
  const auto receiver = FindNativePrimaryReceiver(identity_,3);
  const auto alpha = FindNativeAlphaIntent();
  const auto blend = FindNativeEnabledBlendIntent();
  if (!camera || !lighting || !fog || !receiver || !alpha || !blend) return refuse("native camera/lighting/fog/receiver/alpha/blend");
  const auto features = ComposeNativeMaterialFeatures(program.ranges[*primitive].features,output->object,
      lighting->inputs,program.ranges[*primitive].reflection.enabled);
  if (!features) return refuse("owned material feature recipe");
  NativeWaterImages images;
  images.bump = output->bump; images.environment = output->environment;
  images.planar = output->planar; images.snapshot = output->snapshot;
  images.shadow = NativeImageLease::From(receiver->image);
  const auto bottom = FindCompletedNativeWaterBottom();
  if (output->material.shore && !bottom) return refuse("authored shore needs completed bottom depth/projection");
  // Legal unused descriptors are explicit leases. They are never sampled for
  // disabled features, and must still pass the active-attachment exclusion.
  images.bottom = bottom ? bottom->image : images.shadow;
  if (!output->material.refraction) images.snapshot = images.planar;
  if (output->material.reflection != WaterReflectionEnvironment) {
    static const NativeTextureHandle unused_cube = [] {
      NativeTextureData data{NativeTextureFormat::RGBA8,NativeTextureDimension::Cube,1,1,1,1,
          std::vector<std::vector<uint8_t>>(6,std::vector<uint8_t>{0,0,0,255})};
      std::vector<uint8_t> encoded;
      if (!EncodeNativeTexture(data,encoded)) return NativeTextureHandle{};
      return NativeTextureHandle(std::make_shared<NativeTextureAsset>(NativeTextureContentId(encoded),std::move(data)));
    }();
    images.environment = AcquireNativeTextureGpu(unused_cube); // existing bounded GPU asset store, no disk cache
  }
  NativeRigidPassInputs pass;
  const auto vector = [](const LightingVector &v) { return RigidFloat4{v[0],v[1],v[2],v[3]}; };
  pass.world_to_clip = {camera->world_to_clip,camera->world_to_clip};
  pass.world_to_shadow = receiver->world_to_shadow;
  pass.cameras = {vector(lighting->inputs.camera_position),vector(lighting->inputs.camera_position)};
  pass.ambient = vector(lighting->inputs.ambient); pass.colour_grade = vector(lighting->inputs.color_scale);
  pass.shadow_colour_strength = vector(receiver->colour);
  pass.shadow_filter = {lighting->inputs.shadow_bias,.4f*lighting->inputs.shadow_bias,
      .65f/float(receiver->image->shape.width),0};
  pass.fog = *fog;
  pass.lights = lights_;
  const auto receive = pending_ ? uint8_t(pending_->shadow_allowed) : bd::mem::load<uint8_t>(entry+295);
  if (receive > 1 || program.shadow_policies[*primitive] == NativeShadowPolicy::Unknown) return refuse("owned shadow participation");
  const uint32_t flags = (features->diffuse ? WaterDiffuse : 0) | (features->fog ? WaterFog : 0) |
      (receive && program.shadow_policies[*primitive] == NativeShadowPolicy::Receive ? WaterShadow : 0);
  const auto projection = bottom ? bottom->world_to_bottom : RenderMatrix{};
  const std::array<uint32_t,3> layers{images.planar.image.layers,images.snapshot.image.layers,images.bottom.image.layers};
  // Staged work keeps its frame's exact pose even after a later source handoff.
  // The remaining sorted-entry adapter compares raw values BEFORE resolving
  // render time. Neither route exports blended matrices to legacy consumers.
  const auto render_pose = pending_ ? pending_->render_pose : ResolveNativeRenderPose(*pose);
  if (!render_pose || node >= render_pose->transforms.size()) return refuse("native water render pose");
  auto input = BuildNativeWaterInstance(render_pose->transforms[node],output->material,pass,{projection,projection},layers,flags);
  if (!input || !SetNativeWaterCutout(*input,alpha->enabled,uint32_t(alpha->compare),alpha->threshold) ||
      !images.Ready(input->image_layers) || !NativeWaterWorldBounds(*geometry,*input)) return refuse("water GPU values/image leases/wave bounds");
  NativeWaterScenePlan plan;
  plan.geometry = geometry; plan.input = *input; plan.images = std::move(images); plan.blend = *blend;
  plan.depth_write = ReadWord(uint64_t(visual_)+3000) == 8 ? false : bool(receive);
  plan.alpha_to_coverage = alpha->alpha_to_coverage;
  if (pending_) {
    plan.cull = pending_->cull == PrimitiveCull::Back ? plume::RenderCullMode::BACK :
        pending_->cull == PrimitiveCull::Front ? plume::RenderCullMode::FRONT : plume::RenderCullMode::NONE;
  } else {
  const auto winding = bd::mem::load<uint16_t>(entry+286);
  if (winding != 0x1000 && winding != 0x2000 && winding != 0x3000) return refuse("explicit winding policy");
  const auto face = DeferredFaces(winding == 0x2000,winding == 0x3000 ? 2 : bd::mem::load<uint8_t>(entry+288));
  plan.cull = face == DeferredCullFace::Back ? plume::RenderCullMode::BACK :
      face == DeferredCullFace::Front ? plume::RenderCullMode::FRONT : plume::RenderCullMode::NONE;
  }
  // Explicit native water sampling policy: repeating animated normals, clamped
  // screen/depth/cube inputs and the existing comparison sun filter. No fetch bits.
  for (uint32_t role=0; role<6; ++role) {
    auto &sampler = plan.samplers[role];
    sampler.addressU = sampler.addressV = sampler.addressW = role ? plume::RenderTextureAddressMode::CLAMP : plume::RenderTextureAddressMode::WRAP;
    sampler.minFilter = sampler.magFilter = plume::RenderFilter::LINEAR;
    sampler.mipmapMode = plume::RenderMipmapMode::LINEAR;
    if (role == 3) { // D32 shoreline depth need not support ordinary linear filtering.
      sampler.minFilter = sampler.magFilter = plume::RenderFilter::NEAREST;
      sampler.mipmapMode = plume::RenderMipmapMode::NEAREST;
    }
    sampler.comparisonEnabled = role == 5;
    sampler.comparisonFunc = plume::RenderComparisonFunction::LESS_EQUAL;
  }
  {
    auto &s = state(); std::lock_guard lock(s.mutex);
    auto *commands = ActiveNativeSceneCommands(s.render_target ? s.render_target->texture : nullptr,
        s.depth_stencil ? s.depth_stencil->texture : nullptr);
    if (!s.ready || !s.device || !s.command_list_open || !commands || !commands->ColorShape() ||
        commands->ColorShape()->layers != 1 || !commands->Camera(frame_,3)) return refuse("native mono command scope");
    for (uint32_t role=0; role<6; ++role)
      if (commands->WritesImage(plan.images.Image(role))) return refuse("sampled water image aliases active attachment");
  }
  NativeWaterSceneSubmission submission{identity_.instance,identity_.model_generation,frame_};
  submission.plans.push_back(std::move(plan));
  if (!SubmitNativeWaterScenePackets(std::move(submission))) throw std::runtime_error("Native water packet submission failed");
  ++stats.water_consumed;
  return true;
}
} // namespace bd::gpu::scene
REX_HOOK_RAW(sub_82454720) {
  bd::gpu::scene::Prepare(ctx, base, true);
  bd::gpu::scene::RefreshNativeVisualInputsAfterWriter();
}
REX_HOOK_RAW(sub_82455150) {
  bd::gpu::scene::Prepare(ctx, base, false);
  bd::gpu::scene::RefreshNativeVisualInputsAfterWriter();
}
