/**
 * @brief Checked temporary texture-table layout and selection boundary.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause License
 */
#pragma once
#include "gpu/scene/native_texture_table.h"
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace bd::gpu::scene {
// Lock spans both collection and publication: an image event cannot land in
// between and leave an unpublished table with stale leases. The capture and
// publish callbacks must obey mirror -> table lock order, never the reverse.
template <class Mutex, class Capture, class Publish>
void PublishTextureTableSnapshot(Mutex &mutex, std::span<const uint32_t> sources,
                                Capture capture, Publish publish) {
  std::lock_guard lock(mutex);
  std::vector<NativeTextureTableSlot> slots;
  slots.reserve(sources.size());
  for (uint32_t source : sources) {
    auto binding = capture(source);
    const bool available = !source || bool(binding.primary);
    slots.push_back({std::move(binding), available});
  }
  publish(std::move(slots));
}

// hcgLoadTextureArray writes count/data at +0/+4 and texture at record+24.
// hcgTextureListRelease independently consumes this same 28-byte record layout.
template <class Read>
std::optional<std::vector<uint32_t>> ReadTextureTableSources(uint32_t table, Read read) {
  if (!table || (table & 3) || table > UINT32_MAX - 7) return {};
  const auto count = read(table), entries = read(uint64_t(table) + 4);
  if (!count || !entries || *count > 4096 ||
      (*count && (!*entries || (*entries & 3) ||
                   uint64_t(*entries) + uint64_t(*count) * 28 > uint64_t(UINT32_MAX) + 1))) return {};
  std::vector<uint32_t> sources;
  sources.reserve(*count);
  for (uint32_t i = 0; i < *count; ++i) {
    const auto source = read(uint64_t(*entries) + uint64_t(i) * 28 + 24);
    if (!source) return {};
    sources.push_back(*source);
  }
  return sources;
}
// The original adds in 32 bits, then compares unsigned. Keep that import rule
// out of the native table API, whose consumer selects an ordinary slot index.
inline uint32_t TextureTableSourceIndex(uint32_t offset, uint32_t selector) {
  return offset + selector;
}

// The asynchronous LoadTexlist poll sub_8217B3C0 returns 1 only after all
// 20-byte requests complete and every table record+24 has been assigned.
// sub_8217B050 only allocates the table; publishing at that point is too early.
template <class Read>
std::optional<uint32_t> CompletedTextureTable(uint32_t asset, uint32_t result, Read read) {
  if (result != 1 || !asset || (asset & 3) || asset > UINT32_MAX - 187) return {};
  const auto table = read(uint64_t(asset) + 184);
  return table && *table ? table : std::nullopt;
}

// Rebuild only at an image producer update. Old readers keep an immutable
// lease. The source keys are confined to this temporary association adapter.
template <class Library, class Table, class Slot>
Table RebindTextureTable(Library &library, const Table &table,
                        std::span<const uint32_t> sources, size_t source_capacity,
                        uint32_t changed, const Slot &replacement) {
  if (!table || sources.size() != table->slots.size() || source_capacity < sources.size() ||
      source_capacity > SIZE_MAX / sizeof(uint32_t)) return {};
  auto slots = table->slots;
  bool found = false;
  for (size_t i = 0; i < slots.size(); ++i)
    if (sources[i] == changed && slots[i] != replacement) { slots[i] = replacement; found = true; }
  return found ? library.Create(std::move(slots), source_capacity * sizeof(uint32_t)) : table;
}
} // namespace bd::gpu::scene
