/**
 * @brief Whole sorted visual scheduler; explicit remaining authored adapters.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/hooks/native_visual_schedule.h"
#include "gpu/hooks/native_ui.h"
#include "gpu/scene/native_visual_schedule.h"
#include "gpu/scene/visual_schedule_import.h"
#include "gpu/scene/immediate_ui_import.h"
#include "gpu/constant_buffers.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/xthread.h>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <stdexcept>

#pragma clang fp contract(off)

REX_EXTERN(__imp__Visual__DrawSortedQueues);
REX_EXTERN(bdBeginRenderPass);
REX_EXTERN(bdEndRenderPass);
REX_EXTERN(bdVisualObjectInitBones);
REX_EXTERN(bdSetRenderState);
REX_EXTERN(bdSetSamplerState);
REX_EXTERN(bdSetVertexDeclarationCached);
REX_EXTERN(bdInitDefaultTextures);
REX_EXTERN(D3DDevice_SetTexture);
REX_EXTERN(D3DDevice_SetVertexShaderConstantFN);
REX_EXTERN(D3DDevice_SetPixelShaderConstantFN);
REX_EXTERN(D3DDevice_SetVertexShaderConstantB);
REX_EXTERN(D3DDevice_SetPixelShaderConstantB);
REX_EXTERN(sub_821793D0);
REXCVAR_DEFINE_BOOL(bd_native_visual_schedule, true, kCvarGroup,
    "Host-owned sorted model/primitive ordering and complete scheduler.");

namespace bd::gpu::hooks {
namespace {
using namespace scene;
constexpr uint32_t kQueues = (uint32_t(-32035) << 16) - 26748;
constexpr uint32_t kContext = (uint32_t(-32035) << 16) - 26552;
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
constexpr uint32_t kOne = (uint32_t(-32251) << 16) + 20908;
constexpr uint32_t kScale = (uint32_t(-32247) << 16) - 3264;
constexpr uint32_t kModelScope = (uint32_t(-32036) << 16) - 5536;
constexpr uint32_t kEngine = (uint32_t(-32034) << 16) - 19936;
constexpr uint32_t kMaterial = (uint32_t(-32034) << 16) - 32552;
constexpr uint32_t kChoices = (uint32_t(-32137) << 16) + 29804;
constexpr uint32_t kResetColour = (uint32_t(-32033) << 16) + 16340;
constexpr uint32_t kLighting = (uint32_t(-32133) << 16) - 31620;
constexpr uint32_t kThread = (uint32_t(-32035) << 16) - 26664;
struct Stats {
  uint64_t native = 0, empty = 0, models = 0, primitives = 0, deferred = 0, overflow = 0;
  uint64_t compatibility = 0, refused = 0, faults = 0, callbacks = 0, pass_adapters = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
thread_local bool active = false;
// One 32 KiB order, reused between the two non-overlapping dispatch phases.
// No guest bucket heads, next pointers, persistent copies, or per-view allocation.
thread_local std::unique_ptr<NativeVisualOrder> order;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-visual-schedule] native {} empty {} models {} primitives {} deferred {} overflow {}; "
          "compatibility {} refused {} faults {}; model/material callbacks {} pass adapters {}; "
          "host ordering/dispatch, authored queues/visuals and state/resource/deferred-consumer adapters remain",
      stats.native, stats.empty, stats.models, stats.primitives, stats.deferred, stats.overflow,
      stats.compatibility, stats.refused, stats.faults, stats.callbacks, stats.pass_adapters);
  stats.frame = frame; stats.reported = true;
}
void Check(bool valid) {
  if (valid) return;
  ++stats.faults; stats.reported = false; Report();
  throw std::runtime_error("Native visual schedule lost a validated authored input");
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address > UINT32_MAX || bytes > UINT32_MAX ||
      address + bytes - 1 > UINT32_MAX || !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
std::optional<uint32_t> Word(uint64_t address) {
  if ((address & 3) || !Range(address, 4)) return {};
  return bd::mem::load<uint32_t>(uint32_t(address));
}
uint32_t Read(uint64_t address) { const auto value = Word(address); Check(value.has_value()); return *value; }
void Write(uint64_t address, uint32_t value) {
  Check(Word(address).has_value()); bd::mem::store<uint32_t>(uint32_t(address), value);
}
float Scalar(uint64_t address) { return std::bit_cast<float>(Read(address)); }

struct Adapter {
  PPCContext &ctx;
  uint8_t *base;
  uint32_t model_base = 0, primitive_base = 0;
  const uint32_t one;
  const float zero, scale;
  const uint64_t saved_stack;
  bool model_scope = false;
  Adapter(PPCContext &context, uint8_t *memory)
      : ctx(context), base(memory), one(Read(kOne)), zero(Scalar(kOne + 132)),
        scale(Scalar(kScale)), saved_stack(ctx.r1.u64) {
    active = true;
    // Calls inherit this context and its normal argument-home area; never use
    // typed imports that re-root r1 above live caller frames.
    ctx.r1.u32 -= 208;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
    ctx.fpscr.disableFlushMode();
  }
  ~Adapter() {
    if (model_scope) {
      bd::mem::store<uint8_t>(kModelScope + 1, 0);
      bd::mem::store<uint32_t>(kModelScope + 520, 0);
    }
    ctx.r1.u64 = saved_stack; active = false;
    ctx.fpscr.disableFlushMode();
    Video::MarkVSConstantsDirty(); Video::MarkPSConstantsDirty();
  }
  uint32_t Call(PPCFunc *fn, std::initializer_list<uint64_t> arguments = {}) {
    const std::array<PPCRegister *, 8> registers{
        &ctx.r3, &ctx.r4, &ctx.r5, &ctx.r6, &ctx.r7, &ctx.r8, &ctx.r9, &ctx.r10};
    Check(arguments.size() <= registers.size());
    size_t index = 0;
    for (auto value : arguments) registers[index++]->u64 = value;
    fn(ctx, base); ctx.fpscr.disableFlushMode();
    return ctx.r3.u32;
  }
  void Callback(uint32_t function, uint32_t visual) {
    auto *fn = REX_KERNEL_STATE()->function_dispatcher()->GetFunction(function);
    Check(fn != nullptr); ctx.last_indirect_target = function;
    Call(fn, {visual}); ++stats.callbacks;
  }
  bool Sort(NativeVisualOrder &target, uint32_t count, bool model) {
    const uint32_t start = Read(kQueues + (model ? 4 : 16));
    const uint32_t stride = model ? 104 : 52, flags = model ? 92 : 36;
    if (count && !Range(start, uint64_t(count) * stride)) return false;
    if (model) model_base = start; else primitive_base = start;
    const uint32_t limits = kQueues + (model ? 8 : 20);
    return target.Build(count, {Scalar(limits + 4), Scalar(limits), zero,
        std::bit_cast<float>(one), scale}, model, [&](uint32_t index) -> std::optional<VisualSortInput> {
      const auto depth = Word(uint64_t(start) + uint64_t(index) * stride);
      const auto bits = Word(uint64_t(start) + uint64_t(index) * stride + flags);
      if (!depth || !bits) return {};
      return VisualSortInput{std::bit_cast<float>(*depth), *bits};
    });
  }
  void State(uint32_t field, uint32_t value) { Call(bdSetRenderState, {field, value}); }
  uint32_t Blend(uint32_t flags) {
    const auto blend = ImportVisualBlend(flags);
    State(72, blend.source); State(76, blend.destination);
    return blend.mode;
  }
  void Depth(uint32_t flags_address) {
    State(48, !(Read(flags_address) & 8));
    if (Read(kQueues + 1064)) State(40, !(Read(flags_address) & 128));
  }
  void ModelScope(bool enable) {
    bd::mem::store<uint8_t>(kModelScope + 1, uint8_t(enable));
    Write(kModelScope + 520, uint32_t(enable)); model_scope = enable;
  }
  void Model(uint32_t index) {
    const uint64_t entry = uint64_t(model_base) + uint64_t(index) * 104;
    const uint32_t visual = Read(entry + 8);
    if (!visual) return;
    struct ModelPort {
      Adapter &owner;
      uint64_t entry, visual;
      uint32_t Entry(uint32_t offset) { return Read(entry + offset); }
      void Store(uint32_t offset, uint32_t value) { Write(visual + offset, value); }
      uint32_t Table() { return Read(visual); }
      uint32_t Method(uint32_t table, uint32_t slot) { return Read(uint64_t(table) + slot); }
      void Callback(uint32_t function) { owner.Callback(function, uint32_t(visual)); }
      void CopyMatrix() {
        // All four vectors load before any store in the original matrix copy.
        const uint64_t destination = visual +
            (rex::system::XThread::GetCurrentThreadId() == Read(kThread) ? 2388 : 2452);
        Check(Range(entry + 12, 64) && Range(destination, 64));
        std::memmove(bd::mem::at<uint8_t>(uint32_t(destination)), bd::mem::at<uint8_t>(uint32_t(entry + 12)), 64);
      }
      void InitBones() { owner.Call(bdVisualObjectInitBones, {visual}); ++stats.callbacks; }
      void State(uint32_t field, uint32_t value) { owner.State(field, value); }
      bool DepthPolicy() { return Read(kQueues + 1064) != 0; }
      uint32_t Blend(uint32_t flags) { return owner.Blend(flags); }
    } port{*this, entry, visual};
    PrepareSortedVisualModel(port); ++stats.models;
  }
  void BeginPrimitives() { Call(bdBeginRenderPass, {Read(kContext), 3}); ++stats.pass_adapters; }
  uint32_t PrimitiveCount() { return Read(kQueues + 1044); }
  uint32_t DeferredCount() { return Read(kQueues + 1052); }
  void PreparePrimitives() {
    for (auto [field, value] : std::array<std::pair<uint32_t, uint32_t>, 8>{{
        {40, 1}, {104, 6}, {96, 1}, {100, 2}, {60, 1}, {56, 0}, {52, 0}, {108, 0}}}) State(field, value);
    // Existing native sampler producers replace the two inline packed writes.
    Call(bdSetSamplerState, {0, 0, 2}); Call(bdSetSamplerState, {0, 4, 2});
  }
  void SortPrimitives(NativeVisualOrder &target) { Check(Sort(target, PrimitiveCount(), false)); }
  void PrepareSharedMaterial() {
    if (Read(uint64_t(Read(kContext)) + 7064) && Read(kLighting) && Read(kMaterial + 352) != 1) {
      Write(kMaterial + 352, 1); Write(kMaterial + 384, 1);
      Write(kMaterial + 408, Read(kMaterial + 408) + 1);
    }
    Call(D3DDevice_SetVertexShaderConstantFN, {Read(kDevice), 0, kMaterial, 5, 3ull << 62});
    Call(D3DDevice_SetPixelShaderConstantFN, {Read(kDevice), 0, kMaterial + 80, 14, 15ull << 60});
    Call(D3DDevice_SetVertexShaderConstantB, {Read(kDevice), 0, kMaterial + 304, 6});
    Call(D3DDevice_SetPixelShaderConstantB, {Read(kDevice), 0, kMaterial + 328, 11});
    Call(sub_821793D0, {(uint32_t(-32035) << 16) + 32120, 0});
    Call(sub_821793D0, {(uint32_t(-32035) << 16) + 32340, 0}); stats.callbacks += 2;
  }
  uint32_t PrimitiveFlags(uint32_t index) { return Read(uint64_t(primitive_base) + uint64_t(index) * 52 + 36); }
  void Defer(uint32_t index, uint32_t count) {
    // Temporary output ABI for sub_824252D0, not a native frame/resolve claim.
    Write(uint64_t(Read(kQueues)) + count * 4, primitive_base + index * 52);
    Write(kQueues + 1052, count + 1); ++stats.deferred;
  }
  void DeferredOverflow() { ++stats.overflow; }
  void SelectMode(uint32_t mode) {
    const uint32_t offset = mode == 4 ? 60 : 48;
    const auto declaration = Read(kChoices + offset + 8);
    if (declaration) Call(bdSetVertexDeclarationCached, {declaration});
    Write(kEngine + 96, Read(kChoices + offset));
    Write(kEngine + 100, Read(kChoices + offset + 4));
    Call(bdInitDefaultTextures, {0});
  }
  void Primitive(uint32_t index) {
    const uint64_t entry = uint64_t(primitive_base) + uint64_t(index) * 52;
    Depth(uint32_t(entry + 36)); Blend(Read(entry + 36));
    Call(D3DDevice_SetTexture, {Read(kDevice), 0, Read(entry + 8), 1ull << 43});
    Call(DrawNativeImmediateUi, {Read(entry + 32), Read(entry + 12), entry + 16, 0});
    ++stats.primitives;
  }
  void ResetColour() {
    const auto plan = BuildImmediateUiImport(Read(kDevice), 0, 0, kResetColour, one, Word);
    Check(plan.has_value());
    plan->Apply([](uint32_t address, uint32_t word) { Write(address, word); });
    PublishNativeShaderParameters(plan->device, false, 3, 1, plan->colour.data());
    Video::MarkPSConstantsDirty();
  }
  void EndPrimitives() { Call(bdEndRenderPass, {Read(kContext)}); ++stats.pass_adapters; }
};

bool Prepare(PPCContext &ctx, uint32_t models, uint32_t primitives) {
  if (models > kSortedModelLimit || primitives > kSortedPrimitiveLimit || ctx.r1.u32 < 4096 ||
      (ctx.r1.u32 & 15) || !Range(uint64_t(ctx.r1.u32) - 4096, 4096) ||
      !rex::system::XThread::GetCurrentThread()) return false;
  const uint64_t scratch = uint64_t(ctx.r1.u32) - 208;
  auto safe = [scratch](uint64_t address, uint64_t bytes) {
    return Range(address, bytes) && !(address < scratch + 208 && scratch < address + bytes);
  };
  for (auto [address, bytes] : std::array<std::pair<uint32_t, uint32_t>, 11>{{
      {kQueues, 1068}, {kContext, 4}, {kDevice, 4}, {kOne, 136}, {kScale, 4},
      {kModelScope, 524}, {kEngine, 104}, {kMaterial, 412}, {kChoices, 72},
      {kResetColour, 20}, {kThread, 4}}}) if (!safe(address, bytes)) return false;
  if (!safe(Read(kContext), 7068) || !safe(Read(kDevice), 9984) || !safe(kLighting, 4)) return false;
  for (auto [count, offset, stride] : std::array<std::array<uint32_t, 3>, 2>{{
      {models, 4, 104}, {primitives, 16, 52}}}) {
    const auto start = Read(kQueues + offset);
    if (count && (!safe(start, uint64_t(count) * stride) || (start & 3))) return false;
  }
  const auto start = Read(kQueues + 4);
  for (uint32_t i = 0; i < models; ++i) {
    const auto entry = uint64_t(start) + i * 104;
    const auto visual = Read(entry + 8);
    if (!visual) continue;
    const bool special = Read(entry + 92) & 0x100000;
    if (!safe(visual, special ? 3460 : 4964) || (visual & 3)) return false;
    const auto table = Read(visual);
    if (!safe(table, 24) || (table & 3)) return false;
    auto *dispatcher = REX_KERNEL_STATE()->function_dispatcher();
    if (!dispatcher->GetFunction(Read(uint64_t(table) + 4)) ||
        (!special && !dispatcher->GetFunction(Read(uint64_t(table) + 20)))) return false;
  }
  return true;
}
} // namespace

void DrawNativeSortedVisuals(PPCContext &ctx, uint8_t *base) {
  const bool enabled = REXCVAR_GET(bd_native_visual_schedule);
  const auto models = Word(kQueues + 1048), primitives = Word(kQueues + 1044);
  if (enabled && !active && models && primitives && !*models && !*primitives) {
    ++stats.empty; Report(); return;
  }
  bool supported = enabled && !active && models && primitives && Prepare(ctx, *models, *primitives);
  if (supported) {
    if (!order) order = std::make_unique<NativeVisualOrder>();
    Adapter adapter(ctx, base);
    // Failure before any publication/callback may use the original exactly once.
    supported = !*models || adapter.Sort(*order, *models, true);
    if (supported) {
      ExecuteVisualSchedule(adapter, *order, *models, *primitives);
      ++stats.native; Report(); return;
    }
  }
  ++stats.compatibility; stats.refused += enabled;
  LegacyShaderParameterScope parameters;
  __imp__Visual__DrawSortedQueues(ctx, base);
  Video::MarkVSConstantsDirty(); Video::MarkPSConstantsDirty(); Report();
}
} // namespace bd::gpu::hooks
