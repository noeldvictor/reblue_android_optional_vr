/**
 * @brief Completed motion loads -> owned keyed channels -> native skeleton poses.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_animation_source.h"
#include "gpu/scene/native_animation_controller_source.h"
#include "gpu/scene/native_animation_bridge.h"
#include "gpu/scene/native_skeleton_source.h"
#include "gpu/scene/native_material.h"
#include "gpu/scene/native_model_materials.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include <mutex>
#include <stdexcept>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_BOOL(bd_native_animation, false, kCvarGroup,
    "Load-owned indexed/named animation sampling for ordinary native skeletons; pending desktop qualification.");
REXCVAR_DECLARE(bool, bd_native_instances);
REXCVAR_DECLARE(bool, bd_native_skeleton);
REXCVAR_DECLARE(bool, bd_native_materials_verify);
REX_EXTERN(__imp__sub_8217BD70);
REX_EXTERN(__imp__sub_8217C5E8);
REX_EXTERN(__imp__sub_82288680);
REX_EXTERN(__imp__sub_8217BD00);
REX_EXTERN(__imp__sub_8217C580);
REX_EXTERN(__imp__bdVisualObjectAnimSlotUpdate);
REX_EXTERN(__imp__bdAnimationUpdate);
REX_EXTERN(__imp__sub_822D3CB0);
REX_EXTERN(__imp__sub_82289888);
REX_EXTERN(__imp__sub_8228A3E8);
REX_EXTERN(__imp__sub_82284BE0);
REX_EXTERN(bdVisualObjectCollisionTestNearby);
REX_EXTERN(bdEffectUpdate);

namespace bd::gpu::scene::animation_bridge {
namespace {
constexpr uint32_t kSamplerState = 0x82DC99E0;
enum class Missing : size_t { Model, Asset, Restrictions, Output, Sampling, Weight, Subtree, Count };
constexpr std::array missing_names{"model", "asset", "restrictions", "output", "sampling", "weight", "subtree"};
struct Store {
  std::mutex mutex;
  NativeAnimationResidency assets;
  uint64_t loaded = 0, load_refused = 0, sampled = 0, whole = 0, preserved = 0, checks = 0, wrong = 0;
  uint64_t cubic = 0, registered = 0, prepare_refused = 0, weighted = 0, subtree = 0;
  uint64_t mixed = 0, mix_checks = 0, filtered = 0, canonicalized = 0, indexed = 0;
  uint64_t controllers=0, controller_checks=0, controller_refused=0, controller_samples=0, controller_mixes=0;
  uint64_t controller_interior=0, controller_subtree=0, channel_handoffs=0, channel_changed=0;
  uint64_t controller_advancing=0;
  uint64_t late_controllers=0, late_checks=0, late_samples=0, late_reused=0, late_handoffs=0;
  uint64_t slot_publications=0, slot_reused=0, late_refused=0;
  std::array<uint64_t,size_t(Missing::Count)> missing{};
  uint32_t frame = 0;
};
Store &Clips() { static Store store; return store; }
bool Enabled() {
  return REXCVAR_GET(bd_native_animation) && REXCVAR_GET(bd_native_instances) && REXCVAR_GET(bd_native_skeleton);
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address > UINT32_MAX || bytes > uint64_t(UINT32_MAX)+1-address ||
      !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page=(address & ~uint64_t(4095))+4096; page<address+bytes; page+=4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
std::optional<uint32_t> Word(uint64_t address) {
  if ((address & 3) || !Range(address,4)) return {};
  return bd::mem::load<uint32_t>(uint32_t(address));
}
void Report(Store &store) {
  const auto frame = FrameStatFrameCount();
  if (frame-store.frame < 300) return;
  uint64_t unavailable = 0; for (auto count : store.missing) unavailable += count;
  BD_INFO("[native-animation] frame {} loads {} refused {} resident {} bytes {}; sampled {} whole {} preserved {} unavailable {}; checked {} wrong {}; cubic {}; registered {} catalog {} prepare-refused {}; weighted {} subtree {}; mixed {} mix-checked {}; filtered {} indexed {}; owned keys/layers, original slot clocks and outgoing channel adapter remain",
      frame,store.loaded,store.load_refused,store.assets.ResidentCount(),store.assets.Bytes(),
      store.sampled,store.whole,store.preserved,unavailable,store.checks,store.wrong,store.cubic,
      store.registered,store.assets.Size(),store.prepare_refused,store.weighted,store.subtree,store.mixed,store.mix_checks,store.filtered,store.indexed);
  store.frame = frame;
  if (store.controllers || store.controller_refused)
    BD_INFO("[native-animation-controller] frame {} completed {} checked {} refused {}; samples {} mixes {} subtree {} interior {}; handoffs {} changed {}; advancing {}; native clocks/layers, one outgoing boundary; late-writer validation remains",
        frame,store.controllers,store.controller_checks,store.controller_refused,store.controller_samples,store.controller_mixes,
        store.controller_subtree,store.controller_interior,store.channel_handoffs,store.channel_changed,store.controller_advancing);
  if (store.late_controllers || store.late_refused || store.slot_publications)
    BD_INFO("[native-animation-late] frame {} completed {} checked {} refused {}; samples {} reused {} handoffs {}; slot-published {} slot-reused {}; native layers continue to skeleton; untracked-write guard retained",
        frame,store.late_controllers,store.late_checks,store.late_refused,store.late_samples,
        store.late_reused,store.late_handoffs,store.slot_publications,store.slot_reused);
}
bool Unavailable(Missing reason) {
  auto &store = Clips(); std::lock_guard lock(store.mutex);
  if (++store.missing[size_t(reason)] <= 2)
    BD_INFO("[native-animation-unavailable] {}; original whole sampling call retained",missing_names[size_t(reason)]);
  Report(store); return false;
}
thread_local uint32_t loading_owner = 0;
struct LoadScope {
  uint32_t previous;
  explicit LoadScope(uint32_t owner) : previous(loading_owner) { loading_owner = owner; }
  ~LoadScope() { loading_owner = previous; }
};
void Retire(uint32_t owner) {
  auto &store = Clips(); std::lock_guard lock(store.mutex); store.assets.Retire(owner);
}
void Import(uint32_t source) {
  auto &store = Clips(); std::lock_guard lock(store.mutex);
  // Called only after initialization inside one of the two completed loaders.
  // Retire stale lookup before import, including unsupported source-address reuse.
  store.assets.Invalidate(source);
  if (!loading_owner || !Enabled()) return;
  const auto type = animation_source::Half(uint64_t(source)+6,Word);
  const bool published = type && *type <= 3 && store.assets.Register(loading_owner,source);
  ++(published ? store.registered : store.load_refused);
  if (!published && store.load_refused <= 4) {
    BD_INFO("[native-animation-load-refused] type {}; original motion remains available",type ? int(*type) : -1);
  }
}
thread_local uint32_t slot_graph = 0;
thread_local uint32_t slot_visual = 0;
thread_local bool controller_reference=false;
struct ReferenceScope {
  bool previous=controller_reference;
  ReferenceScope() { controller_reference=true; }
  ~ReferenceScope() { controller_reference=previous; }
};
// Bounded one-shot handoff, not a process-wide source-address cache. Another
// controller or a skeleton handoff consumes/replaces it on the same update lane.
thread_local animation_source::ControllerHandoff completed_controller;
auto TakeCompletedLayer(uint32_t visual, uint32_t graph, uint64_t generation, uint32_t source) {
  bool changed=false;
  animation_source::ControllerHandoff::Difference difference;
  auto result=completed_controller.TakeLayer(visual,graph,generation,source,Word,changed,&difference);
  if (changed) {
    auto &store=Clips(); std::lock_guard lock(store.mutex);
    if (++store.channel_changed <= 6)
      BD_INFO("[native-animation-channel-changed] visual {:08X} graph {:08X} model {} joint {} word {} expected {:08X} actual {:08X} readable {}; checked import retained",
          visual,graph,generation,difference.joint,difference.word,difference.expected,
          difference.actual.value_or(0),difference.actual.has_value());
  }
  return result;
}
void PrepareSlot(uint32_t visual, uint32_t slot) {
  const auto source = animation_source::SelectedSlotSource(visual,slot,Word);
  if (!source || !*source) return;
  auto &store = Clips(); std::lock_guard lock(store.mutex);
  store.assets.Prepare(*source,FrameStatFrameCount(),[&](uint32_t address,size_t budget) {
    animation_source::ImportTrace trace;
    uint64_t missing_word=0;
    auto asset = animation_source::ReadKeyedAsset(address,budget,[&](uint64_t word) {
      const auto value=Word(word); if (!value && !missing_word) missing_word=word; return value;
    },&trace);
    ++(asset ? store.loaded : store.prepare_refused);
    if (!asset && store.prepare_refused <= 4)
      BD_INFO("[native-animation-prepare-refused] source {:08X} budget {} stage {} track {} channel {} key {} missing-word {:X}; original source retained",
          address,budget,trace.stage,trace.track,trace.channel,trace.key,missing_word);
    if (asset && trace.unique_names < trace.descriptors && ++store.canonicalized <= 4)
      BD_INFO("[native-animation-canonicalized] source {:08X} descriptors {} unique {} bytes {}; first-match curves owned; no duplicate payloads",
          address,trace.descriptors,trace.unique_names,asset->RetainedBytes());
    return asset;
  });
}
struct VisualScope {
  uint32_t previous;
  explicit VisualScope(uint32_t visual) : previous(slot_graph) {
    slot_graph = Enabled() ? Word(uint64_t(visual)+2620).value_or(0) : 0;
    if (!slot_graph || !FindLoadedNativeModel(slot_graph)) { slot_graph=0; return; }
    // Six physical slots include the separately selected subtree overlays, not
    // just the active whole-body count at +2208. Prepare before any sampler call.
    for (uint32_t slot=0; slot<6; ++slot) PrepareSlot(visual,slot);
  }
  ~VisualScope() { slot_graph = previous; }
};
struct SlotScope {
  uint32_t previous, previous_visual;
  explicit SlotScope(uint32_t visual, uint32_t slot) : previous(slot_graph), previous_visual(slot_visual) {
    slot_visual=visual;
    slot_graph = Enabled() ? Word(uint64_t(visual)+2620).value_or(0) : 0;
    if (!slot_graph || !FindLoadedNativeModel(slot_graph)) { slot_graph=0; return; }
    PrepareSlot(visual,slot);
  }
  ~SlotScope() { slot_graph=previous; slot_visual=previous_visual; }
};
bool Sample(PPCContext &ctx, uint8_t *base, bool preserve) {
  if (!Enabled() || controller_reference) return false;
  const float weight=preserve ? float(ctx.f2.f64) : 1.0f;
  if (!std::isfinite(weight)) return Unavailable(Missing::Weight);
  // The original dispatcher returns before touching nodes, channels or globals.
  if (preserve && std::abs(ctx.f2.f64) < kNativeAnimationWeightEpsilon) return true;
  const uint32_t graph = preserve ? slot_graph : ctx.r4.u32;
  const auto model = FindLoadedNativeModel(graph);
  if (!model || model->AnimationTargets().empty() || model->AnimationTargets().size() != model->Skeleton().size())
    return Unavailable(Missing::Model);
  std::shared_ptr<const NativeAnimationAsset> asset;
  { auto &store=Clips(); std::lock_guard lock(store.mutex); asset=store.assets.Find(ctx.r5.u32); }
  if (!asset) return Unavailable(Missing::Asset);
  const bool indexed=asset->Indexed();
  uint32_t root_pose=model->Skeleton().front().pose_index;
  const bool single_subtree=preserve && !indexed && ctx.r8.u32 != 0;
  bool partial_subtree=false;
  if (preserve) {
    const auto index=Word(ctx.r4.u32), hash=Word(uint64_t(ctx.r4.u32)+4);
    if (!index || *index >= model->AnimationTargets().size() || !hash ||
        model->AnimationTargets()[*index] != *hash) return Unavailable(Missing::Subtree);
    root_pose=*index;
    partial_subtree=single_subtree || Word(uint64_t(graph)+16) != ctx.r4.u32;
  }
  const auto compression=Word(kSamplerState), exclusion = Word(kSamplerState+4), depth = Word(kSamplerState+24);
  const auto included=indexed ? NativeJointName{{},0} : skeleton_source::ReadJointName(kSamplerState+8,Word);
  const auto euler_mode=Word(0x827A7EE4); // byte +2: source qZ*qY*qX mode is zero
  // Dense dispatch does not set/reset sampler globals. Its legacy inherited
  // compressed-mode variant is not the ordinary indexed asset contract.
  if (!compression || !exclusion || !included.Valid() || !depth || (indexed ? *compression != 0 : *depth != 0) ||
      !euler_mode || ((*euler_mode >> 8) & 255))
    return Unavailable(Missing::Restrictions);
  const bool direct=!preserve || (weight == 1 && !single_subtree && !*exclusion);
  std::array<NativeJointName,30> excluded_names;
  std::array<std::string_view,30> excluded_views;
  size_t excluded_count=0;
  // Direct keyed sampling ignores exclusions; an included name takes priority
  // in weighted traversal. The controller caps this temporary pointer table at30.
  if (!indexed && !direct && included.View().empty()) {
    if (*exclusion > excluded_names.size()) return Unavailable(Missing::Restrictions);
    for (uint32_t n=0; n<*exclusion; ++n) {
      const auto pointer=Word(0x82DBEC70+uint64_t(n)*4);
      if (!pointer) return Unavailable(Missing::Restrictions);
      if (!*pointer) continue;
      auto &name=excluded_names[excluded_count];
      name=skeleton_source::ReadJointName(*pointer,Word);
      if (!name.Valid()) return Unavailable(Missing::Restrictions);
      excluded_views[excluded_count++]=name.View();
    }
  }
  const NativeAnimationFilter filter{included.View(),std::span(excluded_views).first(excluded_count),direct};
  const float seconds = float(ctx.f1.f64/30.0);
  // The weighted dispatcher does not clamp; its slot caller does. Refuse other
  // out-of-domain calls instead of silently changing their behavior.
  if (preserve && (!std::isfinite(seconds) || seconds < 0 || seconds > asset->Duration()))
    return Unavailable(Missing::Sampling);
  const uint32_t destination = ctx.r3.u32;
  const auto count = model->AnimationTargets().size();
  if ((destination & 3) || !Range(destination,count*48)) return Unavailable(Missing::Output);
  animation_source::ControllerLayer layer(count);
  const bool visual_output=slot_visual && Word(uint64_t(slot_visual)+2628) == destination &&
      Word(uint64_t(slot_visual)+1868) == count;
  bool reused=false;
  if (preserve) {
    auto previous=visual_output ? TakeCompletedLayer(slot_visual,graph,model->Generation(),destination) : std::nullopt;
    if (previous) { layer=std::move(previous->layer); reused=true; }
    else {
      std::vector<animation_source::ChannelRecord> records(count);
      const auto *input = bd::mem::at<const be_u32>(destination);
      for (size_t n=0; n<count; ++n) for (size_t word=0; word<12; ++word) records[n][word] = input[n*12+word];
      layer=animation_source::ControllerLayer::Decode(records);
    }
  }
  const bool applied=layer.Apply(*asset,model->AnimationTargets(),
      model->Skeleton(),seconds,weight,root_pose,single_subtree,filter,!preserve);
  if (!applied)
    return Unavailable(Missing::Sampling);
  const auto records=layer.Encode();
  const auto selected=SelectNativeAnimationNodes(model->Skeleton(),root_pose,single_subtree,filter);
  bool used_cubic=false;
  for (size_t n=0; n<model->Skeleton().size(); ++n) {
    if (!selected || !selected->channels[n]) continue;
    const auto *track=asset->FindTrack(model->AnimationTargets()[model->Skeleton()[n].pose_index],model->Skeleton()[n].pose_index);
    used_cubic |= track && track->splines && (track->splines->translation.active ||
        track->splines->rotation.active || track->splines->scale.active);
  }
  if (REXCVAR_GET(bd_native_materials_verify)) {
    if (preserve) __imp__sub_8228A3E8(ctx,base); else __imp__sub_82289888(ctx,base);
    const auto *original = bd::mem::at<const be_u32>(destination);
    bool same = true; size_t first_joint = 0, first_word = 0;
    for (size_t n=0; n<count; ++n) for (size_t word=0; word<12; ++word) {
      const uint32_t expected = original[n*12+word], actual = records[n][word];
      const bool active = (word>=2 && word<5 && (records[n][0]&1)) ||
          (word>=5 && word<9 && (records[n][0]&2)) || (word>=9 && (records[n][0]&4));
      const float a = std::bit_cast<float>(actual), b = std::bit_cast<float>(expected);
      const bool equal = active ? std::isfinite(a) && std::isfinite(b) &&
          std::abs(a-b) <= 1e-4f*std::max(1.0f,std::abs(b)) : actual == expected;
      if (same && !equal) { first_joint = n; first_word = word; }
      same &= equal;
    }
    const bool state_same=Word(kSamplerState) == (indexed ? *compression : 0u) && Word(kSamplerState+24) == *depth &&
        Word(kSamplerState+4) == (preserve && !indexed ? 0u : *exclusion);
    if (!state_same) BD_ERROR("[native-animation-state-drift] sampler traversal state did not return to its authored boundary");
    same &= state_same;
    auto &store=Clips(); std::lock_guard lock(store.mutex); ++store.checks;
    if (!same) {
      ++store.wrong;
      BD_ERROR("[native-animation-drift] model {} preserve {} seconds {} weight {} root {} single {} joint {} word {} native {:08X} original {:08X}; no replacement/fallback",
          model->Generation(),preserve,seconds,weight,root_pose,single_subtree,first_joint,first_word,records[first_joint][first_word],
          uint32_t(original[first_joint*12+first_word]));
      throw std::runtime_error("Native animation channel comparison failed");
    }
  }
  auto *output = bd::mem::at<be_u32>(destination);
  for (size_t n=0; n<count; ++n) for (size_t word=0; word<12; ++word) output[n*12+word] = records[n][word];
  if (!indexed) {
    bd::mem::store<uint32_t>(kSamplerState,0);
    if (preserve) bd::mem::store<uint32_t>(kSamplerState+4,0);
  }
  if (visual_output && !completed_controller.Publish({slot_visual,graph,destination,model->Generation(),
          std::move(layer.channels),records,true}))
    throw std::runtime_error("Native standalone animation publication refused");
  auto &store=Clips(); std::lock_guard lock(store.mutex);
  store.slot_publications+=visual_output; store.slot_reused+=reused;
  ++store.sampled; ++(preserve ? store.preserved : store.whole); store.cubic += used_cubic;
  store.weighted += preserve && weight != 1; store.subtree += partial_subtree;
  store.filtered += !filter.included.empty() || !filter.excluded.empty(); store.indexed += indexed; Report(store);
  return true;
}
bool Mix(PPCContext &ctx, uint8_t *base) {
  if (!Enabled() || controller_reference) return false;
  if (ctx.r4.s32 <= 0) return true;
  const auto model=FindLoadedNativeModel(slot_graph);
  const uint32_t count=ctx.r4.u32, destination=ctx.r3.u32, left=ctx.r5.u32, right=ctx.r6.u32;
  if (!model || count != model->Skeleton().size()) return Unavailable(Missing::Model);
  const uint64_t bytes=uint64_t(count)*48;
  auto valid=[&](uint32_t input) {
    return !(input&3) && Range(input,bytes) && (input == destination ||
        uint64_t(input)+bytes <= destination || uint64_t(destination)+bytes <= input);
  };
  if ((destination&3) || !Range(destination,bytes) || !valid(left) || !valid(right)) return Unavailable(Missing::Output);
  auto read=[&](uint32_t address) {
    std::vector<animation_source::ChannelRecord> records(count);
    const auto *input=bd::mem::at<const be_u32>(address);
    for (size_t n=0; n<count; ++n) for (size_t word=0; word<12; ++word) records[n][word]=input[n*12+word];
    return records;
  };
  const auto a=read(left), b=read(right);
  std::vector<animation_source::ChannelRecord> records;
  const float weight=float(ctx.f1.f64);
  if (!animation_source::MixChannelRecords(a,b,weight,destination == left,destination == right,records))
    return Unavailable(Missing::Sampling);
  if (REXCVAR_GET(bd_native_materials_verify)) {
    __imp__sub_82284BE0(ctx,base);
    const auto expected=read(destination);
    for (size_t n=0; n<count; ++n) for (size_t word=0; word<12; ++word) {
      const bool active=(word>=2 && word<5 && (records[n][0]&1)) ||
          (word>=5 && word<9 && (records[n][0]&2)) || (word>=9 && (records[n][0]&4));
      const float native=std::bit_cast<float>(records[n][word]), original=std::bit_cast<float>(expected[n][word]);
      const bool equal=active ? std::isfinite(native) && std::isfinite(original) &&
          std::abs(native-original) <= 1e-4f*std::max(1.0f,std::abs(original)) : records[n][word] == expected[n][word];
      if (!equal) {
        auto &store=Clips(); std::lock_guard lock(store.mutex); ++store.wrong;
        BD_ERROR("[native-animation-mix-drift] model {} weight {} aliases {}/{} joint {} word {} native {:08X} original {:08X}; no replacement/fallback",
            model->Generation(),weight,destination == left,destination == right,n,word,records[n][word],expected[n][word]);
        throw std::runtime_error("Native animation layer comparison failed");
      }
    }
    auto &store=Clips(); std::lock_guard lock(store.mutex); ++store.mix_checks;
  }
  auto *output=bd::mem::at<be_u32>(destination);
  for (size_t n=0; n<count; ++n) for (size_t word=0; word<12; ++word) output[n*12+word]=records[n][word];
  auto &store=Clips(); std::lock_guard lock(store.mutex); ++store.mixed; Report(store);
  return true;
}

std::optional<float> Scalar(uint64_t address) {
  const auto value=Word(address);
  if (!value) return {};
  const float scalar=std::bit_cast<float>(*value);
  return std::isfinite(scalar) ? std::optional(scalar) : std::nullopt;
}
std::optional<std::vector<uint32_t>> PointerVector(uint64_t begin_address, uint64_t end_address, size_t maximum) {
  const auto begin=Word(begin_address), end=Word(end_address);
  if (!begin || !end) return {};
  if (!*begin) return std::vector<uint32_t>{};
  if (*end < *begin || ((*end-*begin)&3) || (*end-*begin)/4 > maximum) return {};
  std::vector<uint32_t> values;
  for (uint64_t address=*begin; address<*end; address+=4) {
    const auto value=Word(address); if (!value) return {}; values.push_back(*value);
  }
  return values;
}
std::optional<uint32_t> SourceNode(uint32_t graph, uint32_t pose, size_t count) {
  const auto first=Word(uint64_t(graph)+16); if (!first || !*first) return {};
  std::vector<uint32_t> pending{*first}; std::unordered_set<uint32_t> seen;
  while (!pending.empty()) {
    const uint32_t node=pending.back(); pending.pop_back();
    if (!seen.insert(node).second || seen.size() > count) return {};
    const auto index=Word(node), child=Word(uint64_t(node)+56), sibling=Word(uint64_t(node)+60);
    if (!index || !child || !sibling) return {};
    if (*index == pose) return node;
    if (*sibling) pending.push_back(*sibling);
    if (*child) pending.push_back(*child);
  }
  return {};
}
bool LateLayers(PPCContext &ctx, uint8_t *base) {
  if (!Enabled() || controller_reference) return false;
  auto refuse=[&](const char *reason) {
    auto &store=Clips(); std::lock_guard lock(store.mutex);
    if (++store.late_refused <= 4) BD_INFO("[native-animation-late-refused] {}; original late transaction retained",reason);
    return false;
  };
  const uint32_t visual=ctx.r3.u32;
  const auto model=FindLoadedNativeModel(slot_graph);
  const auto count=Word(uint64_t(visual)+1868), output=Word(uint64_t(visual)+2628);
  if (!model || model->AnimationTargets().empty() || !count || *count != model->Skeleton().size() ||
      !output || (*output&3) || !Range(*output,uint64_t(*count)*48)) return refuse("model/output");
  std::array<NativeAnimationSlot,kNativeAnimationSlots> slots;
  std::array<bool,kNativeAnimationSlots> enabled{};
  std::array<std::shared_ptr<const NativeAnimationAsset>,kNativeAnimationSlots> assets;
  // sub_822D3CB0: authored order 4,5,3; slot5's selection504 is excluded.
  for (const auto [n,gate] : {std::pair{4u,5452u},std::pair{5u,5456u},std::pair{3u,5528u}}) {
    const auto active=Word(uint64_t(visual)+gate);
    if (!active) return refuse("selection gate");
    if (!*active) continue;
    const uint64_t address=uint64_t(visual)+n*56;
    if (n == 5) {
      const auto selection=Word(address+1872);
      if (!selection) return refuse("late selection");
      if (*selection == 504) continue;
    }
    const auto entry=Word(address+1920);
    if (!entry) return refuse("late entry");
    if (!*entry) continue;
    const auto source=Word(uint64_t(*entry)+12), loop=Word(address+1876), loops=Word(address+1880);
    const auto weight=Scalar(address+1884), target=Scalar(address+1888), rate=Scalar(address+1892),
        time=Scalar(address+1896), speed=Scalar(address+1900);
    const auto duration=source ? animation_source::Half(uint64_t(*source)+4,Word) : std::nullopt;
    if (!source || !*source || !loop || !loops || !weight || !target || !rate || !time || !speed || !duration)
      return refuse("late clock");
    auto &slot=slots[n]; enabled[n]=slot.present=true;
    slot.loop=*loop != 0; slot.loops=*loops; slot.weight=*weight; slot.target_weight=*target;
    slot.weight_rate=*rate; slot.time_ticks=*time; slot.time_rate=*speed; slot.duration_ticks=float(*duration);
    { auto &store=Clips(); std::lock_guard lock(store.mutex); assets[n]=store.assets.Find(*source); }
  }
  if (std::ranges::none_of(enabled,[](bool value) { return value; })) return true;
  const auto mode=Word(0x82DEBEEC), compression=Word(kSamplerState), exclusion=Word(kSamplerState+4),
      depth=Word(kSamplerState+24), euler=Word(0x827A7EE4);
  const auto included=skeleton_source::ReadJointName(kSamplerState+8,Word);
  if (!mode || !compression || !exclusion || *exclusion > 30 || !depth || *depth || !euler || ((*euler>>8)&255) ||
      !included.Valid() || Word(0x820551AC) != std::bit_cast<uint32_t>(1.0f) || Word(0x82055230) != 0u)
    return refuse("sampler state");
  std::vector<NativeJointName> exclusions;
  for (uint32_t n=0; n<*exclusion; ++n) {
    const auto pointer=Word(0x82DBEC70+uint64_t(n)*4);
    if (!pointer) return refuse("exclusion table");
    const auto name=*pointer ? skeleton_source::ReadJointName(*pointer,Word) : NativeJointName{};
    if (*pointer && !name.Valid()) return refuse("exclusion name");
    exclusions.push_back(name);
  }
  ctx.fpscr.disableFlushMode();
  const auto plan=PlanNativeAnimationLateLayers(slots,enabled,*mode != 0,included);
  if (!plan) return refuse("late plan");
  auto previous=TakeCompletedLayer(visual,slot_graph,model->Generation(),*output);
  const bool reused=previous.has_value();
  animation_source::ControllerLayer layer;
  if (previous) layer=std::move(previous->layer);
  else {
    std::vector<animation_source::ChannelRecord> records(*count);
    const auto *input=bd::mem::at<const be_u32>(*output);
    for (size_t n=0; n<*count; ++n) for (size_t w=0; w<12; ++w) records[n][w]=input[n*12+w];
    layer=animation_source::ControllerLayer::Decode(records);
  }
  auto result=animation_source::ExecuteController(*plan,assets,model->AnimationTargets(),model->Skeleton(),
      std::move(layer),*compression,exclusions,false);
  if (!result) return refuse("late layers");
  const auto records=result->output.Encode();
  if (REXCVAR_GET(bd_native_materials_verify)) {
    ReferenceScope reference;
    __imp__sub_822D3CB0(ctx,base);
    bool same=Word(kSamplerState) == result->compression && Word(kSamplerState+4) == result->exclusions &&
        Word(kSamplerState+24) == *depth && skeleton_source::ReadJointName(kSamplerState+8,Word).View() == included.View();
    for (uint32_t n=0; n<kNativeAnimationSlots; ++n) if (plan->advanced[n]) {
      const uint64_t address=uint64_t(visual)+n*56; const auto &slot=plan->slots[n];
      same &= Word(address+1880) == slot.loops && Word(address+1884) == std::bit_cast<uint32_t>(slot.weight) &&
          Word(address+1896) == std::bit_cast<uint32_t>(slot.time_ticks);
    }
    const auto *original=bd::mem::at<const be_u32>(*output);
    size_t first_joint=0, first_word=0; bool channel_same=true;
    for (size_t n=0; n<*count; ++n) for (size_t w=0; w<12; ++w) {
      const uint32_t expected=original[n*12+w], actual=records[n][w];
      const bool active=(w>=2 && w<5 && (records[n][0]&1)) || (w>=5 && w<9 && (records[n][0]&2)) ||
          (w>=9 && (records[n][0]&4));
      const float a=std::bit_cast<float>(actual), b=std::bit_cast<float>(expected);
      const bool equal=active ? std::isfinite(a) && std::isfinite(b) && std::abs(a-b) <= 1e-4f*std::max(1.0f,std::abs(b)) : actual == expected;
      if (channel_same && !equal) { first_joint=n; first_word=w; } channel_same &= equal;
    }
    auto &store=Clips(); std::lock_guard lock(store.mutex); ++store.late_checks;
    if (!same || !channel_same) {
      ++store.wrong;
      BD_ERROR("[native-animation-late-drift] model {} state {} channels {} joint {} word {}; no fallback",
          model->Generation(),same,channel_same,first_joint,first_word);
      throw std::runtime_error("Native late animation comparison failed");
    }
  }
  for (uint32_t n=0; n<kNativeAnimationSlots; ++n) if (plan->advanced[n]) {
    const uint32_t address=visual+n*56; const auto &slot=plan->slots[n];
    bd::mem::store<uint32_t>(address+1880,slot.loops); bd::mem::store<float>(address+1884,slot.weight);
    bd::mem::store<float>(address+1896,slot.time_ticks);
  }
  bd::mem::store<uint32_t>(kSamplerState,result->compression); bd::mem::store<uint32_t>(kSamplerState+4,result->exclusions);
  auto *destination=bd::mem::at<be_u32>(*output);
  for (size_t n=0; n<*count; ++n) for (size_t w=0; w<12; ++w) destination[n*12+w]=records[n][w];
  if (!completed_controller.Publish({visual,slot_graph,*output,model->Generation(),std::move(result->output.channels),records,true}))
    throw std::runtime_error("Native late animation publication refused");
  auto &store=Clips(); std::lock_guard lock(store.mutex);
  ++store.late_controllers; store.late_samples+=result->sampled; store.late_reused+=reused; Report(store);
  return true;
}
bool Controller(PPCContext &ctx, uint8_t *base) {
  if (!Enabled() || controller_reference) return false;
  ctx.fpscr.disableFlushMode(); // Same scalar mode as the original prologue/exit.
  completed_controller.Clear();
  auto refuse=[&](const char *reason) {
    auto &store=Clips(); std::lock_guard lock(store.mutex);
    if (++store.controller_refused <= 6) BD_INFO("[native-animation-controller-refused] {}; original complete controller retained",reason);
    return false;
  };
  const uint32_t visual=ctx.r3.u32;
  const auto model=FindLoadedNativeModel(slot_graph);
  const auto count=Word(uint64_t(visual)+1868), output=Word(uint64_t(visual)+2628), active=Word(uint64_t(visual)+2208);
  const auto delta=Scalar(0x82DDA880);
  const auto mode=Word(0x82DEBEEC), stamp=Word(0x82DC99C4), initialized=Word(0x82E18660);
  const auto compression=Word(kSamplerState), exclusion_count=Word(kSamplerState+4), euler=Word(0x827A7EE4);
  if (!model || model->AnimationTargets().empty() || !count || *count != model->Skeleton().size() ||
      !output || (*output&3) || !Range(*output,uint64_t(*count)*48) || !active || *active > kNativeAnimationSlots ||
      !delta || !mode || !stamp || !initialized || !compression || !exclusion_count || *exclusion_count > 30 ||
      !euler || ((*euler>>8)&255) || !Range(kSamplerState,28) || !Range(uint64_t(visual)+1872,6*56) ||
      !Range(uint64_t(visual)+3476,4) || Word(0x820551AC) != std::bit_cast<uint32_t>(1.0f) || Word(0x82055230) != 0u)
    return refuse("model/controller boundary");
  NativeAnimationControllerInput input; input.active=*active; input.delta_ticks=*delta;
  input.sequential=ctx.r4.u32 == 1; input.replace_slots=*mode != 0;
  if (!animation_source::ControllerSourceExtentFits(*count,*active,input.sequential)) return refuse("overlapping source layer extents");
  std::array<std::shared_ptr<const NativeAnimationAsset>,kNativeAnimationSlots> assets;
  std::array<uint8_t,16> included_bytes;
  for (size_t n=0; n<16; ++n) included_bytes[n]=bd::mem::load<uint8_t>(kSamplerState+8+uint32_t(n));
  for (uint32_t n=0; n<kNativeAnimationSlots; ++n) {
    auto &slot=input.slots[n]; const uint64_t address=uint64_t(visual)+n*56;
    const auto entry=Word(address+1920);
    const auto contribution=Scalar(address+1924);
    if (!entry) return refuse("slot entry");
    slot.present=*entry != 0;
    // Contributions of absent slots still participate in the multi-layer loop.
    if (*active != 1 && !input.sequential && n < *active) {
      if (!contribution) return refuse("layer contribution"); slot.contribution=*contribution;
    }
    if (!slot.present) continue;
    const auto source=Word(uint64_t(*entry)+12), loop=Word(address+1876), loops=Word(address+1880);
    const auto weight=Scalar(address+1884), target=Scalar(address+1888), rate=Scalar(address+1892),
        time=Scalar(address+1896), speed=Scalar(address+1900);
    const auto duration=source ? animation_source::Half(uint64_t(*source)+4,Word) : std::nullopt;
    if (!source || !*source || !loop || !loops || !weight || !target || !rate || !time || !speed || !duration)
      return refuse("slot clock");
    slot.loop=*loop != 0; slot.loops=*loops; slot.weight=*weight; slot.target_weight=*target; slot.weight_rate=*rate;
    slot.time_ticks=*time; slot.time_rate=*speed; slot.duration_ticks=float(*duration);
    slot.included=skeleton_source::ReadJointName(address+1904,Word);
    if (!slot.included.Valid()) return refuse("slot name");
    if (n < *active) for (size_t b=0; b<=slot.included.length; ++b) included_bytes[b]=uint8_t(slot.included.bytes[b]);
    { auto &store=Clips(); std::lock_guard lock(store.mutex); assets[n]=store.assets.Find(*source); }
  }
  included_bytes[0]=0;
  std::vector<uint32_t> exclusions;
  if (ctx.r5.u32) {
    const auto overlays=PointerVector(uint64_t(ctx.r5.u32)+4,uint64_t(ctx.r5.u32)+8,kNativeAnimationSlots);
    if (!overlays) return refuse("overlay names");
    input.overlay_count=uint32_t(overlays->size());
    for (uint32_t n=1; n<input.overlay_count; ++n) {
      if (!(*overlays)[n]) return refuse("null overlay name");
      input.overlays[n]=skeleton_source::ReadJointName((*overlays)[n],Word);
      if (!input.overlays[n].Valid()) return refuse("overlay name extent");
      exclusions.push_back((*overlays)[n]);
    }
  }
  const auto excluded_nodes=PointerVector(uint64_t(visual)+4616,uint64_t(visual)+4620,30);
  if (!excluded_nodes || exclusions.size()+excluded_nodes->size() > 30) return refuse("excluded nodes");
  for (uint32_t pose : *excluded_nodes) {
    const auto node=SourceNode(slot_graph,pose,*count);
    if (!node) return refuse("unresolved excluded node");
    exclusions.push_back(*node+64);
  }
  const bool write_exclusions=!exclusions.empty();
  if (!write_exclusions) for (uint32_t n=0; n<*exclusion_count; ++n) {
    const auto pointer=Word(0x82DBEC70+uint64_t(n)*4);
    if (!pointer) return refuse("inherited exclusions"); exclusions.push_back(*pointer);
  }
  std::vector<NativeJointName> excluded_names;
  for (uint32_t pointer : exclusions) {
    auto name=pointer ? skeleton_source::ReadJointName(pointer,Word) : NativeJointName{};
    if (pointer && !name.Valid()) return refuse("excluded name"); excluded_names.push_back(name);
  }
  const auto plan=PlanNativeAnimationController(input);
  if (!plan) return refuse("incomplete native layer plan");
  std::vector<animation_source::ChannelRecord> before(*count);
  const auto *source_channels=bd::mem::at<const be_u32>(*output);
  for (size_t n=0; n<*count; ++n) for (size_t w=0; w<12; ++w) before[n][w]=source_channels[n*12+w];
  auto result=animation_source::ExecuteController(*plan,assets,model->AnimationTargets(),model->Skeleton(),
      animation_source::ControllerLayer::Decode(before),*compression,excluded_names);
  if (!result) return refuse("native layer execution");
  const auto records=result->output.Encode();
  if (records.size() != *count || (write_exclusions && !Range(0x82DBEC70,exclusions.size()*4))) return refuse("outgoing boundary");
  const bool verify=REXCVAR_GET(bd_native_materials_verify);
  if (verify) {
    ReferenceScope reference;
    // Full original executes side effects exactly once, with original channels.
    // Native slot/channel transactions have not modified source state yet.
    __imp__bdAnimationUpdate(ctx,base);
    bool same=Word(uint64_t(visual)+3476) == *stamp && Word(0x82E18660) == (*initialized|1u) &&
        Word(kSamplerState) == result->compression && Word(kSamplerState+4) == result->exclusions && Word(kSamplerState+24) == 0u;
    for (uint32_t n=0; n<kNativeAnimationSlots; ++n) if (plan->advanced[n]) {
      const uint64_t address=uint64_t(visual)+n*56; const auto &slot=plan->slots[n];
      same &= Word(address+1880) == slot.loops && Word(address+1884) == std::bit_cast<uint32_t>(slot.weight) &&
          Word(address+1896) == std::bit_cast<uint32_t>(slot.time_ticks);
    }
    for (size_t n=0; n<16; ++n) same &= bd::mem::load<uint8_t>(kSamplerState+8+uint32_t(n)) == included_bytes[n];
    if (write_exclusions) for (size_t n=0; n<exclusions.size(); ++n) same &= Word(0x82DBEC70+n*4) == exclusions[n];
    size_t first_joint=0, first_word=0; bool channel_same=true;
    for (size_t n=0; n<*count; ++n) for (size_t w=0; w<12; ++w) {
      const uint32_t expected=source_channels[n*12+w], actual=records[n][w];
      const bool active_value=(w>=2 && w<5 && (records[n][0]&1)) ||
          (w>=5 && w<9 && (records[n][0]&2)) || (w>=9 && (records[n][0]&4));
      const float a=std::bit_cast<float>(actual), b=std::bit_cast<float>(expected);
      const bool equal=active_value ? std::isfinite(a) && std::isfinite(b) && std::abs(a-b) <= 1e-4f*std::max(1.0f,std::abs(b)) : actual == expected;
      if (channel_same && !equal) { first_joint=n; first_word=w; } channel_same &= equal;
    }
    auto &store=Clips(); std::lock_guard lock(store.mutex); ++store.controller_checks;
    if (!same || !channel_same) {
      ++store.wrong;
      BD_ERROR("[native-animation-controller-drift] model {} state {} channels {} joint {} word {}; no fallback or second side-effect update",
          model->Generation(),same,channel_same,first_joint,first_word);
      throw std::runtime_error("Native animation controller comparison failed");
    }
  }
  for (uint32_t n=0; n<kNativeAnimationSlots; ++n) if (plan->advanced[n]) {
    const uint32_t address=visual+n*56; const auto &slot=plan->slots[n];
    bd::mem::store<uint32_t>(address+1880,slot.loops); bd::mem::store<float>(address+1884,slot.weight);
    bd::mem::store<float>(address+1896,slot.time_ticks);
  }
  bd::mem::store<uint32_t>(visual+3476,*stamp); bd::mem::store<uint32_t>(0x82E18660,*initialized|1u);
  bd::mem::store<uint32_t>(kSamplerState,result->compression); bd::mem::store<uint32_t>(kSamplerState+4,result->exclusions);
  bd::mem::store<uint32_t>(kSamplerState+24,0);
  for (size_t n=0; n<16; ++n) bd::mem::store<uint8_t>(kSamplerState+8+uint32_t(n),included_bytes[n]);
  if (write_exclusions) for (size_t n=0; n<exclusions.size(); ++n) bd::mem::store<uint32_t>(0x82DBEC70+uint32_t(n)*4,exclusions[n]);
  auto *destination=bd::mem::at<be_u32>(*output);
  for (size_t n=0; n<*count; ++n) for (size_t w=0; w<12; ++w) destination[n*12+w]=records[n][w];
  if (!completed_controller.Publish({visual,slot_graph,*output,model->Generation(),std::move(result->output.channels),records}))
    throw std::runtime_error("Native animation completed channel publication refused");
  if (!verify) {
    // Authored gameplay/effect side effects remain outside the native layer plan.
    if (bd::mem::load<uint32_t>(visual+3532) && bd::mem::load<uint32_t>(visual+3000) == 3) {
      ctx.r3.u32=visual; bdVisualObjectCollisionTestNearby(ctx,base);
    }
    if (bd::mem::load<uint32_t>(visual+3560)) { ctx.r3.u32=visual; bdEffectUpdate(ctx,base); }
    if (bd::mem::load<uint32_t>(visual+3440)) {
      ctx.fpscr.disableFlushMode();
      std::array<float,4> offsets, rates;
      for (uint32_t n=0; n<4; ++n) {
        offsets[n]=bd::mem::load<float>(visual+3444+n*4); rates[n]=bd::mem::load<float>(visual+3460+n*4);
      }
      const auto advanced=AdvanceNativeAnimationOffsets(offsets,rates);
      for (uint32_t n=0; n<4; ++n) bd::mem::store<float>(visual+3444+n*4,advanced[n]);
    }
  }
  auto &store=Clips(); std::lock_guard lock(store.mutex); ++store.controllers;
  for (size_t n=0; n<kNativeAnimationSlots; ++n)
    store.controller_advancing+=plan->advanced[n] && plan->slots[n].time_ticks != input.slots[n].time_ticks;
  store.controller_samples+=result->sampled; store.controller_mixes+=result->mixed;
  store.controller_subtree+=result->subtree; store.controller_interior+=result->interior; Report(store);
  ctx.fpscr.disableFlushMode();
  return true;
}
} // namespace
} // namespace bd::gpu::scene::animation_bridge

namespace bd::gpu::scene {
std::optional<std::vector<NativeJointChannels>> TakeNativeAnimationChannels(
    uint32_t visual, uint32_t graph, uint64_t generation, uint32_t source) {
  using namespace animation_bridge;
  if (!Enabled()) { completed_controller.Clear(); return {}; }
  auto channels=TakeCompletedLayer(visual,graph,generation,source);
  auto &store=Clips(); std::lock_guard lock(store.mutex);
  store.channel_handoffs+=channels.has_value(); store.late_handoffs+=channels && channels->late;
  if (!channels) return {};
  return std::move(channels->layer.channels);
}
void RetireNativeAnimationChannels(uint32_t visual) {
  animation_bridge::completed_controller.Retire(visual);
}
}

REX_HOOK_RAW(sub_8217BD70) {
  const uint32_t owner = ctx.r3.u32;
  bd::gpu::scene::animation_bridge::Retire(owner);
  bd::gpu::scene::animation_bridge::LoadScope loading(owner);
  __imp__sub_8217BD70(ctx,base);
}
REX_HOOK_RAW(sub_8217C5E8) {
  const uint32_t owner = ctx.r3.u32;
  bd::gpu::scene::animation_bridge::Retire(owner);
  bd::gpu::scene::animation_bridge::LoadScope loading(owner);
  __imp__sub_8217C5E8(ctx,base);
}
REX_HOOK_RAW(sub_82288680) {
  const uint32_t source = ctx.r3.u32;
  __imp__sub_82288680(ctx,base);
  bd::gpu::scene::animation_bridge::Import(source);
}
REX_HOOK_RAW(sub_8217BD00) {
  // Standalone motion destructor releases loader+164's backing request.
  bd::gpu::scene::animation_bridge::Retire(ctx.r3.u32);
  __imp__sub_8217BD00(ctx,base);
}
REX_HOOK_RAW(sub_8217C580) {
  // Packed motion destructor: shared aliases retire with their one pack owner.
  bd::gpu::scene::animation_bridge::Retire(ctx.r3.u32);
  __imp__sub_8217C580(ctx,base);
}
REX_HOOK_RAW(bdVisualObjectAnimSlotUpdate) {
  bd::gpu::scene::animation_bridge::SlotScope slot(ctx.r3.u32,ctx.r4.u32);
  __imp__bdVisualObjectAnimSlotUpdate(ctx,base);
}
REX_HOOK_RAW(bdAnimationUpdate) {
  bd::gpu::scene::animation_bridge::VisualScope visual(ctx.r3.u32);
  if (!bd::gpu::scene::animation_bridge::Controller(ctx,base)) __imp__bdAnimationUpdate(ctx,base);
}
REX_HOOK_RAW(sub_822D3CB0) {
  bd::gpu::scene::animation_bridge::VisualScope visual(ctx.r3.u32);
  if (!bd::gpu::scene::animation_bridge::LateLayers(ctx,base)) __imp__sub_822D3CB0(ctx,base);
}
REX_HOOK_RAW(sub_82289888) {
  if (!bd::gpu::scene::animation_bridge::Sample(ctx,base,false)) __imp__sub_82289888(ctx,base);
}
REX_HOOK_RAW(sub_8228A3E8) {
  if (!bd::gpu::scene::animation_bridge::Sample(ctx,base,true)) __imp__sub_8228A3E8(ctx,base);
}
REX_HOOK_RAW(sub_82284BE0) {
  if (!bd::gpu::scene::animation_bridge::Mix(ctx,base)) __imp__sub_82284BE0(ctx,base);
}
