/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lighting.h"
#include "gpu/native_target_images.h"
#include "gpu/scene/native_transform.h"
#include "gpu/scene/native_visual_inputs.h"
#include <cmath>
#include <optional>
#include <utility>
namespace bd::gpu::scene {
struct NativePrimaryReceiver {
  NativeTargetImageHandle image;
  RenderMatrix world_to_shadow{};
  LightingVector colour{};
};
class NativeReceiverPublication {
  std::optional<NativePrimaryReceiver> value_;
  NativeVisualIdentity identity_;
  uint32_t frame_ = 0, view_ = 0;
public:
  void Reset() { value_.reset(); }
  void Publish(NativePrimaryReceiver value, NativeVisualIdentity identity, uint32_t frame, uint32_t view) {
    Reset();
    if (!identity || view >= 16 || !value.image) return;
    for (float item : value.colour) if (!std::isfinite(item)) return;
    for (float item : value.world_to_shadow) if (!std::isfinite(item)) return;
    value_ = std::move(value); identity_ = identity; frame_ = frame; view_ = view;
  }
  std::optional<NativePrimaryReceiver> Read(NativeVisualIdentity identity, uint32_t frame, uint32_t view) const {
    return identity == identity_ && frame == frame_ && view == view_ ? value_ : std::nullopt;
  }
};
// Temporary source-policy conversion, not fields of the retained native packet.
constexpr bool ImportReceiverParticipation(uint32_t technique, uint32_t phase) {
  return technique != 14 && (phase == 0 || phase == 3 || phase == 5 || phase == 6);
}
// Preserve the late read: the compatibility parameter publisher may alias live
// authored storage. The native packet never reads the compatibility colour cache.
template<class Adapter> void RunNativeReceiverSetup(bool enabled, Adapter &adapter) {
  adapter.Reset();
  if (!enabled) return;
  adapter.Preflight();
  adapter.BindCompatibilityImage();
  adapter.FlushCompatibilityParameters();
  const auto colour = adapter.ReadColour();
  adapter.PublishCompatibilityColour(colour);
  adapter.PublishNative(colour);
}
std::optional<NativePrimaryReceiver> FindNativePrimaryReceiver(NativeVisualIdentity identity, uint32_t view);
// Direct ordinary visual producer. Reuses late authored colour publication and
// explicit legacy exports without invoking the participant's guest ABI.
bool PrepareNativePrimaryReceiver(NativeVisualIdentity identity, uint32_t stack);
} // namespace bd::gpu::scene
