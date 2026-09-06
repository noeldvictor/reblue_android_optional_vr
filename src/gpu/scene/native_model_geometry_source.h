/**
 * @file    native_model_geometry_source.h
 * @brief   Checked, load-only model table decoder with an injectable word reader.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_model_materials.h"

namespace bd::gpu::scene {

struct ModelGeometrySource {
  ModelPrimitiveSourceBinding binding;
  uint32_t vertex_count = 0, declaration_slot = 0;
};

// bdSceneGraphNodeProcess creates these tables before graph publication.
// Reader returns optional<uint32_t> in host endian. This function neither
// owns source addresses nor accesses resources/GPU state, and is never a draw
// API. Keep it at the temporary load boundary until the original loader goes.
template <typename Reader>
std::optional<ModelGeometrySource> ReadModelGeometrySource(
    uint32_t mesh, const NativeMaterialRange &range, Reader &&reader) {
  const auto word = [&](uint64_t address) -> std::optional<uint32_t> {
    if (!address || address > UINT32_MAX - 3)
      return {};
    return reader(uint32_t(address));
  };
  if (!mesh || range.stream != 0 || range.index_record == 0xffff ||
      range.vertex_record == 0xffff)
    return {};
  const auto index_count = word(uint64_t(mesh) + 4);
  const auto indices = word(uint64_t(mesh) + 8), vertices = word(uint64_t(mesh) + 16);
  const auto vertex_count = vertices && *vertices ? word(*vertices) : std::nullopt;
  if (!index_count || !indices || !*indices || !vertices || !*vertices ||
      !vertex_count || range.index_record >= *index_count || range.vertex_record >= *vertex_count)
    return {};
  const uint64_t ib_record = uint64_t(*indices) + uint64_t(range.index_record) * 8;
  const uint64_t vb_record = uint64_t(*vertices) + 4 + uint64_t(range.vertex_record) * 12;
  const auto ib = word(ib_record + 4), vb = word(vb_record + 8);
  const auto count = word(vb_record), slot = word(vb_record + 4);
  if (!ib || !*ib || !vb || !*vb || !count || !*count || !slot || !*slot)
    return {};
  return ModelGeometrySource{{*ib, *vb}, *count, *slot};
}

} // namespace bd::gpu::scene
