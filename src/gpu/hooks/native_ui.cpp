/**
 * @brief Whole immediate UI preparation and submission on the host.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/hooks/native_ui.h"
#include "gpu/hooks/draw_dispatch.h"
#include "gpu/native_ui_vertices.h"
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
#include <memory>
#include <stdexcept>

REX_EXTERN(__imp__Visual__DrawVerticesUP);
REXCVAR_DEFINE_BOOL(bd_native_immediate_ui, true, kCvarGroup,
    "Host-owned immediate UI preparation and vertex submission.");
REXCVAR_DEFINE_BOOL(bd_native_immediate_ui_verify, false, kCvarGroup,
    "Compare native UI preparation/vertices with one original draw; diagnostic only.");

namespace bd::gpu::hooks {
namespace {
constexpr uint32_t kDevice = (uint32_t(-32133) << 16) - 31532;
constexpr uint32_t kDefaultColour = (uint32_t(-32033) << 16) - 14948;
constexpr uint32_t kOne = (uint32_t(-32251) << 16) + 20908;
struct Stats {
  uint64_t native = 0, checked = 0, wrong = 0, compatibility = 0, refused = 0;
  uint64_t empty = 0, translated = 0, vertex_bytes = 0, upload_failures = 0;
  uint32_t frame = 0;
  bool reported = false;
};
thread_local Stats stats;
// Serialized immediate submission; only the CPU copy is reused. UploadHostBytes
// owns the fence-managed GPU lifetime, including later queue drains.
// Lazy heap storage avoids embedding 1.5 MiB of zeroes in the executable's TLS
// image or reserving that much for threads that never submit UI geometry.
thread_local std::unique_ptr<NativeUiVertices<>> vertices;
struct Reference {
  const scene::ImmediateUiImportPlan &plan;
  std::span<const uint32_t> vertices;
  uint32_t observations = 0;
};
thread_local Reference *reference = nullptr;
thread_local bool active = false;
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
void Report() {
  const auto frame = FrameStatFrameCount();
  if (stats.reported && frame - stats.frame < 300) return;
  BD_INFO("[native-immediate-ui] native {} checked {} wrong {} compatibility {} refused {}; "
          "empty {} translated {} vertex bytes {} upload failures {}; "
          "host preparation/geometry, authored vertex sources, sorted scheduler and draw-state adapters remain",
      stats.native, stats.checked, stats.wrong, stats.compatibility, stats.refused,
      stats.empty, stats.translated, stats.vertex_bytes, stats.upload_failures);
  stats.frame = frame; stats.reported = true;
}
void Compare(bool same) {
  if (same) return;
  ++stats.wrong;
  // Report immediately, not only when the periodic reporting interval expires.
  stats.reported = false; Report();
  throw std::runtime_error("Native immediate UI differs from original preparation/upload");
}
struct Scope {
  const bool outer_overlay = state().overlay2DScope, outer_active = active;
  Reference *outer_reference = reference;
  Scope() { state().overlay2DScope = true; active = true; reference = nullptr; }
  ~Scope() {
    state().overlay2DScope = outer_overlay;
    active = outer_active; reference = outer_reference;
    Video::MarkVSConstantsDirty(); Video::MarkPSConstantsDirty();
  }
};
bool Overlap(uint64_t a, uint64_t bytes, uint64_t b, uint64_t other) {
  return bytes && other && a < b + other && b < a + bytes;
}
} // namespace

void ObserveOriginalImmediateUi(uint32_t device, uint32_t primitive,
                                uint32_t count, uint32_t stride, const uint8_t *bytes) {
  if (!reference) return;
  ++reference->observations;
  Compare(reference->observations == 1 && device == reference->plan.device &&
          primitive == 6 && stride == kImmediateUiStride && bytes &&
          reference->vertices.size() == uint64_t(count) * kImmediateUiWords &&
          reference->plan.Matches(Word));
  // Independent read of the scratch actually submitted by the original body;
  // no native publication/geometry is copied over it to make the check pass.
  for (size_t i = 0; i < reference->vertices.size(); ++i) {
    const auto *p = bytes + i * 4;
    const uint32_t original = uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 |
                              uint32_t(p[2]) << 8 | p[3];
    Compare(original == reference->vertices[i]);
  }
}

void DrawNativeImmediateUi(PPCContext &ctx, uint8_t *base) {
  const bool reentered = active;
  Scope scope;
  const uint32_t count = ctx.r3.u32, source = ctx.r4.u32;
  const uint64_t bytes = uint64_t(count) * kImmediateUiStride;
  const bool enabled = REXCVAR_GET(bd_native_immediate_ui);
  const auto device = Word(kDevice), one = Word(kOne);
  std::optional<scene::ImmediateUiImportPlan> plan;
  if (enabled && !reentered && device && one && ImmediateUiCountSupported(count) &&
      ctx.r1.u32 >= 128 && !(ctx.r1.u32 & 15) && Range(uint64_t(ctx.r1.u32) - 128, 128)) {
    const uint64_t old_frame = uint64_t(ctx.r1.u32) - 128;
    ctx.fpscr.disableFlushMode();
    plan = scene::BuildImmediateUiImport(*device, ctx.r5.u32, ctx.r6.u32, kDefaultColour, *one,
        [old_frame](uint64_t address) {
          return Overlap(address, 4, old_frame, 128) ? std::nullopt : Word(address);
        });
    bool supported = plan && (!bytes || (Range(source, bytes) && !Overlap(source, bytes, old_frame, 128)));
    if (supported && bytes) {
      // Rare self-modifying/stack aliases retain the explicit original path;
      // never snapshot geometry before parameter writes that would change it.
      for (uint32_t i = 0; i < plan->size; ++i)
        supported &= !Overlap(source, bytes, plan->writes[i].address, 4);
    }
    if (supported && !vertices) vertices = std::make_unique<NativeUiVertices<>>();
    if (!supported || !vertices->Import(
        {bytes ? bd::mem::at<const uint8_t>(source) : nullptr, size_t(bytes)}, count)) plan.reset();
  }
  if (!plan) {
    ++stats.compatibility; stats.refused += enabled;
    LegacyShaderParameterScope parameters;
    __imp__Visual__DrawVerticesUP(ctx, base);
    Report(); return;
  }
  stats.empty += count == 0; stats.translated += plan->translated;
  stats.vertex_bytes += bytes;
  if (REXCVAR_GET(bd_native_immediate_ui_verify)) {
    Reference expected{*plan, vertices->Words()};
    reference = &expected;
    LegacyShaderParameterScope parameters;
    __imp__Visual__DrawVerticesUP(ctx, base);
    reference = nullptr;
    Compare(ctx.r3.u64 == 0 && expected.observations == uint32_t(count != 0));
    if (!count) Compare(plan->Matches(Word));
    ++stats.checked;
  } else {
    plan->Apply([](uint32_t address, uint32_t word) { bd::mem::store<uint32_t>(address, word); });
    PublishNativeShaderParameters(plan->device, false, 3, 1, plan->colour.data());
    if (plan->translated)
      PublishNativeShaderParameters(plan->device, true, 20, 1, plan->translation.data());
    Video::MarkVSConstantsDirty(); Video::MarkPSConstantsDirty();
    if (!DispatchHostImmediateUi(plan->device, vertices->Words())) {
      ++stats.upload_failures;
      throw std::runtime_error("Native immediate UI vertex upload failed");
    }
    ctx.r3.u64 = 0;
    ++stats.native;
  }
  Report();
}
} // namespace bd::gpu::hooks
