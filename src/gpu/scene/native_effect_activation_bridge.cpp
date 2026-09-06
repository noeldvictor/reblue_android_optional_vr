/**
 * @brief   Native effect activation, registry mutation and preparation lifecycle.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_effect_activation.h"
#include "gpu/scene/native_effect_lifecycle.h"
#include "gpu/scene/native_registry_array.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "gpu/frame_stats.h"
#include <array>
#include <cstring>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>
#include <rex/system/function_dispatcher.h>
#include <stdexcept>

REX_EXTERN(__imp__sub_82173DF8);
REX_EXTERN(__imp__sub_8221D678);
REX_EXTERN(__imp__sub_8221D9A8);
REX_EXTERN(__imp__sub_8221D530);
REX_EXTERN(__imp__sub_8221DB00);
REX_EXTERN(__imp__sub_8221D548);
REX_EXTERN(__imp__sub_8221DBE0);
REX_EXTERN(__imp__sub_8221DCA0);
REX_EXTERN(__imp__sub_8221D5E0);
REX_EXTERN(sub_8221D678);
REX_EXTERN(sub_8221D9A8);
REX_EXTERN(sub_826BE0A8); // temporary shared array allocator, not registry logic
REX_EXTERN(sub_826BEF30);
REXCVAR_DECLARE(bool, bd_native_passes);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kRegistry = (uint32_t(-32030) << 16) - 31132;
constexpr uint32_t kShadow = (uint32_t(-32035) << 16) + 24832;
constexpr uint32_t kReflection = (uint32_t(-32035) << 16) + 29040;
constexpr uint32_t kPair = (uint32_t(-32035) << 16) + 32120;
constexpr uint32_t kIndexed = (uint32_t(-32035) << 16) + 25248;
constexpr uint32_t kCount = (uint32_t(-32137) << 16) + 16748;
constexpr uint32_t kAuxiliary = (uint32_t(-32035) << 16) + 28608;
constexpr uint32_t kPost = (uint32_t(-32136) << 16) + 14888;
struct Stats {
  uint64_t activations = 0, registrations = 0, removals = 0, membership = 0;
  uint64_t compatibility = 0, refused = 0, faults = 0, metadata = 0;
  uint64_t allocations = 0, insertions = 0, erasures = 0;
  uint32_t frame = 0;
};
thread_local Stats stats;
struct LifecycleStats {
  std::array<uint64_t, 2> prepares{}, finishes{};
  uint64_t begin_callbacks = 0, end_callbacks = 0, resource_begins = 0, resource_ends = 0;
  uint64_t destroyed = 0, frees = 0, compatibility = 0, refused = 0;
};
thread_local LifecycleStats lifecycle;
void Report() {
  const auto frame = FrameStatFrameCount();
  if (frame - stats.frame < 300) return;
  BD_INFO("[native-effect-activation] updates {} registrations {} removals {} membership {}; "
          "compatibility {} refused {} faults {}; metadata callbacks {} array allocations {} "
          "insertions {} erasures {}; native policy/order/mutation, imported flags, metadata and array storage",
          stats.activations, stats.registrations, stats.removals, stats.membership,
          stats.compatibility, stats.refused, stats.faults, stats.metadata,
          stats.allocations, stats.insertions, stats.erasures);
  BD_INFO("[native-effect-lifecycle] model prepare {} finish {} visual prepare {} finish {}; "
          "participant callbacks begin {} end {} resource begin {} end {}; destroyed {} frees {}; "
          "compatibility {} refused {} shared faults {}; host preparation/cleanup/teardown, "
          "imported callbacks, identities and shared storage",
          lifecycle.prepares[0], lifecycle.finishes[0], lifecycle.prepares[1], lifecycle.finishes[1],
          lifecycle.begin_callbacks, lifecycle.end_callbacks, lifecycle.resource_begins, lifecycle.resource_ends,
          lifecycle.destroyed, lifecycle.frees, lifecycle.compatibility, lifecycle.refused, stats.faults);
  stats.frame = frame;
}
bool Range(uint64_t address, uint64_t bytes) {
  if (!address || !bytes || address + bytes - 1 > UINT32_MAX ||
      !bd::mem::try_at<uint8_t>(uint32_t(address))) return false;
  for (uint64_t page = (address & ~uint64_t(4095)) + 4096; page < address + bytes; page += 4096)
    if (!bd::mem::try_at<uint8_t>(uint32_t(page))) return false;
  return true;
}
bool Words(uint64_t address, uint64_t bytes) { return !(address & 3) && Range(address, bytes); }
void Check(bool valid) {
  if (!valid) { ++stats.faults; throw std::runtime_error("Native effect registry lost a validated import"); }
}
template <class T> T Read(uint32_t address) {
  Check(Range(address, sizeof(T)) && (sizeof(T) < 4 || !(address & 3)));
  return bd::mem::load<T>(address);
}
template <class T> void Write(uint32_t address, T value) {
  Check(Range(address, sizeof(T)) && (sizeof(T) < 4 || !(address & 3)));
  bd::mem::store<T>(address, value);
}
uint32_t VirtualAddress(uint32_t object, uint32_t slot) {
  if (!Words(object, 4)) return 0;
  const auto table = bd::mem::load<uint32_t>(object);
  return Words(uint64_t(table) + slot, 4) ? bd::mem::load<uint32_t>(table + slot) : 0;
}
bool StackReady(PPCContext &ctx) {
  return ctx.r1.u32 >= 1024 && !(ctx.r1.u32 & 15) && Words(uint64_t(ctx.r1.u32) - 1024, 1120);
}
bool ArrayReady(uint32_t record) {
  if (!Words(record, 12)) return false;
  const auto count = bd::mem::load<int32_t>(record + 8);
  const auto capacity = bd::mem::load<int32_t>(record + 4);
  return count >= 0 && capacity >= count &&
      (!capacity || Words(bd::mem::load<uint32_t>(record), uint64_t(capacity) * 4));
}
bool RegistryReady(uint32_t registry, uint32_t participant) {
  if (!Words(registry, 36)) return false;
  for (uint32_t group = 0; group < 3; ++group)
    if (!ArrayReady(registry + group * 12)) return false;
  for (uint32_t slot : {24u, 28u}) {
    const auto address = VirtualAddress(participant, slot);
    if (!address || !REX_KERNEL_STATE()->function_dispatcher()->GetFunction(address)) return false;
  }
  return true;
}
struct CallScope {
  PPCContext &ctx;
  uint8_t *base;
  const uint64_t saved_stack;
  CallScope(PPCContext &context, uint8_t *memory) : ctx(context), base(memory), saved_stack(ctx.r1.u64) {
    ctx.r1.u32 -= 256;
    Write<uint32_t>(ctx.r1.u32, uint32_t(saved_stack));
  }
  ~CallScope() { ctx.r1.u64 = saved_stack; }
  uint32_t Call(PPCFunc *fn, uint32_t first, uint32_t second = 0) {
    ctx.r3.u64 = first; ctx.r4.u64 = second;
    fn(ctx, base);
    return ctx.r3.u32;
  }
  uint32_t Virtual(uint32_t participant, uint32_t slot) {
    const auto address = VirtualAddress(participant, slot);
    auto *fn = address ? REX_KERNEL_STATE()->function_dispatcher()->GetFunction(address) : nullptr;
    Check(fn != nullptr);
    ctx.last_indirect_target = address;
    ++stats.metadata;
    return Call(fn, participant);
  }
};
struct RegistryAdapter : CallScope {
  const uint32_t registry;
  RegistryAdapter(PPCContext &ctx, uint8_t *base) : CallScope(ctx, base), registry(ctx.r3.u32) {}
  RegistryAdapter(PPCContext &ctx, uint8_t *base, uint32_t address) : CallScope(ctx, base), registry(address) {}
  uint32_t Record(uint32_t group) { return registry + group * 12; }
  int32_t Count(uint32_t group) { return Read<int32_t>(Record(group) + 8); }
  uint32_t Mask(uint32_t participant) { return Virtual(participant, 28); }
  int32_t Priority(uint32_t participant) { return int32_t(Virtual(participant, 24)); }
  uint32_t Entry(uint32_t group, int32_t index) {
    Check(index >= 0);
    const uint64_t address = uint64_t(Read<uint32_t>(Record(group))) + uint64_t(index) * 4;
    Check(Words(address, 4));
    return Read<uint32_t>(uint32_t(address));
  }
  uint32_t Allocate(uint32_t capacity) {
    Check(capacity && capacity <= UINT32_MAX / 4);
    const auto data = Call(sub_826BE0A8, capacity * 4);
    Check(Words(data, uint64_t(capacity) * 4));
    ++stats.allocations;
    return data;
  }
  void Copy(uint32_t destination, uint32_t source, uint32_t count) {
    if (!count) return;
    Check(Words(destination, uint64_t(count) * 4) && Words(source, uint64_t(count) * 4));
    std::memmove(bd::mem::at<uint8_t>(destination), bd::mem::at<uint8_t>(source), size_t(count) * 4);
  }
  struct Slots {
    RegistryAdapter &owner;
    uint32_t record;
    Slots(RegistryAdapter &adapter, uint32_t group) : owner(adapter), record(adapter.Record(group)) {
      Check(ArrayReady(record));
    }
    uint32_t Count() { return bd::gpu::scene::Read<uint32_t>(record + 8); }
    uint32_t Capacity() { return bd::gpu::scene::Read<uint32_t>(record + 4); }
    uint32_t MaxCapacity() { return UINT32_MAX / 4; }
    bool HasStorage() { return bd::gpu::scene::Read<uint32_t>(record) != 0; }
    uint32_t Address(uint32_t index) {
      const uint64_t address = uint64_t(bd::gpu::scene::Read<uint32_t>(record)) + uint64_t(index) * 4;
      Check(Words(address, 4));
      return uint32_t(address);
    }
    uint32_t Read(uint32_t index) { return bd::gpu::scene::Read<uint32_t>(Address(index)); }
    void Write(uint32_t index, uint32_t value) { bd::gpu::scene::Write<uint32_t>(Address(index), value); }
    void SetCount(uint32_t count) { bd::gpu::scene::Write<uint32_t>(record + 8, count); }
    void Reserve(uint32_t capacity, bool zero_tail) {
      const auto old_capacity = Capacity();
      const auto old_data = bd::gpu::scene::Read<uint32_t>(record);
      const auto next = owner.Allocate(capacity);
      // Preserve capacity contents for remaining engine readers. Insertion's
      // unused new tail is unspecified; append growth explicitly zero-fills it.
      owner.Copy(next, old_data, old_capacity);
      if (zero_tail)
        for (uint32_t i = old_capacity; i < capacity; ++i) bd::gpu::scene::Write<uint32_t>(next + i * 4, 0);
      if (old_data) owner.Call(sub_826BEF30, old_data);
      bd::gpu::scene::Write<uint32_t>(record, next);
      bd::gpu::scene::Write<uint32_t>(record + 4, capacity);
    }
  };
  void Insert(uint32_t group, int32_t index, uint32_t participant) {
    Check(index >= 0);
    Slots slots(*this, group);
    InsertRegistryEntry(slots, uint32_t(index), participant);
    ++stats.insertions;
  }
  void Append(uint32_t group, uint32_t participant) {
    Slots slots(*this, group);
    AppendRegistryEntry(slots, participant);
    ++stats.insertions;
  }
  void Erase(uint32_t group, int32_t index) {
    Check(index >= 0);
    Slots slots(*this, group);
    EraseRegistryEntry(slots, uint32_t(index));
    ++stats.erasures;
  }
  uint32_t Storage(uint32_t group) { return Read<uint32_t>(Record(group)); }
  void Free(uint32_t storage) { Call(sub_826BEF30, storage); ++lifecycle.frees; }
  void SetStorage(uint32_t group, uint32_t storage) { Write<uint32_t>(Record(group), storage); }
  void SetCapacity(uint32_t group, uint32_t capacity) { Write<uint32_t>(Record(group) + 4, capacity); }
  void SetCount(uint32_t group, uint32_t count) { Write<uint32_t>(Record(group) + 8, count); }
};
struct PreparationAdapter : RegistryAdapter {
  const uint32_t group, input, extra;
  PreparationAdapter(PPCContext &ctx, uint8_t *base, uint32_t registry,
                     uint32_t kind, uint32_t subject = 0, uint32_t argument = 0)
      : RegistryAdapter(ctx, base, registry), group(kind), input(subject), extra(argument) {}
  int32_t Count() { return RegistryAdapter::Count(group); }
  uint32_t Participant(int32_t index) {
    const auto participant = Entry(group, index);
    Check(Words(participant, 6));
    return participant;
  }
  uint32_t Invoke(uint32_t object, uint32_t slot, bool preparing = false) {
    const auto address = VirtualAddress(object, slot);
    auto *fn = address ? REX_KERNEL_STATE()->function_dispatcher()->GetFunction(address) : nullptr;
    Check(fn != nullptr);
    ctx.r3.u64 = object;
    if (preparing) { ctx.r4.u64 = input; ctx.r5.u64 = extra; }
    ctx.last_indirect_target = address;
    fn(ctx, base);
    return ctx.r3.u32;
  }
  int32_t Begin(int32_t index) {
    const auto result = Invoke(Participant(index), group * 8, true);
    ++lifecycle.begin_callbacks;
    return int32_t(result);
  }
  void End(int32_t index) { Invoke(Participant(index), group * 8 + 4); ++lifecycle.end_callbacks; }
  uint8_t Active(int32_t index) { return Read<uint8_t>(Participant(index) + 4 + group); }
  void SetActive(int32_t index, uint8_t active) { Write<uint8_t>(Participant(index) + 4 + group, active); }
  uint32_t InputResource() { return Read<uint32_t>(input + 4); }
  uint32_t Resource() { return Read<uint32_t>(registry + 36); }
  void SetResource(uint32_t resource) { Write<uint32_t>(registry + 36, resource); }
  void BeginResource(uint32_t resource) { Invoke(resource, 32); ++lifecycle.resource_begins; }
  void EndResource(uint32_t resource) { Invoke(resource, 36); ++lifecycle.resource_ends; }
};
bool PreparationReady(PPCContext &ctx, uint32_t registry, uint32_t group) {
  return StackReady(ctx) && Words(registry, 40) && ArrayReady(registry + group * 12);
}
bool TryPrepareEffect(PPCContext &ctx, uint8_t *base, uint32_t registry,
                      uint32_t group, uint32_t input, uint32_t extra) {
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !PreparationReady(ctx, registry, group) || (!group && !Words(input, 8))) {
    ++lifecycle.compatibility; lifecycle.refused += enabled;
    return false; // the only refusal point, before any native side effects
  }
  PreparationAdapter adapter(ctx, base, registry, group, input, extra);
  ctx.r3.s64 = group ? PrepareEffectParticipants(adapter) : PrepareEffectModel(adapter);
  ++lifecycle.prepares[group];
  return true;
}
bool TryFinishEffect(PPCContext &ctx, uint8_t *base, uint32_t registry, uint32_t group) {
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !PreparationReady(ctx, registry, group)) {
    ++lifecycle.compatibility; lifecycle.refused += enabled;
    return false;
  }
  PreparationAdapter adapter(ctx, base, registry, group);
  if (group) FinishEffectParticipants(adapter);
  else FinishEffectModel(adapter);
  ++lifecycle.finishes[group];
  return true;
}
bool TryDestroyEffectRegistry(PPCContext &ctx, uint8_t *base) {
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !StackReady(ctx) || !Words(kRegistry, 40) ||
      !ArrayReady(kRegistry) || !ArrayReady(kRegistry + 12) || !ArrayReady(kRegistry + 24)) {
    ++lifecycle.compatibility; lifecycle.refused += enabled;
    return false;
  }
  RegistryAdapter adapter(ctx, base, kRegistry);
  DestroyEffectRegistry(adapter);
  ++lifecycle.destroyed;
  return true;
}
struct ActivationAdapter : CallScope {
  const uint32_t effects;
  ActivationAdapter(PPCContext &ctx, uint8_t *base) : CallScope(ctx, base), effects(ctx.r3.u32) {}
  uint32_t Offset(RenderFeature feature) { const auto i = uint32_t(feature); return i <= 7 ? i + 12 : i + 10; }
  uint8_t Cached(RenderFeature feature) { return Read<uint8_t>(effects + Offset(feature)); }
  void SetCached(RenderFeature feature, uint8_t value) { Write<uint8_t>(effects + Offset(feature), value); }
  void SetMode(uint32_t value) { Write<uint32_t>(effects + 24, value); }
  void SetPost(uint8_t value) { Write<uint8_t>(kPost + 12, value); }
  uint32_t Participant(EffectParticipantGroup group, int32_t index) {
    Check(index >= 0);
    switch (group) {
    case EffectParticipantGroup::Shadow: return kShadow;
    case EffectParticipantGroup::Reflection: return kReflection + uint32_t(index) * 280;
    case EffectParticipantGroup::Pair: return kPair + uint32_t(index) * 220;
    case EffectParticipantGroup::Indexed: Check(index < 8); return kIndexed + uint32_t(index) * 420;
    case EffectParticipantGroup::Auxiliary: return kAuxiliary;
    }
    throw std::runtime_error("Invalid effect participant group");
  }
  bool ChangeAllowed(uint32_t participant) { return Read<uint8_t>(participant + 9) != 0; }
  uint8_t Active(uint32_t participant) { return Read<uint8_t>(participant + 8); }
  void SetActive(uint32_t participant, uint8_t value) { Write<uint8_t>(participant + 8, value); }
  void Membership(uint32_t participant, bool active) {
    Call(active ? sub_8221D678 : sub_8221D9A8, kRegistry, participant);
    ++stats.membership;
  }
  int32_t IndexedCount() { const auto count = Read<int32_t>(kCount); Check(count >= 0 && count <= 8); return count; }
};
bool ActivationReady(PPCContext &ctx) {
  if (!StackReady(ctx) || !Words(ctx.r3.u32, 28) || ctx.r5.u32 > 11 || !Words(kRegistry, 36)) return false;
  for (auto [address, size] : std::array<std::pair<uint32_t, uint32_t>, 7>{{
       {kShadow, 10}, {kReflection, 2800 + 10}, {kPair, 230}, {kIndexed, 3360},
       {kAuxiliary, 10}, {kPost, 13}, {kCount, 4}}})
    if (!Range(address, size)) return false;
  if (ctx.r5.u32 == 10) {
    const auto count = bd::mem::load<int32_t>(kCount);
    if (count < 0 || count > 8) return false;
  }
  return true;
}
} // namespace
} // namespace bd::gpu::scene

REX_HOOK_RAW(sub_82173DF8) {
  using namespace bd::gpu::scene;
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !ActivationReady(ctx)) {
    ++stats.compatibility; stats.refused += enabled;
    __imp__sub_82173DF8(ctx, base);
  } else {
    const auto feature = RenderFeature(ctx.r5.u32);
    const auto value = ctx.r4.u8;
    ActivationAdapter adapter(ctx, base);
    ApplyRenderFeature(adapter, feature, value);
    ++stats.activations;
  }
  Report();
}
REX_HOOK_RAW(sub_8221D678) {
  using namespace bd::gpu::scene;
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !StackReady(ctx) || !RegistryReady(ctx.r3.u32, ctx.r4.u32)) {
    ++stats.compatibility; stats.refused += enabled;
    __imp__sub_8221D678(ctx, base);
  } else {
    const auto participant = ctx.r4.u32;
    RegistryAdapter adapter(ctx, base);
    RegisterEffectParticipant(adapter, participant);
    ++stats.registrations;
  }
  Report();
}
REX_HOOK_RAW(sub_8221D9A8) {
  using namespace bd::gpu::scene;
  const bool enabled = REXCVAR_GET(bd_native_passes);
  if (!enabled || !StackReady(ctx) || !RegistryReady(ctx.r3.u32, ctx.r4.u32)) {
    ++stats.compatibility; stats.refused += enabled;
    __imp__sub_8221D9A8(ctx, base);
  } else {
    const auto participant = ctx.r4.u32;
    RegistryAdapter adapter(ctx, base);
    UnregisterEffectParticipant(adapter, participant);
    ++stats.removals;
  }
  Report();
}
REX_HOOK_RAW(sub_8221D530) {
  using namespace bd::gpu::scene;
  if (!TryPrepareEffect(ctx, base, kRegistry, 0, ctx.r3.u32, ctx.r4.u32))
    __imp__sub_8221D530(ctx, base);
  Report();
}
REX_HOOK_RAW(sub_8221DB00) {
  using namespace bd::gpu::scene;
  if (!TryPrepareEffect(ctx, base, ctx.r3.u32, 0, ctx.r4.u32, ctx.r5.u32))
    __imp__sub_8221DB00(ctx, base);
  Report();
}
REX_HOOK_RAW(sub_8221D548) {
  using namespace bd::gpu::scene;
  if (!TryFinishEffect(ctx, base, kRegistry, 0)) __imp__sub_8221D548(ctx, base);
  Report();
}
REX_HOOK_RAW(sub_8221DBE0) {
  using namespace bd::gpu::scene;
  if (!TryPrepareEffect(ctx, base, ctx.r3.u32, 1, ctx.r4.u32, ctx.r5.u32))
    __imp__sub_8221DBE0(ctx, base);
  Report();
}
REX_HOOK_RAW(sub_8221DCA0) {
  using namespace bd::gpu::scene;
  if (!TryFinishEffect(ctx, base, ctx.r3.u32, 1)) __imp__sub_8221DCA0(ctx, base);
  Report();
}
REX_HOOK_RAW(sub_8221D5E0) {
  using namespace bd::gpu::scene;
  if (!TryDestroyEffectRegistry(ctx, base)) __imp__sub_8221D5E0(ctx, base);
  Report();
}
