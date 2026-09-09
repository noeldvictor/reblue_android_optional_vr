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
enum class MaterialImageSource : uint8_t { Table, Unknown, NormalTable };
struct MaterialImageAssignment {
  MaterialImageSource source = MaterialImageSource::Table;
  uint8_t channel = 0, selector = 0;
  // Authored alpha at this command, before pass overrides or later commands.
  // Phase1 only binds the base table/special/late image when this is true.
  bool shadow_alpha = false;
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
  bool native_eye = false;
};
template <class Image> struct MaterialTextureInputs {
  std::array<float, 4> initial_uv{}, reset_uv{};
  bool owns_uv = false, skip_overrides = false;
  std::vector<MaterialTextureOverride<Image>> overrides;
  std::vector<MaterialTextureOverride<Image>> late_images;
  std::optional<uint32_t> special_selector;
  MaterialImageSelection<Image> special_image;
  std::optional<uint32_t> tint_selector;
  std::array<float,4> tint{};
};
template <class Image> struct MaterialTextureValues {
  std::array<Image, 16> images{};
  uint16_t image_mask = 0;
  std::array<float, 4> uv{}, secondary_uv{};
  bool owns_uv = false;
  uint8_t native_eye_uv_mask = 0; // provenance of the offsets actually selected
  std::array<std::array<float,4>,3> colours{{{1,1,1,1},{1,1,1,1},{1,1,1,1}}};
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
    size_t max_primitives = 4096, bool shadow_phase = false) {
  if (ranges.size() > max_primitives || assignments.size() > 65536 ||
      inputs.overrides.size() > 256 || inputs.late_images.size() > 256)
    return false;
  std::vector<MaterialTextureValues<Image>> values;
  values.reserve(ranges.size());
  MaterialTextureValues<Image> state;
  state.uv = inputs.initial_uv;
  // Ordinary node setup initializes both UV pairs from the object. Subsequent
  // channel 0/1 overrides/reset affect only uv; layer 2 keeps this initial pair.
  // Do not borrow the final layer-0 offset or an earlier object's shader state.
  state.secondary_uv = inputs.initial_uv;
  state.owns_uv = inputs.owns_uv;
  std::array<bool, 2> uv_overridden{};
  bool shadow_normal_seen = false;
  std::optional<uint8_t> tinted_channel;
  size_t cursor = 0;
  for (const auto &range : ranges) {
    if (range.texture_assignment_end < cursor || range.texture_assignment_end > assignments.size())
      return false;
    while (cursor < range.texture_assignment_end) {
      const auto step = assignments[cursor++];
      if (step.channel >= 16) return false;
      MaterialImageSelection<Image> selected;
      if (step.source == MaterialImageSource::NormalTable) {
        if (step.channel != 4) return false;
        // Normal commands bypass visual/animation overrides. Phase1 maps every
        // command to selector0 BEFORE repeated-command elision. A phase0 disable
        // leaves the actual binding intact, though the old sorted scratch still
        // records its independent lookup255 for the outgoing entry binder.
        selected = shadow_phase ? (shadow_normal_seen ? MaterialImageSelection<Image>{MaterialImageAction::Keep} : lookup(0)) :
            step.selector == 255 ? MaterialImageSelection<Image>{MaterialImageAction::Keep} : lookup(step.selector);
        shadow_normal_seen = true;
      } else if (step.source == MaterialImageSource::Table) {
        bool early_image = false, uv_match = false;
        if (!inputs.skip_overrides) {
          for (const auto &entry : inputs.overrides) {
            if (entry.selector != step.selector ||
                (entry.channel && entry.channel != uint32_t(step.channel) + 1)) continue;
            if (entry.uv) {
              if (step.channel < 2) {
                for (size_t c = 0; c < 2; ++c) state.uv[step.channel * 2 + c] = (*entry.uv)[c];
                uv_overridden[step.channel] = true;
                const uint8_t bit=uint8_t(1u << step.channel);
                state.native_eye_uv_mask=(state.native_eye_uv_mask & ~bit) | (entry.native_eye ? bit : 0);
              }
              uv_match = true;
              break; // even this record's image is skipped
            }
            if (entry.replaces_image) { selected = entry.image; early_image = true; }
          }
          if (step.channel < 2 && !uv_match && uv_overridden[step.channel]) {
            for (size_t c = 0; c < 2; ++c) state.uv[step.channel * 2 + c] = inputs.reset_uv[step.channel * 2 + c];
            uv_overridden[step.channel] = false;
            state.native_eye_uv_mask &= ~uint8_t(1u << step.channel);
          }
        }
        if (!early_image && shadow_phase && (step.channel != 0 || !step.shadow_alpha)) {
          selected = {MaterialImageAction::Keep};
        } else if (!early_image) {
          selected = lookup(step.selector);
          // bdSceneNodeDrawSingle 822802DC resets all three to white. Only
          // the late table path reaches 82281A74/82281A98; early image binds
          // preserve the previous tint. The source has ONE last-tinted channel,
          // not an independently reset flag per layer.
          if (!shadow_phase && inputs.tint_selector) {
            if (*inputs.tint_selector == step.selector) {
              if (step.channel >= state.colours.size()) return false;
              state.colours[step.channel] = inputs.tint;
              tinted_channel = step.channel;
            } else if (tinted_channel == step.channel) {
              state.colours[step.channel] = {1,1,1,1};
              tinted_channel.reset();
            }
          }
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
