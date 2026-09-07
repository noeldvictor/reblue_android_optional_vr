/**
 * @brief Host authored-light publication; remaining shader staging is an adapter.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_selected_lights_source.h"
#include "gpu/scene/native_lighting_bridge.h"
#include "gpu/scene/native_material_texture_bridge.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <cstring>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_8218B0F0);
REXCVAR_DECLARE(bool, bd_native_lighting);
REXCVAR_DECLARE(bool, bd_native_materials_verify);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kManager = (uint32_t(-32030) << 16) - 31092;
constexpr uint32_t kPublisher = (uint32_t(-32030) << 16) + 18164;
constexpr uint32_t kView = (uint32_t(-32035) << 16) - 26424;
constexpr uint32_t kStrength = (uint32_t(-32247) << 16) - 5576;
constexpr uint32_t kOne = (uint32_t(-32251) << 16) + 20908;
constexpr uint32_t kZero = (uint32_t(-32251) << 16) + 21040;
thread_local SelectedLightSourceState current;
thread_local uint32_t current_selection = 0, current_frame = ~0u;
struct Stats {
  uint64_t publications = 0, changes = 0, compatibility = 0, checked = 0, wrong = 0;
  uint64_t snapshots = 0, unavailable = 0;
  uint64_t draw_checks = 0, draw_wrong = 0;
};
thread_local Stats stats;
std::optional<uint32_t> Word(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX-3) return {};
  const auto *value = bd::mem::try_at<const be_u32>(uint32_t(address));
  return value ? std::optional(uint32_t(*value)) : std::nullopt;
}
void Report() {
  BD_INFO("[native-selected-lights] {} publications {} changed slots {} compatibility; "
          "{} checks wrong {}; {} object snapshots {} unavailable; {} draw checks wrong {}; authored selection and shader staging adapters remain",
      stats.publications, stats.changes, stats.compatibility, stats.checked, stats.wrong,
      stats.snapshots, stats.unavailable, stats.draw_checks, stats.draw_wrong);
}
bool Publish(PPCContext &ctx, uint8_t *base) {
  const auto selection = ctx.r4.u32;
  if (ctx.r3.u32 != kPublisher || ctx.r1.u32 < 176 || (ctx.r1.u32 & 15)) return false;
  const auto view = Word(kView), count = Word(kManager+46816), numerator = Word(kStrength);
  if (!view || !count || !numerator || Word(kOne) != 0x3f800000u || Word(kZero) != 0u) return false;
  const auto scratch = uint64_t(ctx.r1.u32)-176;
  const auto safe_word = [&](uint64_t address) -> std::optional<uint32_t> {
    if (address < ctx.r1.u32 && address+4 > scratch) return {};
    return Word(address);
  };
  const auto publication = PrepareSelectedLights(kPublisher, selection, *view, kManager+24016,
      *count, std::bit_cast<float>(*numerator), current, safe_word,
      [](double angle) { return std::cos(angle); });
  if (!publication) return false;
  for (size_t n = 0; n < publication->count; ++n) {
    const auto address = publication->writes[n].address;
    if (address == kView || address == kManager+46816 || address == kStrength ||
        address == kOne || address == kZero) return false;
  }
  if (REXCVAR_GET(bd_native_materials_verify)) {
    __imp__sub_8218B0F0(ctx, base);
    ++stats.checked;
    for (size_t n = 0; n < publication->count; ++n) {
      const auto &write = publication->writes[n];
      const auto actual = Word(write.address);
      bool same = actual && *actual == write.word;
      if (!same && actual && write.cosine) {
        const auto a = std::bit_cast<float>(*actual), b = std::bit_cast<float>(write.word);
        same = std::isfinite(a) && std::isfinite(b) && std::abs(a-b) <= 2e-6f;
      }
      if (!same) {
        ++stats.wrong;
        BD_ERROR("[native-selected-light-mismatch] address {:08X} actual {:08X} expected {:08X} cosine {}",
            write.address, actual.value_or(0), write.word, write.cosine);
        throw std::runtime_error("Native selected-light publication differs from original");
      }
    }
  }
  // Compatibility writes consume the computed publication, never reread a
  // shader-register payload to manufacture native light ownership.
  for (size_t n = 0; n < publication->count; ++n) {
    const auto &write = publication->writes[n];
    bd::mem::store<uint32_t>(write.address, write.word);
  }
  current = publication->state;
  current_selection = selection;
  current_frame = FrameStatFrameCount();
  if (current.known == 7 && PublishNativeMaterialLights(selection, current.lights)) ++stats.snapshots;
  else InvalidateNativeMaterialLights();
  ++stats.publications; stats.changes += publication->changed;
  return true;
}
} // namespace

std::optional<NativeSelectedLights> FindNativeSelectedLights(uint32_t selection) {
  if (!REXCVAR_GET(bd_native_lighting) || current.known != 7 || current_selection != selection ||
      current_frame != FrameStatFrameCount()) { ++stats.unavailable; return {}; }
  ++stats.snapshots;
  return current.lights;
}
void NativeSelectedLightsReport() { Report(); }
void CheckNativeSelectedLights(const NativeSelectedLights &lights, const uint8_t *pixel_constants) {
  ++stats.draw_checks;
  bool same = true;
  for (size_t slot = 0; slot < lights.size(); ++slot) {
    std::array<float, 16> actual;
    std::memcpy(actual.data(), pixel_constants+(20+slot*4)*16, sizeof(actual));
    const auto &light = lights[slot];
    const int kind = actual[3] < .5f ? LitDisabled : actual[3] < 1.5f ? LitDirectional :
                     actual[3] < 2.5f ? LitSpot : LitPoint;
    same &= std::isfinite(actual[3]) && kind == light.kind;
    const auto xyz = [&](size_t at, LitVector expected) {
      return actual[at] == expected.x && actual[at+1] == expected.y && actual[at+2] == expected.z;
    };
    if (light.kind == LitDisabled) continue;
    same &= xyz(8, light.colour);
    if (light.kind == LitDirectional || light.kind == LitSpot) same &= xyz(4, light.direction);
    if (light.kind == LitPoint || light.kind == LitSpot)
      same &= xyz(0, light.position) && actual[11] == light.inverse_range;
    if (light.kind == LitSpot)
      same &= actual[7] == light.cone_strength && std::isfinite(actual[12]) &&
              std::abs(actual[12]-light.cone_cosine) <= 2e-6f;
  }
  if (!same) {
    ++stats.draw_wrong;
    BD_ERROR("[native-selected-light-mismatch] object snapshot differs at normal-lit draw");
    throw std::runtime_error("Native selected lights differ at draw consumption");
  }
}
void PublishNativeSelectedLights(PPCContext &ctx, uint8_t *base) {
  if (!REXCVAR_GET(bd_native_lighting) || !Publish(ctx, base)) {
    current = {}; current_selection = 0; current_frame = ~0u;
    InvalidateNativeMaterialLights();
    ++stats.compatibility;
    __imp__sub_8218B0F0(ctx, base);
  }
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(sub_8218B0F0) {
  bd::gpu::scene::PublishNativeSelectedLights(ctx, base);
}
