/**
 * @brief Transactional effect boundary; no packed-channel reader or retained cache.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_effect_animation.h"
#include "gpu/scene/native_material_uv.h"
#include "gpu/scene/native_material_uv_program.h"
#include "gpu/scene/native_image_catalog_source.h"
#include "gpu/scene/native_material_animation_source.h"
#include <bit>

namespace bd::gpu::scene::animation_source {
// Verification-only census BEFORE controller admission. A missing read is not
// evidence of absent authored content. No payload sampling, writes or residency.
struct EffectDrivers {
  uint32_t uv=0, named=0, translated=0, rotated=0, unresolved=0, no_channels=0;
};
template<class Read>
std::optional<EffectDrivers> ObserveEffectDrivers(uint32_t visual, Read &&read) {
  if (!visual || (visual&3) || uint64_t(visual)+3568 > uint64_t(UINT32_MAX)+1) return {};
  const auto records=read(uint64_t(visual)+3560);
  if (!records) return {};
  EffectDrivers result;
  if (!*records) return result;
  const auto count=read(uint64_t(visual)+3564);
  if (!count) return {};
  if (int32_t(*count) <= 0) return result;
  if (*count > 256 || (*records&3) || uint64_t(*records)+uint64_t(*count)*152 > uint64_t(UINT32_MAX)+1) return {};
  for (uint32_t n=0; n<*count; ++n) {
    const uint64_t record=uint64_t(*records)+n*152;
    const auto enabled=read(record+20);
    if (!enabled) return {};
    if (!*enabled) continue;
    ++result.uv;
    const auto name=read(record+120); // first byte of the authored joint NAME
    if (!name) return {};
    if (!(*name>>24)) continue;
    ++result.named;
    const auto source=read(uint64_t(visual)+2628), joint=read(record+12);
    if (!source || !joint) return {};
    if (int32_t(*joint) < 0) ++result.unresolved;
    if (!*source) ++result.no_channels;
    if (!*source || int32_t(*joint) < 0) continue;
    const auto rotation=read(record+16);
    if (!rotation) return {};
    ++(*rotation ? result.rotated : result.translated);
  }
  return result;
}

// AnimeData's effect catalog is separate from the skeletal clip catalog.
// Poll states 1..4 can load/allocate/change dependencies. Refuse before polling;
// the complete original controller must execute once, never a partial replay.
template<class Read>
std::optional<uint32_t> ReadReadyEffect(const material_image_source::LoadedCatalog &catalog, uint32_t id, Read &&read) {
  const auto ordinal=catalog.catalog.Cue(id);
  if (ordinal == UINT32_MAX) return 0u;
  const auto &entry=catalog.exports[ordinal];
  const auto state=read(uint64_t(entry.owner)+316);
  if (!state || (*state >= 1 && *state <= 4)) return {};
  return int32_t(*state) >= 6 ? entry.node : 0u; // first terminal match wins, even absent
}
template<class Read>
std::optional<double> ReadEffectDuration(uint32_t entry, double scale, Read &&read) {
  if (!std::isfinite(scale)) return {};
  double duration=0;
  if (entry) {
    if (entry&3) return {};
    const auto object=read(uint64_t(entry)+16);
    if (!object || !*object || (*object&3)) return {};
    const auto word=read(uint64_t(*object)+312);
    if (!word) return {};
    const double value=std::bit_cast<float>(*word);
    // Preserve fctiwz then integer->float, including the INT_MIN NaN result.
    const int32_t ticks=std::isnan(value) || value <= double(INT32_MIN) ? INT32_MIN :
        value >= double(INT32_MAX) ? INT32_MAX : int32_t(value);
    duration=float(ticks);
  }
  // Existing bdDrawDistanceScaleHook scales both candidates BEFORE max. Despite
  // its historical name this caller compares an AnimeData clock, not distance.
  return scale == 1 ? duration : duration*scale;
}
struct EffectUpdate {
  struct UV { uint32_t destination=0; std::array<float,2> value{}; };
  uint32_t visual=0;
  std::array<uint32_t,9> timeline{}; // outgoing-only IDs/entries; not a native owner
  NativeMaterialAnimation animation;
  bool called=false, clock_written=false, transitioned=false;
  uint32_t translated=0, rotated=0;
  uint32_t table=0, owned_offsets=0, owned_descriptors=0;
  NativeMaterialUVs material;
  std::vector<UV> uv;
  template<class Write> void Publish(Write &&write) const {
    if (transitioned) for (size_t n=0; n<timeline.size(); ++n) write(uint64_t(visual)+2212+n*4,timeline[n]);
    else if (clock_written) write(uint64_t(visual)+2224,timeline[3]);
    for (const auto &entry : uv) for (size_t n=0; n<2; ++n)
      write(uint64_t(entry.destination)+n*4,std::bit_cast<uint32_t>(entry.value[n]));
  }
  template<class Read> bool Matches(Read &&read) const {
    if (!called) return true;
    for (size_t n=0; n<timeline.size(); ++n)
      if (read(uint64_t(visual)+2212+n*4) != timeline[n]) return false;
    for (const auto &entry : uv) for (size_t n=0; n<2; ++n) {
      const auto word=read(uint64_t(entry.destination)+n*4);
      if (!word) return false;
      const float expected=std::bit_cast<float>(*word), actual=entry.value[n];
      if (!std::isfinite(expected) || std::abs(actual-expected) > 1e-4f*std::max(1.0f,std::abs(expected))) return false;
    }
    return true;
  }
};

// The bridge must validate the previous publication's source association before
// passing it here. Offset reuse does not itself check unknown outgoing writers.
template<class Read>
std::optional<EffectUpdate> PrepareEffectUpdate(uint32_t visual, float delta, double duration_scale,
    std::span<const NativeJointChannels> channels, Read &&read, const NativeMaterialUVs *previous=nullptr,
    const NativeMaterialUVProgram *program=nullptr, const material_image_source::LoadedCatalog *catalog=nullptr,
    const NativeMaterialAnimation *animation=nullptr) {
  if (!visual || (visual&3) || uint64_t(visual)+3752 > uint64_t(UINT32_MAX)+1) return {};
  const auto records=read(uint64_t(visual)+3560);
  if (!records) return {};
  EffectUpdate result; result.visual=visual;
  if (!*records) return result; // controller does not call bdEffectUpdate at all
  if (!catalog || !animation || !animation->Valid() || animation->catalog.get() != &catalog->catalog) return {};
  result.called=true;
  auto &state=result.animation; state=*animation;
  const NativeEffectClock clock{state.time,state.speed,state.ids[0] != 0,state.loop_bits != 0,state.queued[0] != 0};
  std::optional<double> duration;
  if (clock.NeedsDuration()) {
    const auto entry=[&](size_t n) { return state.cues[n] == UINT32_MAX ? 0 : catalog->exports[state.cues[n]].node; };
    const auto a=ReadEffectDuration(entry(0),duration_scale,read), b=ReadEffectDuration(entry(1),duration_scale,read);
    if (!a || !b) return {};
    duration=*a > *b ? *a : *b;
  }
  const auto step=AdvanceNativeEffectClock(clock,delta,duration);
  if (!step) return {};
  result.clock_written=clock.active;
  if (clock.active) state.time=step->time;
  if (step->transition) {
    const auto a=ReadReadyEffect(*catalog,state.queued[0],read), b=ReadReadyEffect(*catalog,state.queued[1],read);
    if (!a || !b) return {};
    const std::array cues{*a ? catalog->catalog.Cue(state.queued[0]) : UINT32_MAX,
        *b ? catalog->catalog.Cue(state.queued[1]) : UINT32_MAX};
    if (cues != state.cues) {
      result.transitioned=true;
      state.ids=state.queued; state.cues=cues; state.time=0; state.speed=1; state.queued={};
    }
  }
  const auto boundary=material_animation_source::Export(state,*catalog);
  if (!boundary) return {};
  result.timeline=*boundary;
  const auto count=read(uint64_t(visual)+3564);
  if (!count) return {};
  if (int32_t(*count) <= 0) return result;
  if (*count > 256 || (*records&3) || uint64_t(*records)+uint64_t(*count)*152 > uint64_t(UINT32_MAX)+1) return {};
  if (previous && (!previous->Valid() || previous->count != *count)) return {};
  if (program && (!program->Valid() || program->slots.size() != *count)) return {};
  result.table=*records; result.material.count=*count;
  const auto overlaps=[](uint64_t a,uint64_t bytes,uint64_t b,uint64_t extent) { return a < b+extent && b < a+bytes; };
  const auto source=read(uint64_t(visual)+2628);
  if (!source || channels.size() > kMaxNativeJoints ||
      overlaps(*records,uint64_t(*count)*152,visual,3752) ||
      (*source && overlaps(*records,uint64_t(*count)*152,*source,channels.size()*48))) return {};
  for (uint32_t n=0; n<*count; ++n) {
    const uint64_t record=uint64_t(*records)+n*152;
    if (program) {
      const auto &slot=program->slots[n];
      if (!slot.enabled) continue;
      auto motion=slot.Motion(*source != 0,program->radians_per_degree);
      const auto *owned=previous ? previous->Find(n) : nullptr;
      const bool owns_offset=owned && owned->enabled && owned->selector == slot.selector && owned->channel == slot.channel;
      for (uint32_t axis=0; axis<2; ++axis) {
        if (motion.mode == NativeEffectUVMode::Scroll && owns_offset) motion.offset[axis]=owned->uv[axis];
        else {
          const auto output=read(record+28+axis*4); // outgoing range / unconverted late offset writer
          if (!output) return {};
          if (motion.mode == NativeEffectUVMode::Scroll) motion.offset[axis]=std::bit_cast<float>(*output);
        }
      }
      const auto uv=EvaluateNativeEffectUV(motion,delta,channels);
      if (!uv) return {};
      result.translated+=motion.mode == NativeEffectUVMode::Translation;
      result.rotated+=motion.mode == NativeEffectUVMode::Rotation;
      result.owned_offsets+=owns_offset && motion.mode == NativeEffectUVMode::Scroll;
      ++result.owned_descriptors;
      result.uv.push_back({uint32_t(record+28),*uv});
      result.material.entries.push_back({n,slot.selector,slot.channel,*uv,true,NativeMaterialUVOrigin::Effect});
      continue;
    }
    const auto enabled=read(record+20);
    if (!enabled) return {};
    if (!*enabled) continue;
    const auto selector=read(record+4), channel=read(record+8);
    if (!selector || !channel) return {};
    const auto *owned=previous ? previous->Find(n) : nullptr;
    const bool owns_offset=owned && owned->enabled && owned->selector == *selector && owned->channel == *channel;
    NativeEffectUVMotion motion;
    const auto driver=read(record+120);
    if (!driver) return {};
    if ((*driver>>24) && *source) {
      const auto joint=read(record+12);
      if (!joint) return {};
      if (int32_t(*joint) >= 0) {
        const auto rotation=read(record+16);
        if (!rotation) return {};
        motion.joint=*joint;
        motion.mode=*rotation ? NativeEffectUVMode::Rotation : NativeEffectUVMode::Translation;
      }
    }
    for (size_t axis=0; axis<2; ++axis) {
      if (motion.mode == NativeEffectUVMode::Scroll) {
        const auto rate=read(record+36+axis*4);
        if (!rate) return {};
        if (owns_offset) motion.offset[axis]=owned->uv[axis];
        else {
          const auto offset=read(record+28+axis*4);
          if (!offset) return {};
          motion.offset[axis]=std::bit_cast<float>(*offset);
        }
        motion.rate[axis]=std::bit_cast<float>(*rate);
      } else {
        const auto divisor=read(record+(motion.mode == NativeEffectUVMode::Rotation ? 52 : 44)+axis*4);
        if (!divisor || !read(record+28+axis*4)) return {}; // validate outgoing destination too
        motion.divisor[axis]=std::bit_cast<float>(*divisor);
        if (motion.mode == NativeEffectUVMode::Rotation) {
          const auto factor=read(0x8208EA64);
          if (!factor) return {};
          motion.divisor[axis]=float(double(motion.divisor[axis])*double(std::bit_cast<float>(*factor)));
        }
      }
    }
    const auto uv=EvaluateNativeEffectUV(motion,delta,channels);
    if (!uv) return {};
    result.translated+=motion.mode == NativeEffectUVMode::Translation;
    result.rotated+=motion.mode == NativeEffectUVMode::Rotation;
    result.owned_offsets+=owns_offset && motion.mode == NativeEffectUVMode::Scroll;
    result.uv.push_back({uint32_t(record+28),*uv});
    result.material.entries.push_back({n,*selector,*channel,*uv,true,NativeMaterialUVOrigin::Effect});
  }
  return result;
}
} // namespace bd::gpu::scene::animation_source
