/**
 * @brief Binding-time import and temporary late-writer guard for native UV descriptors.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_uv_program.h"
#include "gpu/scene/native_material_uv_source.h"

namespace bd::gpu::scene::material_uv_source {
template<class Read>
std::optional<NativeMaterialUVProgram::Slot> ReadSlot(uint64_t record, Read read) {
  NativeMaterialUVProgram::Slot result;
  const auto selector=read(record+4), channel=read(record+8), enabled=read(record+20), driver=read(record+120);
  if (!selector || !channel || !enabled || !driver) return {};
  result.selector=*selector; result.channel=*channel; result.enabled=*enabled != 0;
  if (*driver>>24) {
    const auto joint=read(record+12);
    if (!joint) return {};
    if (int32_t(*joint) >= 0) {
      const auto rotation=read(record+16);
      if (!rotation) return {};
      result.joint=*joint;
      result.mode=*rotation ? NativeEffectUVMode::Rotation : NativeEffectUVMode::Translation;
    }
  }
  for (size_t axis=0; axis<2; ++axis) {
    const auto rate=read(record+36+axis*4), translation=read(record+44+axis*4), rotation=read(record+52+axis*4);
    if (!rate || !translation || !rotation) return {};
    result.rate[axis]=std::bit_cast<float>(*rate);
    result.translation_divisor[axis]=std::bit_cast<float>(*translation);
    result.rotation_degrees[axis]=std::bit_cast<float>(*rotation);
  }
  return result;
}
struct ProgramPublication {
  Binding binding;
  NativeMaterialUVProgram program;
};
template<class Read>
std::optional<ProgramPublication> ReadProgram(uint32_t visual, Read read) {
  if (!visual || (visual&3) || visual > UINT32_MAX-3567) return {};
  const auto table=read(uint64_t(visual)+3560), count=read(uint64_t(visual)+3564);
  if (!table || !*table || (*table&3) || !count || !*count || *count > NativeMaterialUVs::kMaxSlots ||
      uint64_t(*table)+uint64_t(*count)*152 > uint64_t(UINT32_MAX)+1) return {};
  ProgramPublication result{{*table,*count},{}};
  result.program.slots.reserve(*count);
  for (uint32_t n=0; n<*count; ++n) {
    const auto slot=ReadSlot(uint64_t(*table)+n*152,read);
    if (!slot) return {};
    result.program.slots.push_back(*slot);
  }
  for (size_t axis=0; axis<2; ++axis) {
    const auto origin=read(uint64_t(*table)+76+axis*4), minimum=read(uint64_t(*table)+60+axis*4), maximum=read(uint64_t(*table)+68+axis*4);
    if (!origin || !minimum || !maximum) return {};
    result.program.eye.origin[axis]=std::bit_cast<float>(*origin);
    result.program.eye.minimum[axis]=std::bit_cast<float>(*minimum);
    result.program.eye.maximum[axis]=std::bit_cast<float>(*maximum);
  }
  const auto unit=read(0x8208EA64);
  if (!unit) return {};
  result.program.radians_per_degree=std::bit_cast<float>(*unit);
  return result.program.Valid() ? std::optional(std::move(result)) : std::nullopt;
}
// Guard only: no new program/vector is allocated and no source value becomes
// an evaluation input. Delete this when all descriptor writers publish natively.
template<class Read>
bool MatchesProgram(uint32_t visual, Binding binding, const NativeMaterialUVProgram &program, Read read) {
  if (!program.Valid() || !binding.table || binding.count != program.slots.size() ||
      read(uint64_t(visual)+3560) != std::optional(binding.table) ||
      read(uint64_t(visual)+3564) != std::optional(binding.count) ||
      read(0x8208EA64) != std::optional(std::bit_cast<uint32_t>(program.radians_per_degree))) return false;
  for (uint32_t n=0; n<binding.count; ++n) {
    const auto slot=ReadSlot(uint64_t(binding.table)+n*152,read);
    if (!slot || !slot->Same(program.slots[n])) return false;
  }
  for (uint32_t axis=0; axis<2; ++axis)
    if (read(uint64_t(binding.table)+76+axis*4) != std::optional(std::bit_cast<uint32_t>(program.eye.origin[axis])) ||
        read(uint64_t(binding.table)+60+axis*4) != std::optional(std::bit_cast<uint32_t>(program.eye.minimum[axis])) ||
        read(uint64_t(binding.table)+68+axis*4) != std::optional(std::bit_cast<uint32_t>(program.eye.maximum[axis]))) return false;
  return true;
}
} // namespace bd::gpu::scene::material_uv_source
