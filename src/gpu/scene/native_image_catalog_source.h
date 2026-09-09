/**
 * @brief Bind-owned image catalog and temporary lifetime/export associations.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_image_catalog.h"
#include <optional>

namespace bd::gpu::scene::material_image_source {
struct LoadedCatalog {
  NativeImageCatalog catalog;
  uint64_t instance=0, generation=0;
  struct Export { uint32_t node, owner; };
  struct Word { uint32_t address, value; };
  std::vector<Export> exports;
  std::vector<Word> guard;
  size_t RetainedBytes() const {
    return sizeof(LoadedCatalog)+catalog.entries.capacity()*sizeof(NativeImageCatalog::Entry)+
        (catalog.images.capacity()+catalog.cues.capacity())*sizeof(uint32_t)+
        exports.capacity()*sizeof(Export)+guard.capacity()*sizeof(Word);
  }
  template<class Read> bool Matches(uint64_t identity, uint64_t model_generation, Read read) const {
    if (!identity || instance != identity || generation != model_generation) return false;
    for (const auto &word : guard) if (read(word.address) != std::optional(word.value)) return false;
    return true;
  }
};
template<class Read>
std::optional<LoadedCatalog> ReadCatalog(uint32_t visual, uint64_t instance, uint64_t generation,
    size_t budget, Read source_read) {
  if (!visual || (visual&3) || visual > UINT32_MAX-2267 || !instance || !generation) return {};
  const auto count=source_read(uint64_t(visual)+2260), head=source_read(uint64_t(visual)+2264);
  if (!count || !head || *count > 4096 || (*count == 0) != (*head == 0)) return {};
  const size_t estimate=sizeof(LoadedCatalog)+*count*(sizeof(NativeImageCatalog::Entry)+
      2*sizeof(uint32_t)+sizeof(LoadedCatalog::Export))+(2+4*size_t(*count))*sizeof(LoadedCatalog::Word);
  if (estimate > budget) return {};
  LoadedCatalog result; result.instance=instance; result.generation=generation;
  result.catalog.entries.reserve(*count); result.catalog.images.reserve(*count); result.catalog.cues.reserve(*count);
  result.exports.reserve(*count); result.guard.reserve(2+4*size_t(*count));
  auto read=[&](uint64_t address)->std::optional<uint32_t> {
    if (!address || (address&3) || address > UINT32_MAX-3) return {};
    const auto value=source_read(address);
    if (value) result.guard.push_back({uint32_t(address),*value});
    return value;
  };
  if (read(uint64_t(visual)+2260) != count || read(uint64_t(visual)+2264) != head) return {};
  uint32_t node=*head;
  for (uint32_t n=0; n<*count; ++n) {
    if (!node || (node&3) || node > UINT32_MAX-19) return {};
    const auto next=read(uint64_t(node)+4), id=read(uint64_t(node)+8), kind=read(uint64_t(node)+12), owner=read(uint64_t(node)+16);
    if (!next || !id || !kind || !owner || !*owner || (*owner&3) || *owner > UINT32_MAX-319) return {};
    result.catalog.entries.push_back({*id,int32_t(*kind)}); result.exports.push_back({node,*owner});
    node=*next;
  }
  if (node) return {}; // mismatched count/cycle: never publish a partial catalog
  result.catalog.Index();
  return result.RetainedBytes() <= budget ? std::optional(std::move(result)) : std::nullopt;
}
} // namespace bd::gpu::scene::material_image_source
