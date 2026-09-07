/**
 * @brief Owned primitive winding and direct/deferred participation.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace bd::gpu::scene {
enum class PrimitiveWinding : uint8_t { Pass, Reverse, TwoSided };
enum class PrimitiveCull : uint8_t { None, Front, Back };
enum class PrimitivePolicyOperation : uint8_t { Alpha, Texture };
struct PrimitivePolicyStep {
  PrimitivePolicyOperation operation = PrimitivePolicyOperation::Alpha;
  uint8_t value = 0, channel = 0;
};
// Unchanged means an early image override skipped texture-dependent routing.
// Volume effects require additional authored producers, not an opaque default.
enum class PrimitiveTextureClass : uint8_t { Unchanged, Ordinary, Volume, Unknown };
struct PrimitivePolicyInputs {
  uint32_t phase = 0, pass_mode = 0, technique = 0;
  PrimitiveCull pass_cull = PrimitiveCull::None;
  bool wind_rejects_shadow = false, wind_forces_sorted = false;
  bool special_shadow_block = false, shadow_modulates_colour = false;
  bool texture_effects = false;
};
struct NativePrimitivePolicy {
  PrimitiveCull cull = PrimitiveCull::None;
  bool routing_known = false, direct = false, deferred = false;
  bool alpha_test = false, shadow_allowed = true;
  bool operator==(const NativePrimitivePolicy &) const = default;
};
inline PrimitiveCull ResolvePrimitiveCull(PrimitiveWinding winding, PrimitiveCull basis) {
  if (winding == PrimitiveWinding::TwoSided) return PrimitiveCull::None;
  if (winding != PrimitiveWinding::Reverse || basis == PrimitiveCull::None) return basis;
  return basis == PrimitiveCull::Front ? PrimitiveCull::Back : PrimitiveCull::Front;
}

// Pure semantic program: no source words, renderer globals or captured draws.
// Preserve ordered shadow eligibility: a later zero alpha command does not
// reset a preceding rejection. Pass overrides affect participation, not that
// underlying command state. Resolve each texture class from the owned table.
template <class Range, class TextureClass>
bool ComposePrimitivePolicies(std::span<const PrimitivePolicyStep> steps,
    std::span<const Range> ranges, const PrimitivePolicyInputs &inputs,
    TextureClass texture_class, std::vector<NativePrimitivePolicy> &out,
    size_t max_primitives = 4096) {
  if (ranges.size() > max_primitives || steps.size() > 65536 || inputs.phase > 2) return false;
  std::vector<NativePrimitivePolicy> values;
  values.reserve(ranges.size());
  bool alpha = inputs.technique == 14, sorted = false, shadow = true;
  bool texture_route_known = true;
  size_t cursor = 0;
  for (const auto &range : ranges) {
    if (range.policy_step_end < cursor || range.policy_step_end > steps.size()) return false;
    while (cursor < range.policy_step_end) {
      const auto step = steps[cursor++];
      if (step.operation == PrimitivePolicyOperation::Alpha) {
        const uint8_t command = inputs.phase == 2 ? 0 : step.value;
        alpha = (command & 15) != 0;
        sorted = false;
        if (alpha) {
          shadow = true;
          sorted = !(command & 16);
          if (inputs.technique == 3) {
            if (inputs.wind_rejects_shadow) shadow = false;
            if (inputs.wind_forces_sorted) sorted = true;
          }
          if (inputs.special_shadow_block) shadow = false;
          if (inputs.phase == 1 && inputs.shadow_modulates_colour) sorted = true;
          if (inputs.technique == 8) sorted = false;
          if (inputs.technique == 9) sorted = true;
        }
        if (inputs.technique == 14) { alpha = true; sorted = false; }
      } else if (step.operation == PrimitivePolicyOperation::Texture) {
        if (step.channel >= 16) return false;
        if (inputs.texture_effects) {
          switch (texture_class(step)) {
          case PrimitiveTextureClass::Unchanged: break;
          case PrimitiveTextureClass::Ordinary: texture_route_known = true; break;
          case PrimitiveTextureClass::Volume:
          case PrimitiveTextureClass::Unknown: texture_route_known = false; break;
          }
        }
      } else return false;
    }
    bool pass_alpha = alpha, pass_sorted = sorted;
    if (inputs.pass_mode == 1) pass_sorted = false;
    if (inputs.pass_mode == 2) { pass_alpha = true; pass_sorted = true; }
    const bool suppressed = (pass_alpha && inputs.pass_mode == 3) ||
                            (inputs.phase == 1 && !shadow);
    values.push_back({ResolvePrimitiveCull(range.winding, inputs.pass_cull),
        texture_route_known, texture_route_known && !suppressed && !pass_sorted,
        texture_route_known && !suppressed && pass_sorted, pass_alpha, shadow});
  }
  out = std::move(values);
  return true;
}

struct NativePrimitivePlan {
  uint64_t stamp = 0;
  uint32_t direct = 0, deferred = 0, suppressed = 0;
  bool known = true;
};
inline NativePrimitivePlan SummarizePrimitivePlan(std::span<const NativePrimitivePolicy> values) {
  NativePrimitivePlan plan;
  plan.stamp = 14695981039346656037ull;
  for (const auto &value : values) {
    plan.known &= value.routing_known;
    plan.direct += value.direct; plan.deferred += value.deferred;
    plan.suppressed += value.routing_known && !value.direct && !value.deferred;
    // Deferred compatibility entries still contain their imported winding.
    // Include it until that consumer accepts the named native field too.
    const uint8_t bits = value.routing_known | (value.direct << 1) |
        (value.deferred << 2) | (value.alpha_test << 3) | (value.shadow_allowed << 4) |
        (uint8_t(value.cull) << 5);
    plan.stamp = (plan.stamp ^ bits) * 1099511628211ull;
  }
  plan.stamp = (plan.stamp ^ values.size()) * 1099511628211ull;
  return plan;
}
inline bool PrimitivePlanMatches(std::optional<uint64_t> captured,
                                 const std::optional<NativePrimitivePlan> &current) {
  if (!current || !current->known) return !captured;
  return captured && *captured == current->stamp;
}
} // namespace bd::gpu::scene
