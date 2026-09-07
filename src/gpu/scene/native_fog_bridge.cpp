/**
 * @brief Authored fog producer replacement; shader staging/flush remains an adapter.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_fog_bridge.h"
#include "gpu/scene/native_fog_source.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <cstring>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_82179270);
REX_EXTERN(__imp__sub_82179050);
REXCVAR_DECLARE(bool, bd_native_lighting);
REXCVAR_DECLARE(bool, bd_native_materials_verify);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kFirst = (uint32_t(-32035) << 16)+32120;
struct Layer { std::optional<LitFog> fog; uint32_t frame = ~0u; };
thread_local std::array<Layer, 2> current;
thread_local uint64_t revision = 0;
struct Stats {
  uint64_t updates = 0, inactive = 0, compatibility = 0, resets = 0;
  uint64_t checked = 0, wrong = 0, snapshots = 0, unavailable = 0;
  uint64_t draw_checks = 0, active_layers = 0, draw_wrong = 0;
};
thread_local Stats stats;
std::optional<uint32_t> Word(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX-3) return {};
  const auto *value = bd::mem::try_at<const be_u32>(uint32_t(address));
  return value ? std::optional(uint32_t(*value)) : std::nullopt;
}
size_t Slot(uint32_t owner) { return owner == kFirst ? 0 : owner == kFirst+220 ? 1 : 2; }
}

void NativeFogReport() {
  BD_INFO("[native-fog] {} updates {} inactive {} compatibility {} resets; {} checks wrong {}; "
          "{} object snapshots {} unavailable; {} draw checks {} active layers wrong {}; authored updates and shader staging/flush adapters remain",
      stats.updates, stats.inactive, stats.compatibility, stats.resets, stats.checked, stats.wrong,
      stats.snapshots, stats.unavailable, stats.draw_checks, stats.active_layers, stats.draw_wrong);
}
void UpdateNativeFog(PPCContext &ctx, uint8_t *base) {
  ++revision;
  const auto owner = ctx.r3.u32;
  const auto slot = Slot(owner);
  const auto plan = REXCVAR_GET(bd_native_lighting) && slot < 2 && Word(uint64_t(owner)+68) == slot+1
      ? PrepareNativeFog(owner, current[slot].fog, Word) : std::nullopt;
  if (!plan) {
    current = {}; ++stats.compatibility;
    static thread_local uint32_t reports = 0;
    if (REXCVAR_GET(bd_native_materials_verify) && reports < 8 &&
        (stats.compatibility <= 4 || stats.compatibility % 1024 == 0)) {
      ++reports;
      BD_INFO("[native-fog-refusal] owner {:08X} slot {} identity {} mode {} blend {} start {:.9g} end {:.9g}",
          owner, slot, Word(uint64_t(owner)+68).value_or(0), Word(uint64_t(owner)+60).value_or(0),
          Word(uint64_t(owner)+64).value_or(0), std::bit_cast<float>(Word(uint64_t(owner)+52).value_or(0)),
          std::bit_cast<float>(Word(uint64_t(owner)+56).value_or(0)));
    }
    __imp__sub_82179270(ctx, base);
    return;
  }
  if (REXCVAR_GET(bd_native_materials_verify)) {
    __imp__sub_82179270(ctx, base);
    ++stats.checked;
    for (size_t n = 0; n < plan->count; ++n) {
      const auto &write = plan->writes[n];
      const auto actual = Word(write.address);
      if (actual != write.word) {
        ++stats.wrong;
        BD_ERROR("[native-fog-mismatch] address {:08X} actual {:08X} expected {:08X}",
            write.address, actual.value_or(0), write.word);
        throw std::runtime_error("Native fog publication differs from original");
      }
    }
  }
  for (size_t n = 0; n < plan->count; ++n)
    bd::mem::store<uint32_t>(plan->writes[n].address, plan->writes[n].word);
  current[slot] = {plan->fog, FrameStatFrameCount()};
  ++(plan->count ? stats.updates : stats.inactive);
}
void ResetNativeFog(uint32_t owner) {
  const auto slot = Slot(owner);
  if (slot < 2) { current[slot] = {}; ++revision; ++stats.resets; }
}
uint64_t NativeFogRevision() { return revision; }
bool NativeFogIsCurrent(uint64_t expected) {
  return expected == revision && REXCVAR_GET(bd_native_lighting) &&
      current[0].fog && current[1].fog && current[0].frame == FrameStatFrameCount() &&
      current[1].frame == FrameStatFrameCount();
}
std::optional<NativeFogLayers> FindNativeFogLayers() {
  if (!NativeFogIsCurrent(revision)) {
    ++stats.unavailable; return {};
  }
  ++stats.snapshots;
  return NativeFogLayers{*current[0].fog, *current[1].fog};
}
void CheckNativeFogLayers(const NativeFogLayers &fog, const uint8_t *pixel_constants, uint32_t pixel_bools) {
  if (!(pixel_bools & (1u << 6))) return; // normal material's fog-enable input
  ++stats.draw_checks;
  bool same = true;
  for (size_t n = 0; n < fog.size(); ++n) {
    const auto &layer = fog[n];
    const auto mode = (pixel_bools >> (20+n*4)) & 15;
    same &= bool(mode & 1) == layer.disabled;
    if (layer.disabled) continue;
    ++stats.active_layers;
    same &= bool(mode & 2) == layer.radial &&
        ((mode & 4) ? LitFogBlend : (mode & 8) ? LitFogAdd : LitFogSubtract) == layer.blend;
    std::array<float, 12> actual;
    std::memcpy(actual.data(), pixel_constants+(32+n*3)*16, sizeof(actual));
    const std::array<float, 12> expected{layer.direction.x, layer.direction.y, layer.direction.z, layer.start,
        layer.origin.x, layer.origin.y, layer.origin.z, layer.end,
        layer.colour.x, layer.colour.y, layer.colour.z, layer.opacity};
    same &= actual == expected;
  }
  if (!same) {
    ++stats.draw_wrong;
    BD_ERROR("[native-fog-mismatch] object fog differs at normal-lit draw");
    throw std::runtime_error("Native fog differs at draw consumption");
  }
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(sub_82179270) { bd::gpu::scene::UpdateNativeFog(ctx, base); }
REX_HOOK_RAW(sub_82179050) {
  bd::gpu::scene::ResetNativeFog(ctx.r3.u32);
  __imp__sub_82179050(ctx, base);
}
