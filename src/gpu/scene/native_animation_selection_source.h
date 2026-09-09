/**
 * @brief Ready authored selection -> native slot plan, with no asynchronous replay.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_animation_controller.h"

namespace bd::gpu::scene::animation_source {
struct ReadySelection { uint32_t entry=0, source=0; };
// The catalog is still a source boundary, not a permanent native address index.
// First matching ID wins even when it is pending/null. Never continue to a later
// duplicate. Pending state1 requires a side-effecting poll, so refuse BEFORE
// that poll and let the complete original selection execute exactly once.
template<class ReadWord>
std::optional<ReadySelection> ReadReadySelection(uint32_t catalog, uint32_t id, ReadWord &&read) {
  if (!catalog || (catalog&3)) return {};
  auto node=read(uint64_t(catalog)+8);
  for (size_t visited=0; node && *node && visited<4096; ++visited) {
    if ((*node&3) || uint64_t(*node)+40 > UINT32_MAX) return {};
    const auto name=read(uint64_t(*node)+8);
    if (!name) return {};
    if (*name == id) {
      const auto state=read(uint64_t(*node)+36);
      if (!state || *state == 1) return {};
      const auto source=read(uint64_t(*node)+12);
      if (!source) return {};
      return *source ? ReadySelection{*node,*source} : ReadySelection{};
    }
    node=read(uint64_t(*node)+4);
  }
  return node && !*node ? std::optional(ReadySelection{}) : std::nullopt;
}
struct SlotSelectionUpdate {
  uint32_t destination=0;
  ReadySelection selected;
  std::array<uint32_t,14> before{}, after{};
  bool restart=false;
};
template<class ReadWord>
std::optional<SlotSelectionUpdate> PrepareSlotSelection(uint32_t visual, uint32_t slot,
    uint32_t id, uint32_t loop, bool force, double weight, ReadWord &&read) {
  if (!visual || (visual&3) || slot >= kNativeAnimationSlots || uint64_t(visual)+2260 > UINT32_MAX) return {};
  const auto selected=ReadReadySelection(visual+2248,id,read);
  if (!selected) return {};
  SlotSelectionUpdate result; result.selected=*selected;
  result.destination=visual+1872+slot*56;
  for (size_t w=0; w<result.before.size(); ++w) {
    const auto word=read(uint64_t(result.destination)+w*4);
    if (!word) return {};
    result.before[w]=*word;
  }
  const auto plan=PlanNativeAnimationSelection(result.before[12],selected->entry,force,weight,std::bit_cast<float>(result.before[3]));
  if (!plan) return {};
  result.restart=plan->restart; result.after=result.before;
  if (plan->restart) {
    result.after[0]=id; result.after[1]=loop; result.after[2]=0;
    result.after[3]=std::bit_cast<uint32_t>(plan->weight);
    result.after[6]=0; result.after[12]=selected->entry;
  }
  return result;
}
} // namespace bd::gpu::scene::animation_source
