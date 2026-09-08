/**
 * @brief Bounded import of relocated keyed skeletal clips; no source reads at sample time.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_animation_clip.h"
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

// sub_82198FF8 relocates 36-byte records, sub_821995B8/99628 swap exactly
// count keys (not count+1). sub_82288680 supplies samples per 30Hz logic tick.
// Type 2: T/S = {s16 frame,pad,f32 xyz}, R = {s16 frame,s16 xyz angle}.
// Type 3 compressed curves and dense types 0/1 need distinct source contracts.
template <class ReadWord>
std::optional<NativeAnimationClip> ReadKeyedClip(uint32_t source,
    std::span<const JointBinding> bindings, size_t maximum_bytes, ReadWord &&read) {
  if (!source || bindings.empty() || bindings.size() > kMaxNativeJoints ||
      maximum_bytes < sizeof(NativeAnimationClip)) return {};
  const uint64_t header = source;
  const auto records = Word(header,read), timing = Word(header+12,read);
  const auto duration = Half(header+4,read), type = Half(header+6,read), count = Half(header+8,read);
  if (!records || !*records || !timing || !duration || !type || *type != 2 || !count ||
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
    for (unsigned channel=0; channel<3; ++channel) {
      const auto address = Word(record+channel*4,read);
      const auto keys_count = Half(record+12+channel*2,read);
      if (!address || !keys_count) return {};
      // Null channel means absent even if its unused count is nonzero.
      if (!*address) continue;
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
    tracks.push_back(std::move(track));
  }
  return NativeAnimationClip::Create(bindings.size(),float(*duration)/30,std::move(tracks),maximum_bytes);
}
} // namespace bd::gpu::scene::animation_source
