/**
 * @brief Ready-load image import and temporary exact late-writer association.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_image_animation.h"

namespace bd::gpu::scene::material_image_source {
// Only the boundary owns these associations. Evaluation receives native keys;
// source words are compared, never decoded into a second per-frame asset.
struct LoadedAnimation {
  NativeImageAnimation animation;
  struct KeyExport { uint32_t key, image; };
  struct Word { uint32_t address, value; };
  std::vector<KeyExport> exports;
  std::vector<Word> guard;
  size_t RetainedBytes() const {
    return sizeof(LoadedAnimation)+animation.keys.capacity()*sizeof(NativeImageAnimation::Key)+
        exports.capacity()*sizeof(KeyExport)+guard.capacity()*sizeof(Word);
  }
  template<class Read> bool Matches(Read read) const {
    for (const auto &word : guard) if (read(word.address) != std::optional(word.value)) return false;
    return true;
  }
  template<class Capture> bool MatchesImages(Capture capture) const {
    for (size_t n=0; n<exports.size(); ++n) {
      if (!exports[n].image) continue;
      const auto current=capture(exports[n].image);
      if (current.action != animation.keys[n].image.action || current.image != animation.keys[n].image.image) return false;
    }
    return true;
  }
  uint32_t Index(uint32_t key) const {
    for (uint32_t n=0; n<exports.size(); ++n) if (exports[n].key == key) return n;
    return UINT32_MAX;
  }
};
template<class Read, class Capture>
std::optional<LoadedAnimation> ReadAnimation(uint32_t owner, size_t budget, Read source_read, Capture capture) {
  if (!owner || (owner&3) || owner > UINT32_MAX-319 ||
      source_read(uint64_t(owner)+316) != std::optional(6u)) return {};
  LoadedAnimation result;
  auto read=[&](uint64_t address)->std::optional<uint32_t> {
    if (!address || (address&3) || address > UINT32_MAX-3) return {};
    const auto value=source_read(address);
    if (value) result.guard.push_back({uint32_t(address),*value});
    return value;
  };
  // Two vector words, the comparison constant and eleven words per key.
  const auto begin=source_read(uint64_t(owner)+80), end=source_read(uint64_t(owner)+84);
  if (!begin || !end || (*begin&3) || *end < *begin || (*end-*begin)%4 ||
      (!*begin && *end) || (*end-*begin)/4 > 4096) return {};
  const size_t count=(*end-*begin)/4;
  const size_t estimate=sizeof(LoadedAnimation)+count*(sizeof(NativeImageAnimation::Key)+
      sizeof(LoadedAnimation::KeyExport))+(3+11*count)*sizeof(LoadedAnimation::Word);
  if (estimate > budget) return {};
  result.animation.keys.reserve(count); result.exports.reserve(count); result.guard.reserve(3+11*count);
  if (read(uint64_t(owner)+80) != begin || read(uint64_t(owner)+84) != end) return {};
  if (*begin) {
    const auto zero=read(0x82055230);
    if (!zero || std::bit_cast<float>(*zero) != 0) return {};
  }
  for (uint64_t cursor=*begin; cursor<*end; cursor+=4) {
    const auto key=read(cursor);
    if (!key || !*key) return {};
    const auto start=read(uint64_t(*key)+60), duration=read(uint64_t(*key)+64), last=read(uint64_t(*key)+68);
    const auto repeat=read(uint64_t(*key)+124), loop=read(uint64_t(*key)+128), procedural=read(uint64_t(*key)+212);
    const auto texture=read(uint64_t(*key)+260);
    const auto holder=texture && *texture ? read(uint64_t(*texture)+4) : std::nullopt;
    const auto image=holder && *holder ? read(uint64_t(*holder)+24) : std::nullopt;
    if (!start || !duration || !last || !repeat || !loop || !procedural || !image) return {};
    NativeImageAnimation::Key output{{std::bit_cast<float>(*start),std::bit_cast<float>(*duration),
        std::bit_cast<float>(*last),int32_t(*repeat),*loop != 0},{MaterialImageAction::Keep},*procedural != 0};
    if (!std::isfinite(output.window.start) || !std::isfinite(output.window.duration) ||
        !std::isfinite(output.window.end)) return {};
    result.animation.keys.push_back(std::move(output)); result.exports.push_back({*key,*image});
  }
  if (result.RetainedBytes() > budget) return {};
  for (size_t n=0; n<count; ++n) {
    if (!result.exports[n].image) continue;
    result.animation.keys[n].image=capture(result.exports[n].image);
    // Missing native residency is not an immutable Unknown that can silently
    // survive a later texture upload. The original remains eligible instead.
    if (result.animation.keys[n].image.action != MaterialImageAction::Bind ||
        !result.animation.keys[n].image.image.primary) return {};
  }
  return result;
}
} // namespace bd::gpu::scene::material_image_source
