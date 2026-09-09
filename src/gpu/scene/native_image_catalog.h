/**
 * @brief Authored image/effect selection, independent of source storage.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

namespace bd::gpu::scene {
struct NativeImageCatalog {
  struct Entry { uint32_t id; int32_t kind; };
  std::vector<Entry> entries;
  std::vector<uint32_t> images, cues;
  void Index() {
    images.resize(entries.size()); cues.resize(entries.size());
    std::iota(images.begin(),images.end(),0u); std::iota(cues.begin(),cues.end(),0u);
    std::sort(images.begin(),images.end(),[&](uint32_t a,uint32_t b) {
      if (entries[a].id != entries[b].id) return entries[a].id < entries[b].id;
      if (entries[a].kind != entries[b].kind) return entries[a].kind < entries[b].kind;
      return a < b;
    });
    std::sort(cues.begin(),cues.end(),[&](uint32_t a,uint32_t b) {
      return entries[a].id != entries[b].id ? entries[a].id < entries[b].id : a < b;
    });
  }
  uint32_t Image(uint32_t id, int32_t kind) const {
    const auto found=std::lower_bound(images.begin(),images.end(),Entry{id,kind},[&](uint32_t a,Entry b) {
      return entries[a].id != b.id ? entries[a].id < b.id : entries[a].kind < b.kind;
    });
    return found != images.end() && entries[*found].id == id && entries[*found].kind == kind ? *found : UINT32_MAX;
  }
  uint32_t Cue(uint32_t id) const {
    const auto found=std::lower_bound(cues.begin(),cues.end(),id,[&](uint32_t a,uint32_t b) { return entries[a].id < b; });
    return found != cues.end() && entries[*found].id == id ? *found : UINT32_MAX;
  }
};
} // namespace bd::gpu::scene
