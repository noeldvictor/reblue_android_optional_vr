/**
 * @brief Explicit graphics descriptor bindings, independent of shader register ABI.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace plume { struct RenderPipelineLayout; struct RenderDescriptorSet; }
namespace bd::gpu {
// A bounded value snapshot, not a lookup into the currently selected device.
// Layout/sets and their buffers/images must be retained by the producer through
// the submission fence. Offsets are owned here; no borrowed stack arrays.
// Root descriptors/push constants are not represented by this Vulkan contract.
struct GraphicsBindings {
  static constexpr uint32_t kMaxSets = 4, kMaxOffsets = 8;
  const plume::RenderPipelineLayout *layout = nullptr;
  std::array<plume::RenderDescriptorSet *, kMaxSets> sets{};
  std::array<uint32_t, kMaxOffsets> offsets{};
  std::array<uint8_t, kMaxSets> dynamic_counts{};
  uint8_t set_count = 0;

  bool Valid() const {
    if (!layout || set_count > kMaxSets) return false;
    uint32_t count = 0;
    for (uint32_t i = 0; i < set_count; ++i) {
      if (!sets[i] && dynamic_counts[i]) return false;
      count += dynamic_counts[i];
    }
    return count <= kMaxOffsets;
  }
  // Only use after Valid(). Dynamic offsets are concatenated in set order.
  std::span<const uint32_t> Offsets(uint32_t set) const {
    uint32_t first = 0;
    for (uint32_t i = 0; i < set; ++i) first += dynamic_counts[i];
    return {offsets.data() + first, dynamic_counts[set]};
  }
  bool Matches(const GraphicsBindings &other, uint32_t ignore_offset = kMaxOffsets) const {
    if (!Valid() || !other.Valid() || layout != other.layout || set_count != other.set_count)
      return false;
    uint32_t count = 0;
    for (uint32_t i = 0; i < set_count; ++i) {
      if (sets[i] != other.sets[i] || dynamic_counts[i] != other.dynamic_counts[i]) return false;
      count += dynamic_counts[i];
    }
    for (uint32_t i = 0; i < count; ++i)
      if (i != ignore_offset && offsets[i] != other.offsets[i]) return false;
    return true;
  }
  // Runtime grouping only. Hash semantic fields, never struct padding or the
  // unused tail. Instanced adapters can omit precisely their per-object offset.
  uint64_t Key(uint32_t ignore_offset = kMaxOffsets) const {
    if (!Valid()) return 0;
    uint64_t hash = 14695981039346656037ull;
    auto word = [&](uint64_t value) {
      for (unsigned i = 0; i < 8; ++i) {
        hash = (hash ^ uint8_t(value)) * 1099511628211ull;
        value >>= 8;
      }
    };
    word(reinterpret_cast<uintptr_t>(layout)); word(set_count);
    uint32_t count = 0;
    for (uint32_t i = 0; i < set_count; ++i) {
      word(reinterpret_cast<uintptr_t>(sets[i])); word(dynamic_counts[i]);
      count += dynamic_counts[i];
    }
    for (uint32_t i = 0; i < count; ++i) word(i == ignore_offset ? 0 : offsets[i]);
    return hash;
  }
};

struct GraphicsBindingState {
  GraphicsBindings current;
  uint64_t descriptor_binds = 0, layout_binds = 0;
  bool valid = false;
};

// Invalid input cannot partially change command state. A layout switch forces
// all required sets to be rebound, even when pointer/offset values match.
template <class Commands>
bool ApplyGraphicsBindings(Commands &commands, const GraphicsBindings &bindings,
                           GraphicsBindingState &state) {
  if (!bindings.Valid()) return false;
  const bool changed_layout = !state.valid || bindings.layout != state.current.layout;
  if (changed_layout) {
    commands.setGraphicsPipelineLayout(bindings.layout);
    ++state.layout_binds;
  }
  for (uint32_t set = 0; set < bindings.set_count; ++set) {
    if (!bindings.sets[set]) continue;
    const auto offsets = bindings.Offsets(set);
    bool changed = changed_layout || set >= state.current.set_count ||
        bindings.sets[set] != state.current.sets[set] ||
        bindings.dynamic_counts[set] != state.current.dynamic_counts[set];
    if (!changed) {
      const auto previous = state.current.Offsets(set);
      for (uint32_t i = 0; i < offsets.size(); ++i) changed |= offsets[i] != previous[i];
    }
    if (!changed) continue;
    if (offsets.empty()) commands.setGraphicsDescriptorSet(bindings.sets[set], set);
    else commands.setGraphicsDescriptorSetDynamic(bindings.sets[set], set, offsets.data(), uint32_t(offsets.size()));
    ++state.descriptor_binds;
  }
  state.current = bindings;
  state.valid = true;
  return true;
}
} // namespace bd::gpu
