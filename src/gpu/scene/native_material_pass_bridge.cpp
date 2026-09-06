/**
 * @brief Whole material-pass lifecycle and native shader/declaration binding.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_material_pass.h"
#include "gpu/device.h"
#include "gpu/constant_buffers.h"
#include "gpu/frame_stats.h"
#include "gpu/host_resource_heap.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/function_dispatcher.h>

REX_EXTERN(__imp__bdBeginRenderPass);
REX_EXTERN(__imp__bdEndRenderPass);
REX_EXTERN(__imp__bdRenderPassSetTextureState);
REX_EXTERN(__imp__bdInitDefaultTextures);
REX_EXTERN(__imp__bdSetVertexDeclarationCached);
REXCVAR_DEFINE_BOOL(bd_native_material_passes, true, kCvarGroup,
    "Host material-pass lifecycle, recipe selection and shader/declaration binding.");

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kEngine = (uint32_t(-32034) << 16) - 19936;
constexpr uint32_t kChoices = (uint32_t(-32137) << 16) + 29804;
constexpr uint32_t kParticipants = (uint32_t(-32035) << 16) - 26416;
constexpr uint32_t kDeclaration = (uint32_t(-32036) << 16) - 6096 + 512;
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
enum class Operation { Begin, End, Recipe, Shaders, Declaration };
using Original = void (*)(PPCContext &, uint8_t *);
struct Stats {
  std::array<uint64_t, 5> native{};
  uint64_t vertex_binds = 0, pixel_binds = 0, declarations = 0, callbacks = 0;
  uint64_t compatibility = 0, refused = 0, faults = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
thread_local uint32_t nesting = 0;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-material-pass] begin {} end {} recipes {} shaders {} declaration calls {}; "
          "VS binds {} PS binds {} declarations {} participant callbacks {}; "
          "compatibility {} refused {} faults {}; native dispatch/binding, authored registry/recipes "
          "and resource/shader ABI adapters remain",
      stats.native[0], stats.native[1], stats.native[2], stats.native[3], stats.native[4],
      stats.vertex_binds, stats.pixel_binds, stats.declarations, stats.callbacks,
      stats.compatibility, stats.refused, stats.faults);
  stats.frame = frame; stats.reported = true;
}
void Check(bool valid) {
  if (valid) return;
  ++stats.faults; stats.reported = false; Report();
  throw std::runtime_error("Native material pass lost a validated authored input");
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address > UINT32_MAX || bytes > UINT32_MAX ||
      address + bytes - 1 > UINT32_MAX || !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Word(uint64_t address) { return !(address & 3) && Range(address, 4); }
uint32_t Read(uint64_t address) { Check(Word(address)); return bd::mem::load<uint32_t>(uint32_t(address)); }
void Write(uint64_t address, uint32_t value) { Check(Word(address)); bd::mem::store<uint32_t>(uint32_t(address), value); }
struct Adapter {
  PPCContext &ctx;
  uint8_t *base;
  const uint32_t context;
  const uint64_t saved_stack;
  Adapter(PPCContext &input, uint8_t *memory, uint32_t frame_bytes)
      : ctx(input), base(memory), context(ctx.r3.u32), saved_stack(ctx.r1.u64) {
    ++nesting; ctx.r1.u32 -= frame_bytes;
    bd::mem::store<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
  }
  ~Adapter() { ctx.r1.u64 = saved_stack; --nesting; }
  void ZeroResult() { ctx.r3.u64 = 0; }
  uint32_t ShaderRecipe(uint32_t stage) { return Read(kEngine + 96 + stage * 4); }
  void SetShaderRecipe(uint32_t stage, uint32_t value) { Write(kEngine + 96 + stage * 4, value); }
  uint32_t CachedShader(uint32_t stage) { return Read(kEngine + 104 + stage * 4); }
  void CacheShader(uint32_t stage, uint32_t value) { Write(kEngine + 104 + stage * 4, value); }
  uint32_t Shader(uint32_t recipe) { return Read(recipe); }
  void BindShader(uint32_t stage, uint32_t address) {
    // Same resource lookup as the old endpoint, but no D3D/guest call dispatch.
    auto *shader = HostResourceHeap::FromGuest<GuestShader>(address);
    ctx.r3.u64 = Read(kDevice); // preserve the inherited void-call result register
    if (stage == 0) { Video::SetVertexShader(shader); ++stats.vertex_binds; }
    else { Video::SetPixelShader(shader); ++stats.pixel_binds; }
  }
  uint32_t CachedDeclaration() { return Read(kDeclaration); }
  void CacheDeclaration(uint32_t value) { Write(kDeclaration, value); }
  void BindDeclaration(uint32_t address) {
    auto *declaration = HostResourceHeap::FromGuest<GuestVertexDeclaration>(address);
    ctx.r3.u64 = Read(kDevice);
    Video::SetVertexDeclaration(declaration); ++stats.declarations;
  }
  uint64_t RecipeAddress(uint32_t mode) { return uint64_t(kChoices) + 12 + uint32_t(mode * 12); }
  uint32_t RecipeDeclaration(uint32_t mode) { return Read(RecipeAddress(mode) + 8); }
  uint32_t RecipeShader(uint32_t mode, uint32_t stage) { return Read(RecipeAddress(mode) + stage * 4); }
  void SaveShaderRecipe(uint32_t stage, uint32_t value) { Write(uint64_t(context) + 8 + stage * 4, value); }
  uint32_t SavedShaderRecipe(uint32_t stage) { return Read(uint64_t(context) + 8 + stage * 4); }
  void SetActiveMode(uint32_t value) { Write(kChoices + 8, value); }
  uint32_t ActiveMode() { return Read(kChoices + 8); }
  uint32_t FirstParticipant() { return Read(kParticipants); }
  uint32_t NextParticipant(uint32_t participant) { return Read(uint64_t(participant) + 8); }
  uint32_t ParticipantMode(uint32_t participant) { return Read(uint64_t(participant) + 4); }
  void Invoke(uint32_t participant, bool ending) {
    const auto function = Read(uint64_t(Read(participant)) + (ending ? 20 : 16));
    auto *callback = REX_KERNEL_STATE()->function_dispatcher()->GetFunction(function);
    Check(callback != nullptr); ctx.last_indirect_target = function; ctx.r3.u64 = participant;
    ++stats.callbacks;
    callback(ctx, base); // explicitly counted remaining authored material body
  }
};
bool Prepare(PPCContext &ctx, Operation operation, uint32_t frame_bytes) {
  if (ctx.r1.u32 < 4096 || (ctx.r1.u32 & 15) || !Range(uint64_t(ctx.r1.u32) - 4096, 4096)) return false;
  const auto scratch = uint64_t(ctx.r1.u32) - frame_bytes;
  const auto safe = [scratch, frame_bytes](uint64_t address, uint64_t bytes) {
    return !(address & 3) && Range(address, bytes) &&
        !(address < scratch + frame_bytes && scratch < address + bytes);
  };
  if (!safe(kEngine, 112) || !safe(kChoices, 12) || !safe(kParticipants, 4) ||
      !safe(kDeclaration, 4) || !safe(kDevice, 4)) return false;
  if ((operation == Operation::Begin || operation == Operation::End) && !safe(ctx.r3.u32, 16)) return false;
  const auto mode = operation == Operation::Begin ? ctx.r4.u32 : ctx.r3.u32;
  if ((operation == Operation::Begin || operation == Operation::Recipe) && mode &&
      !safe(uint64_t(kChoices) + 12 + uint32_t(mode * 12), 12)) return false;
  return true;
}
void Execute(PPCContext &ctx, uint8_t *base, Operation operation, Original original) {
  const bool enabled = REXCVAR_GET(bd_native_material_passes);
  const auto argument = ctx.r3.u32, mode = ctx.r4.u32;
  const auto frame_bytes = operation == Operation::Begin || operation == Operation::End ? 96u :
      operation == Operation::Shaders ? 128u : 112u;
  if (enabled) Check(nesting < 64);
  if (enabled && Prepare(ctx, operation, frame_bytes)) {
    Adapter adapter(ctx, base, frame_bytes);
    switch (operation) {
    case Operation::Begin: BeginMaterialPass(adapter, mode); break;
    case Operation::End: EndMaterialPass(adapter); break;
    case Operation::Recipe: SelectMaterialRecipe(adapter, argument); break;
    case Operation::Shaders: BindMaterialShaders(adapter, argument != 0); break;
    case Operation::Declaration: SelectMaterialDeclaration(adapter, argument); break;
    }
    ++stats.native[uint32_t(operation)]; Report(); return;
  }
  ++stats.compatibility; stats.refused += enabled;
  LegacyShaderParameterScope parameters;
  original(ctx, base); Report();
}
} // namespace
} // namespace bd::gpu::scene

#define NATIVE_MATERIAL_HOOK(name, operation) \
  REX_HOOK_RAW(name) { \
    bd::gpu::scene::Execute(ctx, base, bd::gpu::scene::Operation::operation, __imp__##name); \
  }
NATIVE_MATERIAL_HOOK(bdBeginRenderPass, Begin)
NATIVE_MATERIAL_HOOK(bdEndRenderPass, End)
NATIVE_MATERIAL_HOOK(bdRenderPassSetTextureState, Recipe)
NATIVE_MATERIAL_HOOK(bdInitDefaultTextures, Shaders)
NATIVE_MATERIAL_HOOK(bdSetVertexDeclarationCached, Declaration)
#undef NATIVE_MATERIAL_HOOK
