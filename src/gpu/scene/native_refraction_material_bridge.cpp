/**
 * @brief Whole water/refraction setup replacements with counted import adapters.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_refraction_material.h"
#include "gpu/scene/refraction_material_import.h"
#include "gpu/scene/shader_parameter_import.h"
#include "gpu/native_texture_mirror.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_82454720);
REX_EXTERN(__imp__sub_82455150);
REX_EXTERN(bdShaderConstantFlush);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(sub_8221D2C8);
REXCVAR_DECLARE(bool, bd_native_scene_textures);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kPhase = (uint32_t(-32137) << 16) + 16476;
constexpr uint32_t kSettings = (uint32_t(-32035) << 16) - 26552;
constexpr uint32_t kDefaultFactor = (uint32_t(-32251) << 16) + 21040;
constexpr uint32_t kPlanarImage = (uint32_t(-32035) << 16) + 29040 + 12;
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
struct Stats {
  uint64_t water = 0, refraction = 0, compatibility = 0, refused = 0, faults = 0;
  uint64_t parameters = 0, state_adapters = 0, bindings = 0, null_bindings = 0, snapshots = 0, clamped = 0;
  uint64_t debug_bindings = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-refraction-material] water {} refraction {} compatibility {} refused {} faults {}; "
          "parameter adapters {} state adapters {} image bindings {} null no-ops {} snapshots {} clamps {} debug images {}; "
          "host setup/order, authored fields/descriptors, state shadows, getters and snapshot scope adapters remain",
      stats.water, stats.refraction, stats.compatibility, stats.refused, stats.faults,
      stats.parameters, stats.state_adapters, stats.bindings, stats.null_bindings, stats.snapshots, stats.clamped,
      stats.debug_bindings);
  stats.frame = frame;
  stats.reported = true;
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address > UINT32_MAX || bytes > UINT32_MAX ||
      address + bytes - 1 > UINT32_MAX || !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) { return !(address & 3) && Range(address, bytes); }
void Check(bool valid) {
  if (!valid) { ++stats.faults; throw std::runtime_error("Native refraction material lost a validated import"); }
}
std::optional<uint32_t> Word(uint64_t address) {
  return Words(address, 4) ? std::optional(bd::mem::load<uint32_t>(uint32_t(address))) : std::nullopt;
}
uint32_t ReadWord(uint64_t address) { const auto value = Word(address); Check(bool(value)); return *value; }
float ReadFloat(uint64_t address) { Check(Words(address, 4)); return bd::mem::load<float>(uint32_t(address)); }
bool Overlap(uint64_t a, uint64_t bytes, uint64_t scratch) { return a < scratch + 1120 && scratch < a + bytes; }
bool DescriptorReady(uint32_t descriptor, uint64_t scratch) {
  if (!Words(descriptor, 16) || Overlap(descriptor, 16, scratch)) return false;
  const auto flags = ReadWord(descriptor) & 3;
  if (!flags) return true;
  const auto first = ReadWord(uint64_t(descriptor) + 4);
  const auto count = ReadWord(uint64_t(descriptor) + 8) - first;
  const auto source = ReadWord(uint64_t(descriptor) + 12);
  return ParameterRangeSupported(first, count) &&
      (!count || (Range(source, uint64_t(count) * 16) && !Overlap(source, uint64_t(count) * 16, scratch)));
}
bool Ready(PPCContext &ctx, bool water) {
  const uint32_t material = ctx.r3.u32;
  const uint64_t scratch = uint64_t(ctx.r1.u32) - 1024;
  if (ctx.r1.u32 < 1024 || (ctx.r1.u32 & 15) || !Words(scratch, 1120) ||
      !Words(material, water ? 5060 : 4972) || Overlap(material, water ? 5060 : 4972, scratch)) return false;
  if (!water) return DescriptorReady(ReadWord(uint64_t(material) + 4968), scratch);
  if (!Word(kPhase) || !Word(kDefaultFactor) || !Word(kPlanarImage) || !Word(kDevice)) return false;
  if (ReadWord(kPhase) == 3) {
    const auto settings = Word(kSettings);
    if (!settings || !*settings || !Word(uint64_t(*settings) + 7020)) return false;
  }
  const auto read = [scratch](uint64_t address) {
    return Overlap(address, 4, scratch) ? std::nullopt : Word(address);
  };
  const auto destination = ReadWaterFactorDestination(material, read);
  return destination && Words(*destination, 4) && !Overlap(*destination, 4, scratch) &&
      DescriptorReady(ReadWord(uint64_t(material) + 4760), scratch) &&
      DescriptorReady(ReadWord(uint64_t(material) + 4952), scratch) && bool(ReadWaterSceneImage(read));
}
struct Adapter {
  PPCContext &ctx;
  uint8_t *base;
  const uint32_t material;
  const uint64_t saved_stack;
  const bool water;
  Adapter(PPCContext &context, uint8_t *memory, bool is_water)
      : ctx(context), base(memory), material(ctx.r3.u32), saved_stack(ctx.r1.u64), water(is_water) {
    ctx.r1.u32 -= 256;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
    ctx.fpscr.disableFlushMode();
  }
  ~Adapter() { ctx.r1.u64 = saved_stack; }
  void PublishSceneFactor() {
    const bool scene = ReadWord(kPhase) == 3;
    const bool force = scene && ReadWord(uint64_t(ReadWord(kSettings)) + 7020) != 0;
    const int32_t authored = int32_t(ReadWord(uint64_t(material) + 4708));
    const auto destination = ReadWaterFactorDestination(material, Word);
    Check(destination && Words(*destination, 4));
    bd::mem::store<float>(*destination, WaterSceneFactor(scene, force, authored, ReadFloat(kDefaultFactor)));
  }
  void Flush(uint32_t offset, bool clamp) {
    const uint32_t descriptor = ReadWord(uint64_t(material) + offset);
    Check(DescriptorReady(descriptor, uint64_t(uint32_t(saved_stack)) - 1024));
    if (clamp && (ReadWord(descriptor) & 2)) {
      const uint32_t first = ReadWord(uint64_t(descriptor) + 4), end = ReadWord(uint64_t(descriptor) + 8);
      const uint32_t data = ReadWord(uint64_t(descriptor) + 12);
      if (first <= 51 && end > 51 && data) {
        const uint64_t address = uint64_t(data) + (51 - first) * 16 + 12;
        const float before = ReadFloat(address), after = ClampWaterHighlight(before);
        if (before > 1.f) { bd::mem::store<float>(uint32_t(address), after); ++stats.clamped; }
      }
    }
    ctx.r3.u64 = descriptor;
    ++stats.parameters;
    bdShaderConstantFlush(ctx, base); // existing host parameter producer / counted descriptor adapter
  }
  void FlushWaterParameters(uint32_t index) { Flush(index ? 4952 : 4760, true); }
  void FlushRefractionParameters() { Flush(4968, false); }
  void State(uint32_t offset, uint32_t value) {
    ctx.r3.u64 = offset; ctx.r4.u64 = value;
    ++stats.state_adapters;
    bdSetRenderState(ctx, base); // existing native intent producer, temporary cache/getter shadow
  }
  void EnableSourceAlphaBlending() { State(60, 1); State(72, 6); State(76, 7); }
  void EnableDepthTest() { State(40, 1); } // no depth-write override, separate alpha or blend-op reset
  void Bind(uint32_t slot, uint32_t address) {
    if (address) {
      auto *image = ResolveGuestTexture(address); // can wait for IO, never under the video mutex
      if (!image) {
        image = GetOrCreateDebugTexture(); // preserve the existing unsupported-image marker, never replay setup
        ++stats.debug_bindings;
      }
      Check(image != nullptr);
      Video::SetTexture(slot, image);
      ++stats.bindings;
    } else ++stats.null_bindings;
    ctx.r3.u64 = ReadWord(kDevice); // temporary void-callback register convention
  }
  void BindPlanarReflection() { Bind(7, ReadWord(kPlanarImage)); }
  void BindSceneImage() {
    const auto image = ReadWaterSceneImage(Word);
    Check(bool(image)); Bind(12, *image);
  }
  bool WantsSnapshot() { return int32_t(ReadWord(uint64_t(material) + 4700)) > 0; }
  void Snapshot() {
    ctx.r3.u64 = uint64_t(material) + (water ? 4648 : 4932);
    ctx.r4.u64 = material;
    ++stats.snapshots;
    sub_8221D2C8(ctx, base); // native snapshot producer; unowned scopes remain tracked there
  }
};
void Prepare(PPCContext &ctx, uint8_t *base, bool water) {
  const bool enabled = REXCVAR_GET(bd_native_scene_textures);
  if (!enabled || !Ready(ctx, water)) {
    ++stats.compatibility; stats.refused += enabled;
    if (water) __imp__sub_82454720(ctx, base); else __imp__sub_82455150(ctx, base);
    Report(); return;
  }
  Adapter adapter(ctx, base, water);
  if (water) { PrepareWaterMaterial(adapter); ++stats.water; }
  else { PrepareRefractionMaterial(adapter); ++stats.refraction; }
  // No fallback/replay after the first material or GPU side effect.
  Report();
}
} // namespace
} // namespace bd::gpu::scene
REX_HOOK_RAW(sub_82454720) { bd::gpu::scene::Prepare(ctx, base, true); }
REX_HOOK_RAW(sub_82455150) { bd::gpu::scene::Prepare(ctx, base, false); }
