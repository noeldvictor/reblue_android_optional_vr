/**
 * @brief Whole host water animation/property and sampling-demand updates.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/water_material_import.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include "engine/frame_clock.h"
#include "gpu/frame_stats.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_82454398);
REX_EXTERN(__imp__sub_8221D460);
REXCVAR_DECLARE(bool, bd_native_scene_textures);
REXCVAR_DEFINE_BOOL(bd_native_water_verify, false, kCvarGroup,
    "Compare bounded native water/demand publications with original execution; diagnostic only.");

namespace bd::gpu::scene {
namespace {
thread_local bool reference_execution = false;
struct ReferenceScope {
  ReferenceScope() { reference_execution = true; }
  ~ReferenceScope() { reference_execution = false; }
};
struct Stats {
  uint64_t updates = 0, demand = 0, ticks = 0, parameters = 0, mode_words = 0;
  uint64_t compatibility = 0, refused = 0, checked = 0, wrong = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-water-update] updates {} demand callbacks {} tick advances {} parameter words {} mode words {}; "
          "compatibility {} refused {} checked {} wrong {}; authored fields, parameter storage, demand counters and tick source remain imports",
      stats.updates, stats.demand, stats.ticks, stats.parameters, stats.mode_words,
      stats.compatibility, stats.refused, stats.checked, stats.wrong);
  stats.frame = frame; stats.reported = true;
}
std::optional<uint32_t> ReadWord(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX - 3) return {};
  const auto *word = bd::mem::try_at<const be_u32>(uint32_t(address));
  return word ? std::optional(uint32_t(*word)) : std::nullopt;
}
void Update(PPCContext &ctx, uint8_t *base, bool water) {
  const auto original = water ? __imp__sub_82454398 : __imp__sub_8221D460;
  if (reference_execution) { original(ctx, base); return; }
  const bool enabled = REXCVAR_GET(bd_native_scene_textures);
  const uint64_t input = ctx.r3.u64;
  const bool tick = water && bd::engine::TickDue();
  std::optional<WaterUpdatePlan> plan;
  if (enabled) {
    if (water) {
      // Original spills at SP-96..SP-1. Refuse aliases with those writes so
      // omission of the old stack frame cannot change material/control inputs.
      if (ctx.r1.u32 >= 96 && !(ctx.r1.u32 & 15)) {
        const uint64_t scratch = uint64_t(ctx.r1.u32) - 96;
        if (ReadWord(scratch) && ReadWord(scratch + 92)) {
          ctx.fpscr.disableFlushMode();
          plan = BuildWaterMaterialUpdate(ctx.r3.u32, tick, [scratch](uint64_t address) {
            return address < scratch + 96 && scratch < address + 4 ? std::nullopt : ReadWord(address);
          });
        }
      }
    } else plan = BuildSamplingDemandUpdate(ctx.r3.u32, ctx.r4.u32, ReadWord);
  }
  if (!plan) {
    ++stats.compatibility; stats.refused += enabled;
    original(ctx, base); Report(); return;
  }
  const uint64_t result = input + (water ? 4656 : 0);
  if (REXCVAR_GET(bd_native_water_verify)) {
    ReferenceScope reference;
    original(ctx, base);
    ++stats.checked;
    if (ctx.r3.u64 != result || !plan->Matches(ReadWord)) {
      ++stats.wrong;
      Report();
      throw std::runtime_error("Native water publication differs from original");
    }
  } else {
    plan->Apply([](uint32_t address, uint32_t word) { bd::mem::store<uint32_t>(address, word); });
    ctx.r3.u64 = result;
  }
  if (water) { ++stats.updates; stats.ticks += tick; }
  else ++stats.demand;
  stats.parameters += plan->parameters; stats.mode_words += plan->mode;
  Report();
}
} // namespace
} // namespace bd::gpu::scene
REX_HOOK_RAW(sub_82454398) { bd::gpu::scene::Update(ctx, base, true); }
REX_HOOK_RAW(sub_8221D460) { bd::gpu::scene::Update(ctx, base, false); }
