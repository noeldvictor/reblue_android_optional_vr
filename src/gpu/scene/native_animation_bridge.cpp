/**
 * @brief Completed motion loads -> owned keyed channels -> native skeleton poses.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_animation_source.h"
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
    "Load-owned keyed/cubic animation sampling for ordinary native skeletons; pending desktop qualification.");
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
REX_EXTERN(__imp__sub_82289888);
REX_EXTERN(__imp__sub_8228A3E8);
REX_EXTERN(__imp__sub_82284BE0);

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
  uint64_t mixed = 0, mix_checks = 0;
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
  BD_INFO("[native-animation] frame {} loads {} refused {} resident {} bytes {}; sampled {} whole {} preserved {} unavailable {}; checked {} wrong {}; cubic {}; registered {} catalog {} prepare-refused {}; weighted {} subtree {}; mixed {} mix-checked {}; owned keys/layers, original slot clocks and outgoing channel adapter remain",
      frame,store.loaded,store.load_refused,store.assets.ResidentCount(),store.assets.Bytes(),
      store.sampled,store.whole,store.preserved,unavailable,store.checks,store.wrong,store.cubic,
      store.registered,store.assets.Size(),store.prepare_refused,store.weighted,store.subtree,store.mixed,store.mix_checks);
  store.frame = frame;
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
  const bool published = type && (*type == 2 || *type == 3) && store.assets.Register(loading_owner,source);
  ++(published ? store.registered : store.load_refused);
  if (!published && store.load_refused <= 4) {
    BD_INFO("[native-animation-load-refused] type {}; original motion remains available",type ? int(*type) : -1);
  }
}
thread_local uint32_t slot_graph = 0;
void PrepareSlot(uint32_t visual, uint32_t slot) {
  const auto source = animation_source::SelectedSlotSource(visual,slot,Word);
  if (!source || !*source) return;
  auto &store = Clips(); std::lock_guard lock(store.mutex);
  store.assets.Prepare(*source,FrameStatFrameCount(),[&](uint32_t address,size_t budget) {
    auto asset = animation_source::ReadKeyedAsset(address,budget,Word);
    ++(asset ? store.loaded : store.prepare_refused);
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
  uint32_t previous;
  explicit SlotScope(uint32_t visual, uint32_t slot) : previous(slot_graph) {
    slot_graph = Enabled() ? Word(uint64_t(visual)+2620).value_or(0) : 0;
    if (!slot_graph || !FindLoadedNativeModel(slot_graph)) { slot_graph=0; return; }
    PrepareSlot(visual,slot);
  }
  ~SlotScope() { slot_graph=previous; }
};
bool Sample(PPCContext &ctx, uint8_t *base, bool preserve) {
  if (!Enabled()) return false;
  const float weight=preserve ? float(ctx.f2.f64) : 1.0f;
  if (!std::isfinite(weight)) return Unavailable(Missing::Weight);
  // The original dispatcher returns before touching nodes, channels or globals.
  if (preserve && std::abs(ctx.f2.f64) < kNativeAnimationWeightEpsilon) return true;
  const uint32_t graph = preserve ? slot_graph : ctx.r4.u32;
  const auto model = FindLoadedNativeModel(graph);
  if (!model || model->AnimationTargets().empty() || model->AnimationTargets().size() != model->Skeleton().size())
    return Unavailable(Missing::Model);
  uint32_t root_pose=model->Skeleton().front().pose_index;
  const bool single_subtree=preserve && ctx.r8.u32 != 0;
  bool partial_subtree=false;
  if (preserve) {
    const auto index=Word(ctx.r4.u32), hash=Word(uint64_t(ctx.r4.u32)+4);
    if (!index || *index >= model->AnimationTargets().size() || !hash ||
        model->AnimationTargets()[*index] != *hash) return Unavailable(Missing::Subtree);
    root_pose=*index;
    partial_subtree=single_subtree || Word(uint64_t(graph)+16) != ctx.r4.u32;
  }
  const auto exclusion = Word(kSamplerState+4), name = Word(kSamplerState+8), depth = Word(kSamplerState+24);
  // Named/excluded subtrees have authored traversal side effects not owned yet.
  const auto euler_mode=Word(0x827A7EE4); // byte +2: source qZ*qY*qX mode is zero
  if (!exclusion || *exclusion || !name || (*name >> 24) || !depth || *depth ||
      !euler_mode || ((*euler_mode >> 8) & 255))
    return Unavailable(Missing::Restrictions);
  std::shared_ptr<const NativeAnimationAsset> asset;
  { auto &store=Clips(); std::lock_guard lock(store.mutex); asset=store.assets.Find(ctx.r5.u32); }
  if (!asset) return Unavailable(Missing::Asset);
  const float seconds = float(ctx.f1.f64/30.0);
  // The weighted dispatcher does not clamp; its slot caller does. Refuse other
  // out-of-domain calls instead of silently changing their behavior.
  if (preserve && (!std::isfinite(seconds) || seconds < 0 || seconds > asset->Duration()))
    return Unavailable(Missing::Sampling);
  const uint32_t destination = ctx.r3.u32;
  const auto count = model->AnimationTargets().size();
  if ((destination & 3) || !Range(destination,count*48)) return Unavailable(Missing::Output);
  std::vector<animation_source::ChannelRecord> records(count);
  if (preserve) {
    const auto *input = bd::mem::at<const be_u32>(destination);
    for (size_t n=0; n<count; ++n) for (size_t word=0; word<12; ++word) records[n][word] = input[n*12+word];
  }
  const bool applied=preserve ? animation_source::ApplyKeyedLayer(*asset,model->AnimationTargets(),
      model->Skeleton(),seconds,weight,root_pose,single_subtree,records) :
      animation_source::ApplyKeyedAsset(*asset,model->AnimationTargets(),seconds,false,records);
  if (!applied)
    return Unavailable(Missing::Sampling);
  const auto selected=SelectNativeAnimationSubtree(model->Skeleton(),root_pose,single_subtree);
  bool used_cubic=false;
  for (size_t n=0; n<model->Skeleton().size(); ++n) {
    if (preserve && (!selected || !(*selected)[n])) continue;
    const auto *track=asset->FindTrack(model->AnimationTargets()[model->Skeleton()[n].pose_index]);
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
  bd::mem::store<uint32_t>(kSamplerState,0);
  if (preserve) bd::mem::store<uint32_t>(kSamplerState+4,0);
  auto &store=Clips(); std::lock_guard lock(store.mutex);
  ++store.sampled; ++(preserve ? store.preserved : store.whole); store.cubic += used_cubic;
  store.weighted += preserve && weight != 1; store.subtree += partial_subtree; Report(store);
  return true;
}
bool Mix(PPCContext &ctx, uint8_t *base) {
  if (!Enabled()) return false;
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
} // namespace
} // namespace bd::gpu::scene::animation_bridge

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
  // Preserve authored slot clocks, layer ordering, collision and effect updates.
  __imp__bdAnimationUpdate(ctx,base);
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
