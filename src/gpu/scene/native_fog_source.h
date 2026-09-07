/**
 * @brief Checked authored fog publication and temporary staging write plan.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_fog.h"
#include <bit>
#include <cstddef>
#include <cstdint>

namespace bd::gpu::scene {
struct NativeFogWrite { uint32_t address = 0, word = 0; };
struct NativeFogPublication {
  std::optional<LitFog> fog;
  std::array<NativeFogWrite, 16> writes{};
  size_t count = 0;
};
// sub_82179270: inactive updates preserve the last published value. That is
// different from fog mode 0, which publishes an explicitly disabled layer.
template <class Read>
std::optional<NativeFogPublication> PrepareNativeFog(uint32_t owner,
    const std::optional<LitFog> &previous, Read read) {
  if (!owner || (owner & 3) || owner > UINT32_MAX-219) return {};
  NativeFogPublication result;
  std::array<uint32_t, 64> dependencies{};
  size_t dependency_count = 0;
  bool valid = true;
  const auto word = [&](uint64_t address) {
    if (!address || (address & 3) || address > UINT32_MAX-3 || dependency_count == dependencies.size()) {
      valid = false; return 0u;
    }
    const auto value = read(address);
    if (!value) valid = false;
    dependencies[dependency_count++] = uint32_t(address);
    return value.value_or(0);
  };
  const auto enabled = word(uint64_t(owner)+8) >> 24;
  if (!valid) return {};
  if (!enabled) { result.fog = previous; return result; }
  std::array<uint32_t, 12> values;
  for (size_t n = 0; n < values.size(); ++n) values[n] = word(uint64_t(owner)+12+n*4);
  const auto mode = word(uint64_t(owner)+60), blend = word(uint64_t(owner)+64);
  const auto scalar = [&](size_t n) { return std::bit_cast<float>(values[n]); };
  const auto xyz = [&](size_t n) { return LitVec(scalar(n), scalar(n+1), scalar(n+2)); };
  result.fog = ComposeNativeFog({xyz(0), xyz(3), xyz(6), scalar(9), scalar(10), scalar(11),
      mode == 0, mode == 1, blend == 0 ? LitFogBlend : blend == 1 ? LitFogAdd : LitFogSubtract});
  if (!valid || !result.fog) return {};
  const auto destination = [&](uint32_t descriptor, uint32_t stride) {
    const auto buffer = word(uint64_t(owner)+descriptor+4);
    const auto index = word(uint64_t(owner)+descriptor+12);
    const auto begin = buffer ? word(uint64_t(buffer)+12) : 0;
    if (!begin) valid = false;
    return uint64_t(begin)+uint64_t(index)*stride;
  };
  const auto write = [&](uint64_t address, uint32_t value) {
    if (!address || (address & 3) || address > UINT32_MAX-3 || result.count == result.writes.size() ||
        !read(address)) { valid = false; return; }
    result.writes[result.count++] = {uint32_t(address), value};
  };
  const auto direction = destination(96, 16), origin = destination(116, 16), colour = destination(136, 16);
  for (size_t n = 0; n < 3; ++n) write(direction+n*4, values[n]);
  for (size_t n = 0; n < 3; ++n) write(origin+n*4, values[n+3]);
  for (size_t n = 0; n < 4; ++n) write(colour+n*4, values[n+6]);
  write(direction+12, values[10]); write(origin+12, values[11]);
  const std::array<uint32_t, 4> modes{mode == 0, mode == 1, blend == 0, blend == 1};
  for (uint32_t n = 0; n < 4; ++n) write(destination(156+n*16, 4), modes[n]);
  if (!valid || result.count != result.writes.size()) return {};
  for (size_t i = 0; i < result.count; ++i) {
    // The original rereads source/descriptor fields between writes. Do not
    // snapshot aliases, including the slot identity used by the owner bridge.
    if (result.writes[i].address >= owner && uint64_t(result.writes[i].address) < uint64_t(owner)+220) return {};
    for (size_t n = 0; n < dependency_count; ++n)
      if (result.writes[i].address == dependencies[n]) return {};
    for (size_t n = 0; n < i; ++n)
      if (result.writes[i].address == result.writes[n].address) return {};
  }
  return result;
}
} // namespace bd::gpu::scene
