/**
 * @brief Native image selection with checked, temporary catalog/cache exports.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_images.h"
#include "gpu/scene/native_material_uv_program.h"
#include <bit>
#include <limits>

namespace bd::gpu::scene {
struct NativeImageWindow {
  float start=0, duration=0, end=0;
  int32_t repeat=0;
  bool loop=false;
};
// sub_82151E10: inclusive authored interval, indefinite zero-duration hold,
// otherwise integer-period repeat. Last matching window wins; no match keeps
// the preceding selection rather than clearing it.
inline std::optional<bool> NativeImageWindowActive(const NativeImageWindow &window, int32_t frame) {
  if (!std::isfinite(window.start) || !std::isfinite(window.duration) || !std::isfinite(window.end)) return {};
  const float time=float(frame);
  if (window.start >= 0 && window.start <= time && (window.end >= time || window.duration == 0)) return true;
  if (!window.loop || !window.repeat) return false;
  if (double(window.start) < double(INT32_MIN) || double(window.start) > double(INT32_MAX)) return {};
  const int32_t elapsed=std::bit_cast<int32_t>(uint32_t(frame)-uint32_t(int32_t(window.start)));
  if (elapsed < 0) return false;
  const float period=float(double(float(window.repeat))*double(window.duration));
  if (!std::isfinite(period) || period < 1 || double(period) > double(INT32_MAX)) return {};
  return float(elapsed % int32_t(period)) < window.duration;
}
namespace material_image_source {
struct Binding {
  uint32_t table=0, count=0;
  // Comparison/export associations only; never part of the native image owner.
  std::array<uint32_t,256> sources{};
};
struct Update {
  Binding binding;
  NativeMaterialImages images;
  struct Write { uint32_t address, value; };
  std::vector<Write> writes;
  uint32_t selected=0, held=0;
  template<class Store> void Publish(Store store) const {
    for (const auto &write : writes) store(write.address,write.value);
  }
  template<class Read> bool Matches(Read read) const {
    for (const auto &write : writes) if (read(write.address) != std::optional(write.value)) return false;
    return true;
  }
};
template<class Read>
bool Matches(uint32_t visual, const Binding &binding, const NativeMaterialImages &images, Read read) {
  if (!images.Valid() || !binding.table || binding.count != images.entries.size() ||
      read(uint64_t(visual)+3560) != std::optional(binding.table) ||
      read(uint64_t(visual)+3564) != std::optional(binding.count)) return false;
  for (uint32_t n=0; n<binding.count; ++n) {
    const uint64_t record=uint64_t(binding.table)+n*152;
    const auto &entry=images.entries[n];
    const auto enabled=read(record+24);
    if (!enabled || (*enabled != 0) != entry.enabled ||
        read(record+4) != std::optional(entry.selector) || read(record+8) != std::optional(entry.channel) ||
        (entry.enabled && read(record+84) != std::optional(binding.sources[n]))) return false;
  }
  return true;
}
template<class Read, class Capture>
std::optional<Update> Prepare(uint32_t visual, const NativeMaterialUVProgram &program, Read source_read, Capture capture) {
  if (!program.Valid() || !visual || (visual&3) || visual > UINT32_MAX-3567) return {};
  Update result;
  // A transaction-wide cap includes repeated catalog/window walks. Refuse before
  // any guest mutation, allocation callback or publication on malformed cycles.
  size_t reads=0;
  auto read=[&](uint64_t address)->std::optional<uint32_t> {
    if (++reads > 262144 || !address || (address&3) || address > UINT32_MAX-3) return {};
    for (auto it=result.writes.rbegin(); it != result.writes.rend(); ++it)
      if (it->address == address) return it->value;
    return source_read(address);
  };
  auto write=[&](uint64_t address, uint32_t value) {
    if (!read(address)) return false;
    for (auto &item : result.writes) if (item.address == address) { item.value=value; return true; }
    if (result.writes.size() >= 768) return false;
    result.writes.push_back({uint32_t(address),value}); return true;
  };
  const auto table=read(uint64_t(visual)+3560), count=read(uint64_t(visual)+3564);
  if (!table || !*table || (*table&3) || !count || *count != program.slots.size() ||
      uint64_t(*table)+uint64_t(*count)*152 > uint64_t(UINT32_MAX)+1) return {};
  result.binding.table=*table; result.binding.count=*count;
  result.images.entries.resize(*count);
  for (uint32_t n=0; n<*count; ++n) {
    const auto &slot=program.slots[n];
    auto &output=result.images.entries[n];
    output.selector=slot.selector; output.channel=slot.channel; output.enabled=slot.image_enabled;
    if (!slot.image_enabled) continue;
    const uint64_t destination=uint64_t(*table)+n*152+84;
    auto image=read(destination);
    if (!image) return {};
    if (slot.image_animation >= 0) {
      auto catalog=read(uint64_t(visual)+2264);
      if (!catalog) return {};
      if (*catalog) {
        const auto id=read(uint64_t(visual)+2212+(slot.image_animation ? 4 : 0));
        if (!id) return {};
        uint32_t owner=0;
        for (size_t visited=0; *catalog; ++visited) {
          if (visited >= 4096) return {};
          const auto candidate=read(uint64_t(*catalog)+8), kind=read(uint64_t(*catalog)+12);
          if (!candidate || !kind) return {};
          if (*candidate == *id && int32_t(*kind) == slot.image_animation) {
            const auto value=read(uint64_t(*catalog)+16);
            if (!value || !*value) return {};
            owner=*value; break;
          }
          catalog=read(uint64_t(*catalog)+4); if (!catalog) return {};
        }
        if (owner) {
          const auto state=read(uint64_t(owner)+316), begin=read(uint64_t(owner)+324), end=read(uint64_t(owner)+328);
          if (!state || !begin || !end || *end < *begin || (*end-*begin)%4 || (*end-*begin)/4 > 4096) return {};
          uint32_t chosen=0;
          if (*state == 6) {
            const auto time_word=read(uint64_t(visual)+2224), windows=read(uint64_t(owner)+80);
            if (!time_word || !windows) return {};
            const float time=std::bit_cast<float>(*time_word);
            if (!std::isfinite(time) || double(time) < double(INT32_MIN) || double(time) > double(INT32_MAX)) return {};
            if (*windows) {
              const auto zero=read(0x82055230);
              if (!zero || std::bit_cast<float>(*zero) != 0) return {};
              const auto windows_end=read(uint64_t(owner)+84);
              if (!windows_end || *windows_end < *windows || (*windows_end-*windows)%4 || (*windows_end-*windows)/4 > 4096) return {};
              for (uint64_t cursor=*windows; cursor<*windows_end; cursor+=4) {
                const auto window=read(cursor);
                if (!window || !*window) return {};
                const auto start=read(uint64_t(*window)+60), duration=read(uint64_t(*window)+64), last=read(uint64_t(*window)+68);
                const auto loop=read(uint64_t(*window)+128), repeat=read(uint64_t(*window)+124);
                if (!start || !duration || !last || !loop || !repeat) return {};
                const auto active=NativeImageWindowActive({std::bit_cast<float>(*start),std::bit_cast<float>(*duration),
                    std::bit_cast<float>(*last),int32_t(*repeat),*loop != 0},int32_t(time));
                if (!active) return {};
                if (*active) chosen=*window;
              }
            }
          }
          uint32_t selected=0;
          if (*state != 6) {
            if (!write(uint64_t(owner)+328,*begin)) return {};
          } else if (chosen) {
            const auto callback=read(uint64_t(chosen)+212), capacity=read(uint64_t(owner)+332);
            // Procedural selected-key updates and a growing scratch vector still
            // need their original side effects. Never partly execute then retry.
            if (!callback || *callback || !capacity || !*begin || *capacity < *begin || *capacity-*begin < 4 ||
                (*capacity-*begin)%4 || *end > *capacity ||
                !write(*begin,chosen) || !write(uint64_t(owner)+328,*begin+4)) return {};
            selected=chosen; ++result.selected;
          } else if (*begin && *end != *begin) {
            const auto previous=read(*begin);
            if (!previous || !*previous) return {};
            selected=*previous; ++result.held;
          }
          if (selected) {
            const auto texture=read(uint64_t(selected)+260);
            const auto holder=texture && *texture ? read(uint64_t(*texture)+4) : std::nullopt;
            const auto next=holder && *holder ? read(uint64_t(*holder)+24) : std::nullopt;
            if (!next) return {};
            if (*next) { image=*next; if (!write(destination,*image)) return {}; }
          }
        }
      }
    }
    result.binding.sources[n]=*image;
    output.replaces_image=*image != 0;
  }
  // Import leases only after all selection and outgoing writes are admissible.
  // Missing non-null images are Unknown, not an invented Keep or stale texture.
  for (uint32_t n=0; n<*count; ++n) {
    auto &entry=result.images.entries[n];
    entry.image=result.binding.sources[n] ? capture(result.binding.sources[n]) :
        MaterialImageSelection<NativeTextureBinding>{MaterialImageAction::Keep};
  }
  return result.images.Valid() ? std::optional(std::move(result)) : std::nullopt;
}
} // namespace material_image_source
} // namespace bd::gpu::scene
