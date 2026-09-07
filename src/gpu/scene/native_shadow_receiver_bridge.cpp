/**
 * @brief Receiver colour publication at the actual object callback boundary.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_shadow_receiver_bridge.h"
#include "gpu/scene/native_scene_result_bridge.h"
#include "gpu/scene/guest_scene.h"
#include "gpu/resource_bridge.h"
#include "gpu/native_texture_mirror.h"
#include "gpu/resources.h"
#include "gpu/frame_stats.h"
#include "core/memory_helpers.h"
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <cmath>
#include <bit>

REX_EXTERN(__imp__sub_82176708);
namespace bd::gpu::scene {
namespace {
constexpr uint32_t kPrimary = (uint32_t(-32035) << 16) + 24832;
constexpr uint32_t kMode = (uint32_t(-32137) << 16) + 16476;
thread_local NativeReceiverColourPublication receiver;
std::optional<uint32_t> Word(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX-3) return {};
  const auto *value = bd::mem::try_at<const be_u32>(uint32_t(address));
  return value ? std::optional(uint32_t(*value)) : std::nullopt;
}
void Publish(uint32_t source, uint32_t visual) {
  receiver.Reset();
  const auto mode = Word(kMode), tech = Word(uint64_t(visual)+3000), view = Word(kRenderViewIdVa);
  const auto output = Word(uint64_t(source)+12);
  const auto shadow = FindCompletedNativePrimaryShadow();
  if (source != kPrimary || !visual || !mode || !tech || !view || !output || !shadow ||
      *tech == 14 || (*mode != 0 && *mode != 3 && *mode != 5 && *mode != 6)) return;
  // A later resource replacement must not attach this colour to a different
  // image. Resolve the temporary header only here, never in native submission.
  const auto *image = ResolveGuestTexture(*output);
  if (!image || image->nativeImage.owner != shadow->image ||
      image->texture != shadow->image->image.get()) return;
  LightingVector colour;
  for (uint32_t n=0;n<4;++n) {
    const auto value = Word(uint64_t(source)+52+n*4);
    if (!value) return;
    colour[n] = std::bit_cast<float>(*value);
  }
  receiver.Publish(colour, visual, FrameStatFrameCount(), *view);
}
}
std::optional<LightingVector> FindNativePrimaryReceiverColour(uint32_t visual) {
  const auto view = Word(kRenderViewIdVa);
  return view ? receiver.Read(visual, FrameStatFrameCount(), *view) : std::nullopt;
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(sub_82176708) {
  const auto source = ctx.r3.u32, visual = ctx.r4.u32;
  // The remaining object callback binds/flushes compatibility consumers. Its
  // colour read happens after that flush, not at the earlier projection update.
  // Direct nodes consume the publication, not this callback or its registers.
  __imp__sub_82176708(ctx, base);
  bd::gpu::scene::Publish(source, visual);
}
