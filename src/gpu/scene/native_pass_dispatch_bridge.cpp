/**
 * @file    native_pass_dispatch_bridge.cpp
 * @brief   Complete host replacement of pass-start/pass-finish scheduling.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_pass_dispatch.h"
#include "gpu/scene/pass_dispatch_import.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "gpu/frame_stats.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/function_dispatcher.h>
#include <stdexcept>

REX_EXTERN(__imp__bdShaderSystemBeginFrame);
REX_EXTERN(__imp__bdShaderSystemFlush);
REXCVAR_DECLARE(bool, bd_native_passes);

namespace bd::gpu::scene {
namespace {
// bdShaderSystemBeginFrame (0x821869F0) and bdShaderSystemFlush (0x82186B10).
// bdRenderViewSubmit also inlines starts; its finishes use this same dispatcher.
// These addresses and byte flags are imports, not native scene identities.
constexpr uint32_t kParticipants = (uint32_t(-32030) << 16) - 31132;
constexpr uint32_t kPhase = (uint32_t(-32137) << 16) + 16476;
constexpr uint32_t kLightSpace = (uint32_t(-32035) << 16) - 26711;
struct Stats {
  uint64_t begins = 0, ends = 0, begin_callbacks = 0, end_callbacks = 0;
  uint64_t accepted = 0, cleared = 0, compatibility_begin = 0, compatibility_end = 0;
  uint64_t refused = 0, faults = 0;
  uint32_t frame = 0;
};
thread_local Stats stats;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (frame - stats.frame < 300)
    return;
  BD_INFO("[native-pass-dispatch] begins {} ends {}; participant callbacks begin {} end {}; "
          "accepted {} cleared {}; compatibility begin {} end {} refused {} faults {}; "
          "parent branch decisions, descriptor callbacks and participant registry/flags remain imports",
          stats.begins, stats.ends, stats.begin_callbacks, stats.end_callbacks,
          stats.accepted, stats.cleared, stats.compatibility_begin, stats.compatibility_end,
          stats.refused, stats.faults);
  stats.frame = frame;
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address + bytes - 1 > UINT32_MAX ||
      !bd::mem::try_at<uint8_t>(uint32_t(address)))
    return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096;
       page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page)))
      return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) {
  return !(address & 3) && Range(address, bytes);
}
void Check(bool value) {
  if (!value) {
    ++stats.faults;
    // Never replay an original begin/finish after any callback has executed.
    throw std::runtime_error("Pass participant import changed to an invalid address/callback");
  }
}
uint32_t VirtualAddress(uint32_t object, uint32_t slot) {
  if (!Words(object, 4))
    return 0;
  const auto table = bd::mem::load<uint32_t>(object);
  return table && Words(uint64_t(table) + slot, 4)
      ? bd::mem::load<uint32_t>(table + slot) : 0;
}
bool Prepare(PPCContext &ctx, bool begin) {
  if (!Words(kParticipants, 36) || !Words(kPhase, 4) || !Range(kLightSpace, 1) ||
      ctx.r1.u32 < 512 || (ctx.r1.u32 & 15) ||
      !Words(uint64_t(ctx.r1.u32) - 512, 608))
    return false;
  const auto address = VirtualAddress(ctx.r3.u32, begin ? 12 : 20);
  return address && REX_KERNEL_STATE()->function_dispatcher()->GetFunction(address);
}
struct EnginePassAdapter {
  PPCContext &ctx;
  uint8_t *base;
  const uint32_t descriptor, phase;
  const uint64_t saved_stack;
  explicit EnginePassAdapter(PPCContext &context, uint8_t *memory)
      : ctx(context), base(memory), descriptor(ctx.r3.u32), phase(ctx.r4.u32),
        saved_stack(ctx.r1.u64) {
    // Preserve the caller's live frame. Only remaining callbacks need PPC ABI
    // storage; all scheduling and iteration state lives in host locals.
    ctx.r1.u32 -= 128;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
  }
  ~EnginePassAdapter() { ctx.r1.u64 = saved_stack; }
  void Call(uint32_t object, uint32_t slot) {
    const auto address = VirtualAddress(object, slot);
    auto *fn = address ? REX_KERNEL_STATE()->function_dispatcher()->GetFunction(address) : nullptr;
    Check(fn != nullptr);
    ctx.r3.u64 = object;
    ctx.last_indirect_target = address;
    fn(ctx, base);
  }
  uint32_t Participant(uint32_t index) {
    const auto table = bd::mem::load<uint32_t>(kParticipants + 24);
    const uint64_t slot = uint64_t(table) + uint64_t(index) * 4;
    Check(table && Words(slot, 4));
    const auto object = bd::mem::load<uint32_t>(uint32_t(slot));
    Check(Words(object, 7));
    return object;
  }
  int32_t ParticipantCount() { return bd::mem::load<int32_t>(kParticipants + 32); }
  void PublishMode() {
    bd::mem::store<uint32_t>(kPhase, phase);
    bd::mem::store<uint8_t>(kLightSpace, uint8_t(ImportPassLightSpace(phase)));
  }
  void BeginPass() { Call(descriptor, 12); }
  void EndPass() { Call(descriptor, 20); }
  bool BeginParticipant(uint32_t index) {
    ctx.r4.u64 = phase;
    Call(Participant(index), 16);
    ++stats.begin_callbacks;
    return ImportParticipantAccepted(ctx.r3.u32);
  }
  bool ParticipantActive(uint32_t index) {
    return ImportParticipantActive(bd::mem::load<uint8_t>(Participant(index) + 6));
  }
  void EndParticipant(uint32_t index) {
    Call(Participant(index), 20);
    ++stats.end_callbacks;
  }
  void SetParticipantActive(uint32_t index, bool active) {
    bd::mem::store<uint8_t>(Participant(index) + 6, uint8_t(active));
    if (active) ++stats.accepted;
    else ++stats.cleared;
  }
};
} // namespace
} // namespace bd::gpu::scene

REX_HOOK_RAW(bdShaderSystemBeginFrame) {
  using namespace bd::gpu::scene;
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !Prepare(ctx, true)) {
    ++stats.compatibility_begin;
    stats.refused += enabled;
    __imp__bdShaderSystemBeginFrame(ctx, base);
  } else {
    EnginePassAdapter adapter(ctx, base);
    DispatchPassBegin(adapter);
    ++stats.begins;
  }
  Report();
}
REX_HOOK_RAW(bdShaderSystemFlush) {
  using namespace bd::gpu::scene;
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !Prepare(ctx, false)) {
    ++stats.compatibility_end;
    stats.refused += enabled;
    __imp__bdShaderSystemFlush(ctx, base);
  } else {
    EnginePassAdapter adapter(ctx, base);
    DispatchPassEnd(adapter);
    ++stats.ends;
  }
  Report();
}
