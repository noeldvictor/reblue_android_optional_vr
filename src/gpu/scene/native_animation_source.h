/**
 * @brief Bounded import of relocated keyed skeletal clips; no source reads at sample time.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_animation_blend.h"
#include "gpu/scene/native_animation_asset.h"
#include <bit>
#include <unordered_map>

namespace bd::gpu::scene::animation_source {
// Temporary load-boundary association only. The resulting clip retains dense
// model-local pose IDs, never the name hash, source node or source clip pointer.
struct JointBinding { uint32_t pose_index, name_hash; };

// The reader returns big-endian numeric words and rejects unmapped addresses.
// Descriptor hashes at +18 are unaligned: do not use an aligned Word reader
// directly on them, or assume the three 16-bit key counts form padded words.
template <class ReadWord>
std::optional<uint32_t> Word(uint64_t address, ReadWord &&read) {
  if (!address || address > UINT32_MAX-3) return {};
  const auto first = read(address & ~uint64_t(3));
  if (!first) return {};
  const unsigned shift = unsigned(address & 3)*8;
  if (!shift) return first;
  const auto second = read((address & ~uint64_t(3))+4);
  if (!second) return {};
  return (*first << shift) | (*second >> (32-shift));
}
template <class ReadWord>
std::optional<uint16_t> Half(uint64_t address, ReadWord &&read) {
  if (!address || (address & 1) || address > UINT32_MAX-1) return {};
  const auto word = read(address & ~uint64_t(3));
  if (!word) return {};
  return uint16_t(*word >> ((address & 2) ? 0 : 16));
}

// bdVisualObjectSetAnimation writes a ready lookup entry to visual+1920+slot*56;
// bdVisualObjectAnimSlotUpdate reads its initialized motion pointer at entry+12.
template <class ReadWord>
std::optional<uint32_t> SelectedSlotSource(uint32_t visual, uint32_t slot, ReadWord &&read) {
  if (!visual || slot >= 6) return {};
  const auto entry = Word(uint64_t(visual)+1920+uint64_t(slot)*56,read);
  return entry && *entry ? Word(uint64_t(*entry)+12,read) : std::nullopt;
}

// The source decoder's compact float flushes exponent-zero values to signed
// zero and treats exponent31 as finite. It is not IEEE binary16 at the boundary.
inline float CompactFloat(uint16_t bits) {
  const uint32_t sign = uint32_t(bits & 0x8000)<<16;
  if (!(bits & 0x7c00)) return std::bit_cast<float>(sign);
  return std::bit_cast<float>(sign | (uint32_t((bits & 0x7c00)+0x1c000)<<13) | (uint32_t(bits & 0x3ff)<<13));
}

template <class ReadWord>
std::optional<NativeAnimationSplineChannel> ReadSplineChannel(uint32_t source, bool angular,
    float samples_per_tick, size_t maximum_bytes, size_t &used, ReadWord &&read) {
  NativeAnimationSplineChannel result;
  result.active = true; result.key_epsilon = float(.0001/samples_per_tick);
  for (unsigned axis=0; axis<3; ++axis) {
    const auto count = Word(uint64_t(source)+axis*8,read), data = Word(uint64_t(source)+axis*8+4,read);
    if (!count || !data) return {};
    if (!*data) continue; // disabled scalar is zero, even if its unused count is nonzero
    if (!*count || *count > 65536 || uint64_t(*data)+uint64_t(*count)*8 > uint64_t(UINT32_MAX)+1 ||
        *count > (maximum_bytes-used)/sizeof(NativeAnimationSplineKey)) return {};
    auto &keys = result.axes[axis]; keys.reserve(*count);
    if (keys.capacity() > (maximum_bytes-used)/sizeof(NativeAnimationSplineKey)) return {};
    used += keys.capacity()*sizeof(NativeAnimationSplineKey);
    int previous_frame = -32769;
    for (uint32_t n=0; n<*count; ++n) {
      const uint64_t address = uint64_t(*data)+n*8;
      const auto frame = Half(address,read), value = Half(address+2,read);
      const auto tangent = *count > 1 ? Word(address+4,read) : std::optional<uint32_t>(0);
      if (!frame || !value || !tangent || int16_t(*frame) <= previous_frame) return {};
      NativeAnimationSplineKey key;
      key.seconds = (float(int16_t(*frame))*(1.0f/30.0f))/samples_per_tick;
      key.value = angular ? float(int16_t(*value))*0x1p-16f : CompactFloat(*value);
      key.tangent = std::bit_cast<float>(*tangent)*samples_per_tick;
      if (angular) key.tangent *= 0x1p-16f;
      if (angular && !keys.empty() && int16_t(*frame)-previous_frame == 1) keys.back().linear_to_next = true;
      keys.push_back(key); previous_frame = int16_t(*frame);
    }
  }
  return result;
}

// sub_82198FF8 relocates 36-byte records, sub_821995B8/99628 swap exactly
// count keys (not count+1). sub_82288680 supplies samples per 30Hz logic tick.
// Type 2: T/S = {s16 frame,pad,f32 xyz}, R = {s16 frame,s16 xyz angle}.
// Type3 nonconstant channels contain three {u32 count,ptr} scalar descriptors;
// each relocated scalar key is {s16 frame,compact value,f32 tangent}. Constants
// retain the type2 layout. Dense types0/1 remain a distinct unsupported contract.
template <class ReadWord>
std::optional<NativeAnimationClip> ReadKeyedClip(uint32_t source,
    std::span<const JointBinding> bindings, size_t maximum_bytes, ReadWord &&read) {
  if (!source || bindings.empty() || bindings.size() > kMaxNativeJoints ||
      maximum_bytes < sizeof(NativeAnimationClip)) return {};
  const uint64_t header = source;
  const auto records = Word(header,read), timing = Word(header+12,read);
  const auto duration = Half(header+4,read), type = Half(header+6,read), count = Half(header+8,read);
  if (!records || !*records || !timing || !duration || !type || (*type != 2 && *type != 3) || !count ||
      *count > kMaxNativeJoints || uint64_t(*records)+uint64_t(*count)*36 > uint64_t(UINT32_MAX)+1) return {};
  const float rate = std::bit_cast<float>(*timing)*30;
  if (!std::isfinite(rate) || rate <= 0) return {};
  std::array<bool,kMaxNativeJoints> seen{};
  std::unordered_map<uint32_t,uint32_t> mapping;
  for (const auto &binding : bindings) {
    if (binding.pose_index >= bindings.size() || seen[binding.pose_index] ||
        !mapping.emplace(binding.name_hash,binding.pose_index).second) return {};
    seen[binding.pose_index] = true;
  }
  std::unordered_map<uint32_t,bool> record_hashes;
  std::vector<NativeAnimationTrack> tracks;
  // Reserve before keys and debit actual capacity; every next allocation must
  // fit. Unmatched descriptors are not consumer inputs, but duplicate hashes
  // are refused rather than guessing the original traversal cursor's meaning.
  if (bindings.size() > (maximum_bytes-sizeof(NativeAnimationClip))/sizeof(NativeAnimationTrack)) return {};
  tracks.reserve(bindings.size());
  size_t used = sizeof(NativeAnimationClip)+tracks.capacity()*sizeof(NativeAnimationTrack);
  if (used > maximum_bytes) return {};
  for (uint32_t n=0; n<*count; ++n) {
    const uint64_t record = uint64_t(*records)+n*36;
    const auto hash = Word(record+18,read);
    if (!hash || !record_hashes.emplace(*hash,true).second) return {};
    const auto binding = mapping.find(*hash);
    if (binding == mapping.end()) continue;
    NativeAnimationTrack track; track.pose_index = binding->second;
    std::unique_ptr<NativeAnimationSplines> splines;
    for (unsigned channel=0; channel<3; ++channel) {
      const auto address = Word(record+channel*4,read);
      const auto keys_count = Half(record+12+channel*2,read);
      if (!address || !keys_count) return {};
      // Null channel means absent even if its unused count is nonzero.
      if (!*address) continue;
      if (*type == 3 && *keys_count != 1) {
        if (!splines) {
          if (sizeof(NativeAnimationSplines) > maximum_bytes-used) return {};
          used += sizeof(NativeAnimationSplines);
          splines = std::make_unique<NativeAnimationSplines>();
        }
        auto curve = ReadSplineChannel(*address,channel == 1,std::bit_cast<float>(*timing),maximum_bytes,used,read);
        if (!curve) return {};
        (channel == 0 ? splines->translation : channel == 1 ? splines->rotation : splines->scale) = std::move(*curve);
        continue;
      }
      if (!*keys_count || *keys_count > (maximum_bytes-used)/sizeof(NativeAnimationKey)) return {};
      const uint64_t stride = channel == 1 ? 8 : 16;
      if (uint64_t(*address)+uint64_t(*keys_count)*stride > uint64_t(UINT32_MAX)+1) return {};
      auto &keys = channel == 0 ? track.translation : channel == 1 ? track.rotation : track.scale;
      keys.reserve(*keys_count);
      if (keys.capacity() > (maximum_bytes-used)/sizeof(NativeAnimationKey)) return {};
      used += keys.capacity()*sizeof(NativeAnimationKey);
      for (uint32_t k=0; k<*keys_count; ++k) {
        const uint64_t key_address = uint64_t(*address)+k*stride;
        const auto frame = Half(key_address,read);
        if (!frame || int16_t(*frame) < 0) return {};
        NativeAnimationKey key; key.seconds = float(int16_t(*frame))/rate;
        for (unsigned axis=0; axis<3; ++axis) {
          if (channel == 1) {
            const auto angle = Half(key_address+2+axis*2,read);
            if (!angle) return {};
            // A complete signed turn is 65536 units. Convert to exact native
            // turn fractions now; radians only after arc selection/sampling.
            key.value[axis] = float(int16_t(*angle))*0x1p-16f;
          } else {
            const auto value = Word(key_address+4+axis*4,read);
            if (!value) return {};
            key.value[axis] = std::bit_cast<float>(*value);
          }
        }
        keys.push_back(key);
      }
      // The original T/R binary search uses an inclusive upper bound equal to
      // count. It is safe only while authored keys bracket the sample range;
      // don't import adjacent bytes as an imaginary count+1 key.
      if (keys.size() > 1 && keys.back().seconds < float(*duration)/30) return {};
    }
    track.splines = std::move(splines);
    tracks.push_back(std::move(track));
  }
  return NativeAnimationClip::Create(bindings.size(),float(*duration)/30,std::move(tracks),maximum_bytes);
}

// Import all named tracks at completed load, before any model selects a slot.
// Packed aliases refer to this same asset rather than duplicating key storage.
template <class ReadWord>
std::optional<NativeAnimationAsset> ReadKeyedAsset(uint32_t source, size_t maximum_bytes, ReadWord &&read) {
  const auto records = Word(source,read);
  const auto type = Half(uint64_t(source)+6,read), count = Half(uint64_t(source)+8,read);
  if (!records || !*records || !type || (*type != 2 && *type != 3) || !count || !*count || *count > kMaxNativeJoints ||
      uint64_t(*records)+uint64_t(*count)*36 > uint64_t(UINT32_MAX)+1 ||
      maximum_bytes < sizeof(NativeAnimationAsset)+size_t(*count)*sizeof(NativeAnimationAsset::Target)) return {};
  std::vector<JointBinding> bindings;
  std::vector<uint32_t> names;
  bindings.reserve(*count); names.reserve(*count);
  for (uint32_t n=0; n<*count; ++n) {
    const auto hash = Word(uint64_t(*records)+n*36+18,read);
    if (!hash) return {};
    bindings.push_back({n,*hash}); names.push_back(*hash);
  }
  auto clip = ReadKeyedClip(source,bindings,
      maximum_bytes-sizeof(NativeAnimationAsset)-size_t(*count)*sizeof(NativeAnimationAsset::Target),read);
  return clip ? NativeAnimationAsset::Create(std::move(*clip),std::move(names),maximum_bytes) : std::nullopt;
}

// Outgoing compatibility records only; the owned asset and sampler know no
// 48-byte layout. Whole reset clears all bytes; preserve mode keeps unmatched
// records and inactive values, clears missing matched-channel activation only,
// and ORs dirty128 for multi-key tracks. Original gameplay/late writers still
// consume these records until their complete native channel handoff is ready.
using ChannelRecord = std::array<uint32_t,12>;
inline bool ApplyKeyedAsset(const NativeAnimationAsset &asset, std::span<const uint32_t> names,
    float seconds, bool preserve, std::vector<ChannelRecord> &records) {
  if (names.empty() || names.size() > kMaxNativeJoints || (preserve && records.size() != names.size())) return false;
  std::unordered_set<uint32_t> unique;
  for (uint32_t name : names) if (!unique.insert(name).second) return false;
  std::vector<NativeJointChannels> sampled;
  if (!asset.Clip().Sample(seconds,sampled)) return false;
  auto output = preserve ? records : std::vector<ChannelRecord>(names.size());
  for (size_t n=0; n<names.size(); ++n) {
    auto &record = output[n]; record[1] = names[n];
    const auto *track = asset.FindTrack(names[n]);
    if (!track) continue;
    const auto &channel = sampled[track->pose_index];
    record[0] &= ~7u;
    auto put = [&](const auto &value, unsigned word) {
      for (float component : value) record[word++] = std::bit_cast<uint32_t>(component);
    };
    if (channel.translated) { record[0] |= 1; put(channel.translation,2); }
    if (channel.rotated) { record[0] |= 2; put(channel.rotation,5); }
    if (channel.scaled) { record[0] |= 4; put(channel.scale,9); }
    if (track->Animated()) record[0] |= 128;
  }
  records = std::move(output); return true;
}

inline bool ApplyKeyedLayer(const NativeAnimationAsset &asset, std::span<const uint32_t> names,
    std::span<const NativeSkeletonJoint> skeleton, float seconds, float weight,
    uint32_t root_pose, bool single_subtree, std::vector<ChannelRecord> &records) {
  if (names.size() != skeleton.size() || records.size() != names.size() || !std::isfinite(weight)) return false;
  const auto selected=SelectNativeAnimationSubtree(skeleton,root_pose,single_subtree);
  if (!selected) return false;
  if (std::abs(weight) < kNativeAnimationWeightEpsilon) return true;
  std::unordered_set<uint32_t> unique;
  for (auto name : names) if (!unique.insert(name).second) return false;
  std::vector<NativeJointChannels> sampled;
  if (!asset.Clip().Sample(seconds,sampled)) return false;
  auto output=records;
  for (size_t n=0; n<skeleton.size(); ++n) {
    if (!(*selected)[n]) continue;
    const auto &joint=skeleton[n];
    auto &record=output[joint.pose_index]; record[1]=names[joint.pose_index];
    const auto *track=asset.FindTrack(names[joint.pose_index]);
    if (!track) continue;
    NativeJointChannels previous, result;
    previous.translated=(record[0]&1)!=0; previous.rotated=(record[0]&2)!=0; previous.scaled=(record[0]&4)!=0;
    auto get=[&](auto &value,unsigned word) { for (auto &component : value) component=std::bit_cast<float>(record[word++]); };
    if (previous.translated) get(previous.translation,2);
    if (previous.rotated) get(previous.rotation,5);
    if (previous.scaled) get(previous.scale,9);
    if (!BlendNativeChannels(previous,sampled[track->pose_index],joint.blend_rest,weight,result)) return false;
    record[0]&=~7u;
    auto put=[&](const auto &value,unsigned word) { for (float component : value) record[word++]=std::bit_cast<uint32_t>(component); };
    if (result.translated) { record[0]|=1; put(result.translation,2); }
    if (result.rotated) { record[0]|=2; put(result.rotation,5); }
    if (result.scaled) { record[0]|=4; put(result.scale,9); }
    if (track->Animated()) record[0]|=128;
  }
  records=std::move(output); return true;
}

inline bool MixChannelRecords(std::span<const ChannelRecord> left, std::span<const ChannelRecord> right,
    float weight, bool destination_is_left, bool destination_is_right, std::vector<ChannelRecord> &records) {
  if (left.empty() || left.size() > kMaxNativeJoints || left.size() != right.size() || !std::isfinite(weight)) return false;
  std::vector<ChannelRecord> output(left.size());
  for (size_t n=0; n<left.size(); ++n) {
    auto &record=output[n]; record[0]=left[n][0]|right[n][0]; record[1]=left[n][1];
    // The temporary in-place ABI publishes union flags before rereading inputs.
    // Preserve that order here, not in the native channel API. Partial overlaps
    // between different records are rejected by the runtime adapter.
    auto decode=[&](const ChannelRecord &input,bool aliases_output) {
      const uint32_t flags=aliases_output ? record[0] : input[0];
      NativeJointChannels value;
      value.translated=(flags&1)!=0; value.rotated=(flags&2)!=0;
      value.scaled=(flags&4)!=0; value.reset_parent=(flags&64)!=0;
      auto get=[&](auto &channel,unsigned word) { for (auto &component : channel) component=std::bit_cast<float>(input[word++]); };
      if (value.translated) get(value.translation,2);
      if (value.rotated) get(value.rotation,5);
      if (value.scaled) get(value.scale,9);
      return value;
    };
    NativeJointChannels mixed;
    if (!MixNativeChannels(decode(left[n],destination_is_left),decode(right[n],destination_is_right),weight,mixed)) return false;
    auto put=[&](const auto &channel,unsigned word) { for (float value : channel) record[word++]=std::bit_cast<uint32_t>(value); };
    put(mixed.translation,2); put(mixed.rotation,5); put(mixed.scale,9);
  }
  records=std::move(output); return true;
}
} // namespace bd::gpu::scene::animation_source
