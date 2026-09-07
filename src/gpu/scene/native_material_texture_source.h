/**
 * @brief Checked object-level UV/image-override import; no draw-time readers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_textures.h"
#include <bit>
#include <cmath>

namespace bd::gpu::scene {
// Reader accepts checked 64-bit addresses and returns host-endian words.
// Image conversion runs at object setup, outside the Video lock. Its result is
// an owned lease, a known no-op, or explicitly unavailable, never a source key.
template <class Image, class Read, class Capture>
std::optional<MaterialTextureInputs<Image>> ReadMaterialTextureInputs(
    uint32_t visual, Read read, Capture capture) {
  if (!visual || (visual & 3) || visual > UINT32_MAX - 3751) return {};
  const uint64_t object = visual;
  const auto mode = read(object + 3000), special_route = read(object + 3128);
  if (!mode || !special_route || *mode == 11 || *special_route) return {};
  MaterialTextureInputs<Image> result;
  result.skip_overrides = *mode == 6 || *mode == 7 || *mode == 8;
  auto floats = [&](uint64_t address, auto &out) {
    for (size_t i = 0; i < out.size(); ++i) {
      const auto word = read(address + i * 4);
      if (!word) return false;
      out[i] = std::bit_cast<float>(*word);
      if (!std::isfinite(out[i])) return false;
    }
    return true;
  };
  if (!result.skip_overrides) {
    const auto uv = read(object + 3440);
    if (!uv || !floats(object + 3444, result.reset_uv)) return {};
    constexpr uint32_t defaults = (uint32_t(-32035) << 16) - 25620;
    if (!floats(*uv ? object + 3444 : defaults, result.initial_uv)) return {};
    result.owns_uv = true;
    const auto records = read(object + 3560);
    if (!records) return {};
    if (*records) {
      const auto count = read(object + 3564);
      if (!count || *count > 256) return {};
      result.overrides.reserve(*count);
      for (uint32_t i = 0; i < *count; ++i) {
        const uint64_t record = uint64_t(*records) + i * 152;
        const auto uv_on = read(record + 20), image_on = read(record + 24);
        if (!uv_on || !image_on) return {};
        if (!*uv_on && !*image_on) continue;
        const auto selector = read(record + 4), channel = read(record + 8);
        if (!selector || !channel) return {};
        MaterialTextureOverride<Image> entry;
        entry.selector = *selector; entry.channel = *channel;
        if (*uv_on) {
          std::array<float, 2> offset;
          if (!floats(record + 28, offset)) return {};
          entry.uv = offset;
        }
        if (*image_on) {
          const auto image = read(record + 84);
          if (!image) return {};
          entry.replaces_image = *image != 0;
          if (*image) entry.image = capture(*image);
        }
        result.overrides.push_back(std::move(entry));
      }
    }
  }
  const auto special_mode = read(object + 3680), special_selector = read(object + 3712);
  if (!special_mode || !special_selector) return {};
  if (*special_mode == 1) {
    const auto active = read(object + 3748);
    if (!active) return {};
    if (*active >> 24) {
      // This scene-image producer is not yet represented here. Do not retain
      // a table image when the special callback may have replaced it.
      result.special_selector = *special_selector;
    }
  }
  const auto begin = read(object + 3572);
  if (!begin) return {};
  if (*begin) {
    const auto end = read(object + 3576);
    if (!end || *end < *begin || (*end - *begin) % 84 || (*end - *begin) / 84 > 256) return {};
    const uint32_t count = (*end - *begin) / 84;
    result.late_images.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
      const uint64_t record = uint64_t(*begin) + i * 84;
      const auto active = read(record + 80);
      if (!active) return {};
      if (!*active) continue;
      const auto image = read(record + 12), selector = read(record + 8);
      if (!image || !selector) return {};
      if (*image) result.late_images.push_back({*selector, 0, {}, true, capture(*image)});
    }
  }
  return result;
}
} // namespace bd::gpu::scene
