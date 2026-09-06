/**
 * @brief Checked, temporary material parameter/image selection boundary.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/scene_texture_import.h"

namespace bd::gpu::scene {
// Water slot 12 reads the table at +80, not the ordinary current/next scene
// table or the active model-reflection table. Null remains a binding no-op.
template <class Read> std::optional<uint32_t> ReadWaterSceneImage(Read read) {
  const auto table = read(kActiveTextureTable + 80);
  if (!table) return {};
  if (!*table) return 0;
  const auto active = read(kActiveTextureTable + 4);
  if (!active) return {};
  uint32_t index = 0;
  if (*table == *active) {
    const auto offset = read(kActiveTextureTable);
    if (!offset) return {};
    index = *offset;
  }
  const auto count = read(uint64_t(*table));
  if (!count) return {};
  if (index >= *count) return read(kActiveTextureTable + 32);
  const auto entries = read(uint64_t(*table) + 4);
  if (!entries || !*entries) return {};
  const uint64_t address = uint64_t(*entries) + uint64_t(index) * 28 + 24;
  return address <= UINT32_MAX - 3 ? read(address) : std::nullopt;
}
template <class Read>
std::optional<uint32_t> ReadWaterFactorDestination(uint32_t material, Read read) {
  const auto row = read(uint64_t(material) + 5052);
  const auto component = read(uint64_t(material) + 5056);
  const auto owner = read(uint64_t(material) + 5044);
  if (!row || !component || !owner || !*owner) return {};
  const auto buffer = read(uint64_t(*owner) + 12);
  if (!buffer || !*buffer) return {};
  // Preserve the source's low-word index arithmetic at this boundary only.
  const uint32_t offset = ((*row * 4u) + *component) * 4u;
  const uint64_t address = uint64_t(*buffer) + offset;
  return address <= UINT32_MAX - 3 ? std::optional(uint32_t(address)) : std::nullopt;
}
} // namespace bd::gpu::scene
