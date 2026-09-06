/**
 * @file    native_pass_dispatch.h
 * @brief   Ordered host execution of pass and participant lifecycles.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>

namespace bd::gpu::scene {
// The adapter currently imports the engine registry. Read the count AFTER
// opening the pass, then keep that iteration bound even if a callback changes
// the registry. Individual slots remain live and must not be cached across a
// callback. Native registry ownership can replace the adapter independently.
template <class Adapter> void DispatchPassBegin(Adapter &adapter) {
  adapter.PublishMode();
  adapter.BeginPass();
  const int32_t count = adapter.ParticipantCount();
  for (int32_t i = 0; i < count; ++i)
    if (adapter.BeginParticipant(uint32_t(i)))
      adapter.SetParticipantActive(uint32_t(i), true);
}

template <class Adapter> void DispatchPassEnd(Adapter &adapter) {
  const int32_t count = adapter.ParticipantCount();
  for (int32_t i = 0; i < count; ++i) {
    if (!adapter.ParticipantActive(uint32_t(i)))
      continue;
    adapter.EndParticipant(uint32_t(i));
    // Clear the live slot after the callback, not a stale object snapshot.
    adapter.SetParticipantActive(uint32_t(i), false);
  }
  // Empty registries still close the pass; targets outlive participant work.
  adapter.EndPass();
}
} // namespace bd::gpu::scene
