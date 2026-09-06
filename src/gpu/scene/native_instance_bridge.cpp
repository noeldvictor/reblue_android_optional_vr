/**
 * @brief Publish instance poses at their producer, consume without palette reads.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_instance_bridge.h"
#include "gpu/scene/native_instance_source.h"
#include "gpu/scene/native_material.h"
#include "gpu/scene/guest_scene.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include "gpu/frame_stats.h"
#include <algorithm>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/xthread.h>

REXCVAR_DEFINE_BOOL(bd_native_instances, true, kCvarGroup,
    "Producer-owned instance poses for native traversal and draw transforms.");
REXCVAR_DECLARE(bool, bd_native_materials_verify);
REX_EXTERN(__imp__bdVisualObjectInitBones);
REX_EXTERN(__imp__sub_82140DF8);
REX_EXTERN(__imp__sub_8213F5E8);

namespace bd::gpu::scene {
namespace {
static_assert(kVisualBoneContainer == instance_source::kPaletteContainer);
struct Store {
  std::mutex mutex;
  NativeInstanceRegistry instances;
  std::unordered_map<uint32_t, instance_source::Binding> sources;
  uint64_t imports = 0, refused = 0, reads = 0, unavailable = 0, checked = 0, wrong = 0;
  uint64_t handoffs = 0, handoff_missing = 0;
  uint32_t miss_examples = 0;
  uint32_t drift_examples = 0;
  uint32_t frame = 0;
};
Store &Instances() { static Store result; return result; }
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address + bytes > uint64_t(UINT32_MAX) + 1 ||
      !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096;
       page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
std::optional<uint32_t> Word(uint64_t address) {
  if ((address & 3) || !Range(address, 4)) return {};
  return bd::mem::load<uint32_t>(uint32_t(address));
}
void Report(Store &store) {
  const auto frame = FrameStatFrameCount();
  if (frame - store.frame < 300) return;
  const auto stats = store.instances.Stats();
  BD_INFO("[native-instances] {} created {} retired {} live / {} bytes; "
          "{} poses published {} reused; {} producer imports {} refused; "
          "{} consumer reads {} unavailable; {} checks wrong {}; "
          "{} handoffs {} unavailable; original pose calculation, secondary palettes and source index remain",
      stats.created, stats.retired, stats.indexed, stats.bytes, stats.published,
      stats.reused, store.imports, store.refused + stats.refused, store.reads,
      store.unavailable, store.checked, store.wrong, store.handoffs, store.handoff_missing);
  store.frame = frame;
}
void Retire(uint32_t visual) {
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  if (const auto it = store.sources.find(visual); it != store.sources.end()) {
    store.instances.Retire(it->second.instance);
    store.sources.erase(it);
  }
}
void Attach(uint32_t visual) {
  if (!rex::system::XThread::GetCurrentThread()) { Retire(visual); return; }
  const auto input_source = instance_source::ReadPublication(
      visual, rex::system::XThread::GetCurrentThreadId(), Word);
  if (!input_source) {
    Retire(visual);
    auto &store = Instances();
    std::lock_guard lock(store.mutex);
    ++store.refused; Report(store); return;
  }
  const auto graph = input_source->graph;
  const auto generation = LoadedNativeModelGeneration(graph);
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  auto it = store.sources.find(visual);
  if (it != store.sources.end() && it->second.model_generation != generation) {
    store.instances.Retire(it->second.instance);
    store.sources.erase(it); it = store.sources.end();
  }
  if (!generation) {
    ++store.refused; Report(store); return;
  }
  if (it == store.sources.end()) {
    const auto id = store.instances.Create(generation);
    if (!id) { Report(store); return; }
    try { it = store.sources.emplace(visual, instance_source::Binding{id, generation, {}}).first; }
    catch (...) { store.instances.Retire(id); throw; }
  }
  // InitBones is not the last writer (runtime 898 proves later edits before
  // handoff). Establish identity here, but import no provisional matrix values.
  Report(store);
}
void Handoff(uint32_t container) {
  if (container < kVisualBoneContainer) return;
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  const auto it = store.sources.find(container - kVisualBoneContainer);
  // The helper also copies unconverted secondary/other containers. Never
  // discover an instance here or infer one from equal matrix values.
  if (it == store.sources.end()) return;
  const auto transfer = instance_source::ReadTransfer(container, Word);
  // Import once at the authoritative publication boundary, after ALL pose
  // writers and the actual conditional copy. Never re-import from a draw lookup.
  thread_local std::array<RenderMatrix, NativeInstanceRegistry::kMaxTransforms> scratch;
  std::span<const RenderMatrix> completed;
  if (transfer && Range(transfer->destination, uint64_t(transfer->count) * sizeof(RenderMatrix))) {
    const auto *input = bd::mem::at<const be_f32>(transfer->destination);
    for (uint32_t matrix = 0; matrix < transfer->count; ++matrix)
      for (uint32_t element = 0; element < 16; ++element)
        scratch[matrix][element] = float(input[matrix * 16 + element]);
    completed = {scratch.data(), transfer->count};
  }
  const bool transferred = instance_source::PublishCompletedTransfer(
      store.instances, it->second, transfer, completed);
  ++store.imports;
  if (!transferred) ++store.refused;
  ++(transferred ? store.handoffs : store.handoff_missing);
  Report(store);
}
} // namespace

std::shared_ptr<const NativeInstancePose> FindNativeInstancePose(
    uint32_t visual, uint32_t graph, uint32_t palette) {
  if (!REXCVAR_GET(bd_native_instances) || !visual || !graph || !palette) return {};
  const auto generation = LoadedNativeModelGeneration(graph);
  auto &store = Instances();
  std::lock_guard lock(store.mutex);
  const auto it = store.sources.find(visual);
  std::shared_ptr<const NativeInstancePose> pose;
  if (it != store.sources.end())
    pose = instance_source::Find(store.instances, it->second, generation, palette);
  if (!pose && REXCVAR_GET(bd_native_materials_verify) && store.miss_examples < 4) {
    const bool found = it != store.sources.end();
    BD_INFO("[native-instance-miss] visual {:08X} graph {:08X} generation {} indexed {} "
            "stored generation {} palette {:08X} stored {:08X}/{:08X}",
        visual, graph, generation, found, found ? it->second.model_generation : 0,
        palette, found ? it->second.palettes[0] : 0, found ? it->second.palettes[1] : 0);
    ++store.miss_examples;
  }
  if (pose && REXCVAR_GET(bd_native_materials_verify)) {
    // Independent consumer-time check catches missed producer/lane updates.
    bool same = Range(palette, pose->transforms.size() * sizeof(RenderMatrix));
    if (same) {
      const auto *source = bd::mem::at<const be_f32>(palette);
      for (size_t m = 0; m < pose->transforms.size(); ++m)
        for (size_t k = 0; k < 16; ++k)
          same &= pose->transforms[m][k] == float(source[m * 16 + k]);
    }
    ++store.checked;
    if (!same) {
      if (store.drift_examples < 3) {
        BD_INFO("[native-instance-drift] at consumer visual {:08X} graph {:08X} palette {:08X}",
            visual, graph, palette);
        ++store.drift_examples;
      }
      ++store.wrong; pose.reset();
    }
  }
  ++(pose ? store.reads : store.unavailable);
  Report(store);
  return pose;
}

bool CopyNativeInstanceWorld(const NodeTag &tag, float out[16]) {
  if (tag.from_list || !tag.ctx_va ||
      uint64_t(tag.palette_va) + uint64_t(tag.node_index) * 64 != tag.matrix_va) return false;
  const auto pose = FindNativeInstancePose(tag.visual_va,
      bd::mem::try_load<uint32_t>(tag.ctx_va + 4), tag.palette_va);
  if (!pose || tag.node_index >= pose->transforms.size()) return false;
  std::copy_n(pose->transforms[tag.node_index].begin(), 16, out);
  return true;
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(bdVisualObjectInitBones) {
  const uint32_t visual = ctx.r3.u32;
  __imp__bdVisualObjectInitBones(ctx, base);
  if (!REXCVAR_GET(bd_native_instances)) {
    bd::gpu::scene::Retire(visual); return;
  }
  try { bd::gpu::scene::Attach(visual); }
  catch (const std::exception &error) {
    bd::gpu::scene::Retire(visual);
    BD_WARN("[native-instances] producer publication failed: {}", error.what());
  }
}

REX_HOOK_RAW(sub_82140DF8) {
  // Full model-unload entry releases both palettes and the shared model, and is
  // called by the base destructor sub_8213FDD0 as well as reload paths.
  bd::gpu::scene::Retire(ctx.r3.u32);
  __imp__sub_82140DF8(ctx, base);
}

REX_HOOK_RAW(sub_8213F5E8) {
  const uint32_t container = ctx.r3.u32;
  // The derived container's actual vtable points here, not to the ungated
  // base helper sub_82144D10. Only state 3 copies and resets the dirty word.
  const bool copied = bd::gpu::scene::instance_source::TransferReady(
      bd::gpu::scene::Word(uint64_t(container) + 28));
  __imp__sub_8213F5E8(ctx, base);
  if (copied && REXCVAR_GET(bd_native_instances)) {
    try { bd::gpu::scene::Handoff(container); }
    catch (const std::exception &error) {
      if (container >= bd::gpu::scene::kVisualBoneContainer)
        bd::gpu::scene::Retire(container - bd::gpu::scene::kVisualBoneContainer);
      BD_WARN("[native-instances] render publication failed: {}", error.what());
    }
  }
}
