/**
 * @file    native_model_shadow_source.h
 * @brief   Load-time adapter from model controls to owned shadow policy.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_shadow.h"

namespace bd::gpu::scene {

// Reader returns a host-endian optional<uint32_t>. Missing table metadata is
// unknown; an explicitly null table is the original command's no-op. No source
// address, packed control word or reader survives in the resulting policy.
template <typename Reader>
std::optional<NativeMaterialControl> ReadModelMaterialControl(std::optional<uint32_t> table,
                                                            uint16_t control_record, Reader &&read) {
  if (!table)
    return {};
  if (!*table || control_record == 0xffff)
    return NativeMaterialControl{};
  if (control_record > 0x0fff)
    return {};
  // E000 selects a 16-byte asset record. sub_8228AB40 dispatches
  // sub_8228AAB0 for present bit 0; payload bit 3 disables shadow receiving.
  const uint64_t address = uint64_t(*table) + uint64_t(control_record) * 16;
  if (address > UINT32_MAX - 7)
    return {};
  const auto present = read(uint32_t(address));
  const auto flags = read(uint32_t(address + 4));
  if (!present || !flags)
    return {};
  const uint32_t enabled = (*present & 1) ? *flags : 0;
  // Even absent bit0 restores defaults. A null table is the distinct no-op.
  // The second handler writes only output+4, not these four switch bytes.
  return NativeMaterialControl{true, bool(enabled & 1), bool(enabled & 2), bool(enabled & 8)};
}
template <typename Reader>
NativeShadowPolicy ReadModelShadowPolicy(std::optional<uint32_t> table,
                                         uint16_t control_record, Reader &&read) {
  const auto control = ReadModelMaterialControl(table, control_record, std::forward<Reader>(read));
  if (!control) return NativeShadowPolicy::Unknown;
  return control->disable_shadow ? NativeShadowPolicy::Disabled : NativeShadowPolicy::Receive;
}

} // namespace bd::gpu::scene
