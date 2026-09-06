/**
 * @brief   Host effect preparation, paired cleanup and registry teardown.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
namespace bd::gpu::scene {
template <class Adapter>
int32_t PrepareEffectParticipants(Adapter &adapter) {
  const int32_t count = adapter.Count();
  int32_t result = 0;
  for (int32_t i = 0; i < count; ++i) {
    result = adapter.Begin(i);
    if (result == 3) return 3;
    if (result == 1 || result == 2) {
      // The callback may have replaced the participant at this live slot.
      adapter.SetActive(i, 1);
      if (result == 2) break;
      result = 2;
    }
  }
  // This is the last result, not an accumulated acceptance bit.
  return result;
}
template <class Adapter>
void FinishEffectParticipants(Adapter &adapter) {
  const int32_t count = adapter.Count();
  for (int32_t i = 0; i < count; ++i) {
    if (adapter.Active(i) != 1) continue;
    adapter.End(i);
    adapter.SetActive(i, 0); // re-read the live slot after the callback
  }
}
template <class Adapter>
int32_t PrepareEffectModel(Adapter &adapter) {
  const int32_t result = PrepareEffectParticipants(adapter);
  if (result == 3) return result; // leave the previous resource untouched
  const auto resource = adapter.InputResource();
  adapter.SetResource(resource); // publish even a null resource before begin
  if (resource) adapter.BeginResource(resource);
  return result; // the resource callback does not replace the technique result
}
template <class Adapter>
void FinishEffectModel(Adapter &adapter) {
  const auto resource = adapter.Resource();
  if (resource) {
    adapter.EndResource(resource);
    adapter.SetResource({});
  }
  // Snapshot the participant count after resource cleanup, not before it.
  FinishEffectParticipants(adapter);
}
template <class Adapter>
void DestroyEffectRegistry(Adapter &adapter) {
  for (int32_t group = 2; group >= 0; --group) {
    const auto storage = adapter.Storage(uint32_t(group));
    if (storage) {
      adapter.Free(storage);
      adapter.SetStorage(uint32_t(group), {});
    }
    adapter.SetCapacity(uint32_t(group), 0);
    adapter.SetCount(uint32_t(group), 0);
  }
  // Array destruction is not participant/resource destruction. Their paired
  // finish callbacks belong to the preparation scope, not this teardown.
}
} // namespace bd::gpu::scene
