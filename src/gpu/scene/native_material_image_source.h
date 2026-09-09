/**
 * @brief Native image selection with checked, temporary catalog/cache exports.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_images.h"
#include "gpu/scene/native_material_uv_program.h"
#include "gpu/scene/native_image_animation_source.h"
#include "gpu/scene/native_image_catalog_source.h"

namespace bd::gpu::scene {
namespace material_image_source {
enum class Refusal { Boundary, Asset, Time, Procedural, Scratch, HeldKey, Count };
struct Trace { Refusal reason=Refusal::Boundary; uint32_t slot=0, owner=0; };
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
  uint32_t selected=0, held=0, owned_keys=0;
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
template<class Read, class Capture, class Find>
std::optional<Update> Prepare(uint32_t visual, const NativeMaterialUVProgram &program, Read source_read,
    Capture capture, Find find, const LoadedCatalog &catalog, Trace *trace=nullptr) {
  auto refuse=[&](Refusal reason)->std::optional<Update> { if (trace) trace->reason=reason; return {}; };
  if (!program.Valid() || !visual || (visual&3) || visual > UINT32_MAX-3567) return {};
  Update result;
  // A transaction-wide cap includes the remaining outgoing adapter. Refuse before
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
    if (trace) { trace->slot=n; trace->owner=0; trace->reason=Refusal::Boundary; }
    const auto &slot=program.slots[n];
    auto &output=result.images.entries[n];
    output.selector=slot.selector; output.channel=slot.channel; output.enabled=slot.image_enabled;
    if (!slot.image_enabled) continue;
    const uint64_t destination=uint64_t(*table)+n*152+84;
    auto image=read(destination);
    if (!image) return {};
    if (slot.image_animation >= 0) {
      if (!catalog.catalog.entries.empty()) {
        const auto id=read(uint64_t(visual)+2212+(slot.image_animation ? 4 : 0));
        if (!id) return {};
        const auto ordinal=catalog.catalog.Image(*id,slot.image_animation);
        const uint32_t owner=ordinal == UINT32_MAX ? 0 : catalog.exports[ordinal].owner;
        if (owner) {
          if (trace) trace->owner=owner;
          const auto state=read(uint64_t(owner)+316), begin=read(uint64_t(owner)+324), end=read(uint64_t(owner)+328);
          if (!state || !begin || !end || *end < *begin || (*end-*begin)%4 || (*end-*begin)/4 > 4096) return {};
          uint32_t chosen=UINT32_MAX;
          std::shared_ptr<const LoadedAnimation> asset;
          if (*state == 6) {
            asset=find(owner);
            if (!asset) return refuse(Refusal::Asset);
            const auto time_word=read(uint64_t(visual)+2224);
            if (!time_word) return refuse(Refusal::Time);
            const float time=std::bit_cast<float>(*time_word);
            if (!std::isfinite(time) || double(time) < double(INT32_MIN) || double(time) > double(INT32_MAX))
              return refuse(Refusal::Time);
            const auto selection=asset->animation.Select(int32_t(time));
            if (!selection) return refuse(Refusal::Time);
            chosen=*selection;
          }
          uint32_t selected=UINT32_MAX;
          if (*state != 6) {
            if (!write(uint64_t(owner)+328,*begin)) return {};
          } else if (chosen != UINT32_MAX) {
            if (asset->animation.keys[chosen].procedural) return refuse(Refusal::Procedural);
            const auto capacity=read(uint64_t(owner)+332);
            // Procedural selected-key updates and a growing scratch vector still
            // need their original side effects. Never partly execute then retry.
            if (!capacity || !*begin || *capacity < *begin || *capacity-*begin < 4 ||
                (*capacity-*begin)%4 || *end > *capacity ||
                !write(*begin,asset->exports[chosen].key) || !write(uint64_t(owner)+328,*begin+4))
              return refuse(Refusal::Scratch);
            selected=chosen; ++result.selected;
          } else if (*begin && *end != *begin) {
            const auto previous=read(*begin);
            if (!previous || !*previous) return {};
            selected=asset->Index(*previous);
            if (selected == UINT32_MAX) return refuse(Refusal::HeldKey);
            ++result.held;
          }
          if (selected != UINT32_MAX) {
            const auto next=asset->exports[selected].image;
            if (next) {
              image=next; if (!write(destination,*image)) return {};
              output.image=asset->animation.keys[selected].image;
              ++result.owned_keys;
            }
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
    if (entry.image.action != MaterialImageAction::Bind)
      entry.image=result.binding.sources[n] ? capture(result.binding.sources[n]) :
          MaterialImageSelection<NativeTextureBinding>{MaterialImageAction::Keep};
  }
  return result.images.Valid() ? std::optional(std::move(result)) : std::nullopt;
}
} // namespace material_image_source
} // namespace bd::gpu::scene
