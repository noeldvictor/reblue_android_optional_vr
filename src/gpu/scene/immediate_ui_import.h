/**
 * @brief Temporary authored parameter/getter boundary for native UI submission.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace bd::gpu::scene {
// LFS -> double -> STFS quiets a signalling NaN, unlike vertex byte transport.
inline constexpr uint32_t ImmediateUiScalar(uint32_t word) {
  return (word & 0x7f800000) == 0x7f800000 && (word & 0x007fffff)
      ? word | 0x00400000 : word;
}
struct ImmediateUiImportPlan {
  struct Write { uint32_t address, word; };
  std::array<Write, 17> writes{};
  uint32_t size = 0, device = 0;
  std::array<uint32_t, 4> colour{}, translation{};
  bool translated = false;
  template <class Read> std::optional<uint32_t> Word(uint64_t address, Read read) const {
    if (!address || (address & 3) || address > UINT32_MAX - 3) return {};
    for (uint32_t i = size; i; --i)
      if (writes[i - 1].address == address) return writes[i - 1].word;
    return read(address);
  }
  template <class Read> bool Store(uint64_t address, uint32_t word, Read read) {
    if (size == writes.size() || !Word(address, read)) return false;
    writes[size++] = {uint32_t(address), word};
    return true;
  }
  template <class Store> void Apply(Store store) const {
    for (uint32_t i = 0; i < size; ++i) store(writes[i].address, writes[i].word);
  }
  template <class Read> bool Matches(Read read) const {
    for (uint32_t i = 0; i < size; ++i) {
      bool overwritten = false;
      for (uint32_t j = i + 1; j < size; ++j)
        overwritten |= writes[j].address == writes[i].address;
      if (!overwritten && read(writes[i].address) != std::optional(writes[i].word)) return false;
    }
    return true;
  }
};
// Sequential colour reads/writes deliberately see prior writes when inputs
// alias the getter mirror or lazy default. Translation is captured after the
// colour/dirty publication, before any translation destination is written.
template <class Read>
std::optional<ImmediateUiImportPlan> BuildImmediateUiImport(
    uint32_t device, uint32_t colour, uint32_t translation,
    uint32_t default_colour, uint32_t one, Read read) {
  ImmediateUiImportPlan p;
  if (!device) return {};
  p.device = device;
  const auto flags = p.Word(uint64_t(default_colour) + 16, read);
  if (!flags) return {};
  one = ImmediateUiScalar(one);
  if (!(*flags & 1)) {
    for (uint32_t i = 0; i < 4; ++i)
      if (!p.Store(uint64_t(default_colour) + i * 4, one, read)) return {};
    if (!p.Store(uint64_t(default_colour) + 16, *flags | 1, read)) return {};
  }
  if (!colour) colour = default_colour;
  for (uint32_t i = 0; i < 4; ++i) {
    const auto value = p.Word(uint64_t(colour) + i * 4, read);
    if (!value) return {};
    p.colour[i] = ImmediateUiScalar(*value);
    if (!p.Store(uint64_t(device) + 0x1730 + i * 4, p.colour[i], read)) return {};
  }
  const auto ps_dirty = p.Word(uint64_t(device) + 8, read);
  if (!ps_dirty || !p.Store(uint64_t(device) + 8, *ps_dirty | 0x80000000, read)) return {};
  // Low halves of both dirty masks are unchanged by the original OR.
  if (translation) {
    p.translated = true;
    for (uint32_t i = 0; i < 3; ++i) {
      const auto value = p.Word(uint64_t(translation) + i * 4, read);
      if (!value) return {};
      p.translation[i] = ImmediateUiScalar(*value);
    }
    p.translation[3] = one;
    for (uint32_t i = 0; i < 4; ++i)
      if (!p.Store(uint64_t(device) + 0x840 + i * 4, p.translation[i], read)) return {};
    const auto vs_dirty = p.Word(device, read);
    if (!vs_dirty || !p.Store(device, *vs_dirty | 0x04000000, read)) return {};
  }
  return p;
}
} // namespace bd::gpu::scene
