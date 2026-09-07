/**
 * @brief Ordered material image assignments and live object UV/image overrides.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace bd::gpu::scene {
// Channels are the temporary shader interface; table selectors are import
// recipes, never persistent image identities. An unknown writer invalidates
// ownership of its channel instead of pretending the last table image survived.
enum class MaterialImageSource : uint8_t { Table, Unknown };
struct MaterialImageAssignment {
  MaterialImageSource source = MaterialImageSource::Table;
  uint8_t channel = 0, selector = 0;
};
enum class MaterialImageAction : uint8_t { Unknown, Keep, Bind };
template <class Image> struct MaterialImageSelection {
  MaterialImageAction action = MaterialImageAction::Unknown;
  Image image{};
};
template <class Image> struct MaterialTextureOverride {
  uint32_t selector = 0;
  // Zero matches every channel, otherwise channel + 1 (temporary import rule).
  uint32_t channel = 0;
  std::optional<std::array<float, 2>> uv;
  bool replaces_image = false;
  MaterialImageSelection<Image> image;
};
template <class Image> struct MaterialTextureInputs {
  std::array<float, 4> initial_uv{}, reset_uv{};
  bool owns_uv = false, skip_overrides = false;
  std::vector<MaterialTextureOverride<Image>> overrides;
  std::vector<MaterialTextureOverride<Image>> late_images;
  std::optional<uint32_t> special_selector;
  MaterialImageSelection<Image> special_image;
};
template <class Image> struct MaterialTextureValues {
  std::array<Image, 16> images{};
  uint16_t image_mask = 0;
  std::array<float, 4> uv{};
  bool owns_uv = false;
  bool operator==(const MaterialTextureValues &) const = default;
};

// Evaluate in order, once per mesh/object publication, not per draw. A known
// null bind is Keep at the import boundary: A -> null must keep A, whereas an
// unavailable non-null image invalidates A. Starting state is unknown, never a
// neighbouring object's texture. First UV match ends the early override scan;
// an earlier image override also skips the later special/animation bindings.
template <class Image, class Range, class Lookup>
bool ComposeMaterialTextures(std::span<const MaterialImageAssignment> assignments,
    std::span<const Range> ranges, const MaterialTextureInputs<Image> &inputs,
    Lookup lookup, std::vector<MaterialTextureValues<Image>> &out,
    size_t max_primitives = 4096) {
  if (ranges.size() > max_primitives || assignments.size() > 65536 ||
      inputs.overrides.size() > 256 || inputs.late_images.size() > 256)
    return false;
  std::vector<MaterialTextureValues<Image>> values;
  values.reserve(ranges.size());
  MaterialTextureValues<Image> state;
  state.uv = inputs.initial_uv;
  state.owns_uv = inputs.owns_uv;
  std::array<bool, 2> uv_overridden{};
  size_t cursor = 0;
  for (const auto &range : ranges) {
    if (range.texture_assignment_end < cursor || range.texture_assignment_end > assignments.size())
      return false;
    while (cursor < range.texture_assignment_end) {
      const auto step = assignments[cursor++];
      if (step.channel >= 16) return false;
      MaterialImageSelection<Image> selected;
      if (step.source == MaterialImageSource::Table) {
        bool early_image = false, uv_match = false;
        if (!inputs.skip_overrides) {
          for (const auto &entry : inputs.overrides) {
            if (entry.selector != step.selector ||
                (entry.channel && entry.channel != uint32_t(step.channel) + 1)) continue;
            if (entry.uv) {
              if (step.channel < 2) {
                for (size_t c = 0; c < 2; ++c) state.uv[step.channel * 2 + c] = (*entry.uv)[c];
                uv_overridden[step.channel] = true;
              }
              uv_match = true;
              break; // even this record's image is skipped
            }
            if (entry.replaces_image) { selected = entry.image; early_image = true; }
          }
          if (step.channel < 2 && !uv_match && uv_overridden[step.channel]) {
            for (size_t c = 0; c < 2; ++c) state.uv[step.channel * 2 + c] = inputs.reset_uv[step.channel * 2 + c];
            uv_overridden[step.channel] = false;
          }
        }
        if (!early_image) {
          selected = lookup(step.selector);
          if (inputs.special_selector == step.selector &&
              inputs.special_image.action != MaterialImageAction::Keep)
            selected = inputs.special_image;
          for (const auto &entry : inputs.late_images)
            if (entry.replaces_image && entry.selector == step.selector) {
              selected = entry.image;
              break;
            }
        }
      }
      const uint16_t bit = uint16_t(1u << step.channel);
      if (selected.action == MaterialImageAction::Bind) {
        state.images[step.channel] = selected.image;
        state.image_mask |= bit;
      } else if (selected.action != MaterialImageAction::Keep) {
        state.images[step.channel] = {};
        state.image_mask &= ~bit;
      }
    }
    values.push_back(state);
  }
  out = std::move(values);
  return true;
}
} // namespace bd::gpu::scene
