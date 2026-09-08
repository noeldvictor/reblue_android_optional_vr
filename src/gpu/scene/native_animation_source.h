/**
 * @brief Bounded import of relocated keyed skeletal clips; no source reads at sample time.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
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
} // namespace bd::gpu::scene::animation_source
