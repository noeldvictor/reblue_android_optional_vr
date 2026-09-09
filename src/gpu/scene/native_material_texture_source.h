/**
 * @brief Checked object-level UV/image-override import; no draw-time readers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_textures.h"
#include "gpu/scene/native_material_data.h"
#include "gpu/scene/native_material_uv.h"
#include "gpu/scene/native_material_images.h"
#include <bit>
#include <cmath>
#include <type_traits>

namespace bd::gpu::scene {
struct NativeMaterialShadowInputs {
  uint32_t texture_layers = 0;
};
// Phase1's shadowmap PS consumes only texture enable and base alpha, with a
// fixed 0.6 cutoff. It has no object/vertex colour or alpha-reference input.
// Texture mode is the complete +3068 value, not its sorted-participation bool.
template <class Read>
std::optional<NativeMaterialShadowInputs> ReadMaterialShadowInputs(uint32_t visual, Read read) {
  if (!visual || (visual & 3) || visual > UINT32_MAX - 3071) return {};
  const auto mode = read(uint64_t(visual) + 3068);
  if (!mode || *mode > 3) return {}; // larger modes retain unowned boolean state
  return NativeMaterialShadowInputs{*mode};
}
// bdSceneTreeDraw publishes colour at +3404 before its setup callbacks and
// traversal. Read the final value at traversal entry, not the earlier +3004
// source or a sibling draw's shader constants. Mode-11/special-route exclusions
// are established by ReadMaterialTextureInputs before this publication is used.
template <class Read>
std::optional<NativeMaterialObjectInputs> ReadMaterialObjectInputs(uint32_t visual, Read read) {
  if (!visual || (visual & 3) || visual > UINT32_MAX - 3419) return {};
  const auto shininess = read(uint64_t(visual) + 3044);
  const auto diffuse = read(uint64_t(visual) + 3052);
  if (!shininess || !diffuse) return {};
  NativeMaterialObjectInputs result;
  result.writes_shininess = *shininess != 0;
  result.diffuse_enabled = *diffuse != 0;
  for (uint32_t n = 0; n < 4; ++n) {
    const auto value = read(uint64_t(visual) + 3404 + n * 4);
    if (!value) return {};
    result.colour[n] = std::bit_cast<float>(*value);
    if (!std::isfinite(result.colour[n])) return {};
  }
  return result;
}

// Reader accepts checked 64-bit addresses and returns host-endian words.
// Image conversion runs at object setup, outside the Video lock. Its result is
// an owned lease, a known no-op, or explicitly unavailable, never a source key.
template <class Image, class Read, class Capture>
std::optional<MaterialTextureInputs<Image>> ReadMaterialTextureInputs(
    uint32_t visual, Read read, Capture capture, const NativeMaterialUVs *animated = nullptr,
    const NativeMaterialImages *images = nullptr) {
  if (!visual || (visual & 3) || visual > UINT32_MAX - 3751) return {};
  if (animated && !animated->Valid()) return {};
  if (images && !images->Valid()) return {};
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
      if (animated && animated->count != *count) return {};
      if (images && images->entries.size() != *count) return {};
      result.overrides.reserve(*count);
      for (uint32_t i = 0; i < *count; ++i) {
        const uint64_t record = uint64_t(*records) + i * 152;
        const auto *owned_image=images ? &images->entries[i] : nullptr;
        const auto uv_on = read(record + 20);
        const auto image_on = owned_image ? std::optional(uint32_t(owned_image->enabled)) : read(record + 24);
        if (!uv_on || !image_on) return {};
        if (!*uv_on && !*image_on) continue;
        const auto selector = owned_image ? std::optional(owned_image->selector) : read(record + 4);
        const auto channel = owned_image ? std::optional(owned_image->channel) : read(record + 8);
        if (!selector || !channel) return {};
        MaterialTextureOverride<Image> entry;
        entry.selector = *selector; entry.channel = *channel;
        if (*uv_on) {
          std::array<float, 2> offset;
          if (const auto *owned=animated ? animated->Find(i) : nullptr) {
            if (!owned->enabled || owned->selector != *selector || owned->channel != *channel) return {};
            offset=owned->uv; entry.native_animated=true;
            entry.native_eye=owned->origin == NativeMaterialUVOrigin::Eye;
          } else if (!floats(record + 28, offset)) return {};
          entry.uv = offset;
        }
        if (*image_on) {
          if (owned_image) {
            if constexpr (std::is_same_v<Image,NativeTextureBinding>) {
              entry.replaces_image=owned_image->replaces_image; entry.image=owned_image->image;
              entry.native_image=true;
            } else return {};
          } else {
            const auto image = read(record + 84);
            if (!image) return {};
            entry.replaces_image = *image != 0;
            if (*image) entry.image = capture(*image);
          }
        }
        result.overrides.push_back(std::move(entry));
      }
    }
  }
  const auto special_mode = read(object + 3680), special_selector = read(object + 3712);
  if (!special_mode || !special_selector) return {};
  if (*mode == 1 && *special_mode == 0) {
    result.tint_selector = *special_selector;
    if (!floats(object+3716,result.tint)) return {};
  }
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
