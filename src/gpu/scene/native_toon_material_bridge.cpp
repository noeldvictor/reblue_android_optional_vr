/**
 * @brief Whole native Toon update/begin/end and direct texture publication.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_toon_material_bridge.h"
#include "gpu/scene/native_toon_material.h"
#include "gpu/scene/scene_texture_import.h"
#include "gpu/scene/host_draw.h"
#include "gpu/constant_buffers.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "gpu/native_texture_mirror.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_821837B0);
REX_EXTERN(__imp__Visual__Shader__Toon__vf04);
REX_EXTERN(__imp__sub_82183990);
REXCVAR_DEFINE_BOOL(bd_native_toon_material, true, kCvarGroup,
    "Native Toon texture animation and edge-parameter callbacks.");
REXCVAR_DEFINE_BOOL(bd_native_toon_material_verify, false, kCvarGroup,
    "Compare native Toon edge words with original leaf publication; diagnostic only.");

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kUpdate = 0x821837B0, kBegin = 0x82183910, kEnd = 0x82183990;
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
constexpr uint32_t kEngine = (uint32_t(-32034) << 16) - 19936;
struct Stats {
  uint64_t updates = 0, begins = 0, ends = 0, bindings = 0, nulls = 0;
  uint64_t checked = 0, wrong = 0, compatibility = 0, refused = 0, faults = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-toon-material] updates {} begins {} ends {} texture bindings {} nulls {}; "
          "checked {} wrong {} compatibility {} refused {} faults {}; "
          "authored counters/list/edge sources, two inherited edge words, resource/shader ABI remain",
      stats.updates, stats.begins, stats.ends, stats.bindings, stats.nulls,
      stats.checked, stats.wrong, stats.compatibility, stats.refused, stats.faults);
  stats.frame = frame; stats.reported = true;
}
void Check(bool valid) {
  if (valid) return;
  ++stats.faults; stats.reported = false; Report();
  throw std::runtime_error("Native Toon material lost a validated input");
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address > UINT32_MAX || bytes > UINT32_MAX ||
      address + bytes - 1 > UINT32_MAX || !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) { return !(address & 3) && Range(address, bytes); }
uint32_t Read(uint64_t address) { Check(Words(address, 4)); return bd::mem::load<uint32_t>(uint32_t(address)); }
void Write(uint64_t address, uint32_t value) { Check(Words(address, 4)); bd::mem::store<uint32_t>(uint32_t(address), value); }
bool Overlap(uint64_t a, uint64_t a_bytes, uint64_t b, uint64_t b_bytes) {
  return a < b + b_bytes && b < a + a_bytes;
}
struct AnimationPort {
  uint32_t material;
  uint32_t Counter(uint32_t index) { return Read(uint64_t(material) + 16 + index * 4); }
  void SetCounter(uint32_t index, uint32_t value) { Write(uint64_t(material) + 16 + index * 4, value); }
  uint32_t TextureList() { return Read(uint64_t(material) + 12); }
  uint32_t ActiveList() { return Read(kActiveTextureTable + 4); }
  uint32_t ActiveOffset() { return Read(kActiveTextureTable); }
  uint32_t ImageCount(uint32_t list) { return Read(list); }
  uint32_t Image(uint32_t list, uint32_t index) {
    const auto entries = Read(uint64_t(list) + 4);
    Check(entries != 0);
    return Read(uint64_t(entries) + uint64_t(index) * 28 + 24);
  }
  uint32_t FallbackImage() { return Read(kActiveTextureTable + 32); }
  void BindTexture(uint32_t slot, uint32_t address) {
    auto *texture = ResolveGuestTexture(address);
    if (!texture && address) texture = GetOrCreateDebugTexture();
    Video::SetTexture(slot, texture); // same null/inherited resource semantics
    ++stats.bindings; stats.nulls += address == 0;
  }
};
bool Ready(uint32_t callback, PPCContext &ctx) {
  if (callback == kEnd) return Words(kEngine + 40, 4);
  if (ctx.r1.u32 < 128 || (ctx.r1.u32 & 15) || !Words(ctx.r3.u32, 104) ||
      !Words(kDevice, 4) || !Range(uint64_t(ctx.r1.u32) - 128, 128)) return false;
  const uint64_t material = ctx.r3.u32, scratch = uint64_t(ctx.r1.u32) - 128;
  if (Overlap(material, 104, scratch, 128)) return false;
  if (callback == kUpdate) return Words(kActiveTextureTable, 36);
  const auto device = Read(kDevice);
  // Keep the old leaf's rare cross-alias case at its explicit boundary.
  return !(device & 15) && Words(device, 2624) &&
      !Overlap(material, 104, device, 2624) && !Overlap(scratch, 128, device, 2624) &&
      !Overlap(kDevice, 4, device, 2624);
}
void Begin(PPCContext &ctx, uint8_t *base) {
  ctx.fpscr.disableFlushMode();
  const auto device = Read(kDevice);
  std::array<uint32_t, 6> authored;
  for (uint32_t i = 0; i < 6; ++i) authored[i] = Read(uint64_t(ctx.r3.u32) + 48 + i * 4);
  // Fur VS edgeRW.z is consumed. Do not guess a zero until the native fur
  // schema supplies that semantic; the remaining inherited inputs are explicit.
  const auto words = BuildToonEdgeParameters(authored,
      {Read(uint64_t(ctx.r1.u32) - 8), Read(uint64_t(ctx.r1.u32) - 4)});
  const uint64_t dirty = ((uint64_t(Read(device)) << 32) | Read(uint64_t(device) + 4)) | (uint64_t(1) << 51);
  if (REXCVAR_GET(bd_native_toon_material_verify)) {
    __imp__Visual__Shader__Toon__vf04(ctx, base);
    ++stats.checked;
    bool same = Read(device) == uint32_t(dirty >> 32) && Read(uint64_t(device) + 4) == uint32_t(dirty);
    for (uint32_t i = 0; i < 8; ++i) same &= Read(uint64_t(device) + 2592 + i * 4) == words[i];
    if (!same) { ++stats.wrong; Check(false); }
  }
  // Compatibility/getter/reference mirror, not the source of native storage.
  for (uint32_t i = 0; i < 8; ++i) Write(uint64_t(device) + 2592 + i * 4, words[i]);
  Write(device, uint32_t(dirty >> 32)); Write(uint64_t(device) + 4, uint32_t(dirty));
  PublishNativeShaderParameters(device, true, 50, 2, words.data());
  Video::MarkVSConstantsDirty();
  ++stats.begins;
}
} // namespace

bool TryNativeToonMaterial(uint32_t callback, PPCContext &ctx, uint8_t *base) {
  if ((callback != kUpdate && callback != kBegin && callback != kEnd) ||
      !REXCVAR_GET(bd_native_toon_material) || !Ready(callback, ctx)) return false;
  if (callback == kUpdate) {
    AnimationPort port{ctx.r3.u32}; UpdateToonMaterial(port);
    ctx.r3.u64 = Read(kDevice); ++stats.updates;
  } else if (callback == kBegin) Begin(ctx, base);
  else { Write(kEngine + 40, 0); ++stats.ends; }
  Report(); return true;
}
void ToonFallback(PPCContext &ctx, uint8_t *base, void (*original)(PPCContext &, uint8_t *), bool begin) {
  ++stats.compatibility; stats.refused += REXCVAR_GET(bd_native_toon_material);
  original(ctx, base);
  if (begin) { InvalidateNativeShaderParameters(true, 50, 2); Video::MarkVSConstantsDirty(); }
  Report();
}
} // namespace bd::gpu::scene

#define NATIVE_TOON_HOOK(name, address, begin) \
  REX_HOOK_RAW(name) { \
    if (!bd::gpu::scene::TryNativeToonMaterial(address, ctx, base)) \
      bd::gpu::scene::ToonFallback(ctx, base, __imp__##name, begin); \
  }
NATIVE_TOON_HOOK(sub_821837B0, 0x821837B0, false)
NATIVE_TOON_HOOK(Visual__Shader__Toon__vf04, 0x82183910, true)
NATIVE_TOON_HOOK(sub_82183990, 0x82183990, false)
#undef NATIVE_TOON_HOOK
