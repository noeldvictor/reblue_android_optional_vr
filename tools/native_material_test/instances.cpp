#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_instance_source.h"
#include "gpu/scene/native_object_primitive.h"
#include <barrier>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <unordered_map>

using namespace bd::gpu::scene;
namespace {
void Require(bool valid, const char *message) {
  if (!valid) throw std::runtime_error(message);
}
RenderMatrix World(float x) {
  RenderMatrix m{};
  m[0] = m[5] = m[10] = m[15] = 1; m[12] = x;
  return m;
}
void TestSourceHandoff() {
  using namespace instance_source;
  constexpr uint32_t visual = 0x10000, container = visual + 2632;
  // Independent literal addresses model the actual producer/getter/copy
  // layout. +2624 is deliberately a valid but unrelated pointer.
  std::unordered_map<uint64_t, uint32_t> words{
      {visual + 2620, 0x20000}, {visual + 1868, 2}, {visual + 2624, 0xDEAD},
      {kUpdateThread, 37}, {container + 8, 0x30000}, {container + 20, 0x40000},
      {0x30000, 0x50000}, {0x40000, 0x60000}, {0x30008, 2}};
  auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    if ((address & 3) || address > UINT32_MAX) return {};
    const auto it = words.find(address);
    return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  const auto update = ReadPublication(visual, 37, read);
  const auto render = ReadPublication(visual, 98, read);
  Require(update && update->graph == 0x20000 && update->count == 2 &&
          update->lane == 0 && update->palette == 0x50000 && render &&
          render->lane == 1 && render->palette == 0x60000, "actual source layout and thread selection");
  Require(!ReadPublication(UINT32_MAX - 100, 37, read) && !Palette(container, 2, read),
          "source address overflow and lane limits");
  NativeInstanceRegistry registry;
  Binding binding{registry.Create(4), 4, {update->palette, 0}};
  std::vector<RenderMatrix> matrices{World(2), World(4)};
  Require(registry.Publish(binding.instance, update->lane, matrices), "publish decoded producer input");
  Require(!Find(registry, binding, 4, render->palette),
          "producer publication alone is not a render handoff (original regression)");
  const auto bytes = registry.Stats().bytes;
  const auto transfer = ReadTransfer(container, read);
  for (uint32_t flags : {0, 1, 2, 3, 4, 7})
    Require(TransferReady(flags) == (flags == 3), "actual derived copy requires exactly dirty state 3");
  Require(!TransferReady(std::nullopt), "unreadable dirty word cannot advance render pose");
  matrices[1] = World(44); // late pose writer after InitBones, before render copy
  Require(transfer && transfer->source == 0x50000 && transfer->destination == 0x60000 &&
          transfer->count == 2 && PublishCompletedTransfer(registry, binding, transfer, matrices),
          "actual copy boundary imports the final pose, including late writers");
  auto pose = Find(registry, binding, 4, render->palette);
  Require(pose == registry.Read(binding.instance, 0) && registry.Stats().bytes == bytes,
          "render handoff shares the immutable producer pose without copying or residency growth");
  Require(pose->transforms[1][12] == 44 && !Find(registry, binding, 4, update->palette),
          "final publication includes late edits, and does not expose the changing update source");
  matrices[0] = World(8);
  Require(registry.Publish(binding.instance, 0, matrices), "next update before render handoff");
  Require(Find(registry, binding, 4, render->palette) == pose && pose->transforms[0][12] == 2,
          "update cannot advance the render snapshot early");
  Require(ApplyTransfer(registry, binding, transfer) &&
          Find(registry, binding, 4, render->palette)->transforms[0][12] == 8,
          "render advances only at the explicit handoff");
  Require(!Find(registry, binding, 5, render->palette) && !Find(registry, binding, 4, 0x9999),
          "wrong generation or untracked secondary palette cannot use primary pose");
  words[0x30008] = 1;
  Require(!ApplyTransfer(registry, binding, ReadTransfer(container, read)) &&
          !Find(registry, binding, 4, render->palette), "partial copy cannot publish stale render pose");
  words[0x30008] = 2;
  Require(ApplyTransfer(registry, binding, ReadTransfer(container, read)), "handoff recovers");
  words[0x30000] = 0x70000;
  Require(!ApplyTransfer(registry, binding, ReadTransfer(container, read)),
          "unpublished source reallocation invalidates render publication");
  words.erase(0x40000);
  Require(!ReadTransfer(container, read), "missing destination refused");
  words[visual + 1868] = NativeInstanceRegistry::kMaxTransforms + 1;
  Require(!ReadPublication(visual, 37, read), "source count bounded before import");
  words.clear(); matrices.clear(); matrices.shrink_to_fit();
  Require(pose->transforms[0][12] == 2, "leased native pose survives source destruction");
  registry.Retire(binding.instance);
  Require(!Find(registry, binding, 4, render->palette), "unload removes consumer visibility");
  pose.reset();
  Require(registry.Stats().bytes == 0, "handoff leases retire once, without duplicate accounting");
}

void TestRenderPoses() {
  NativeInstanceRegistry registry;
  const auto id = registry.Create(41);
  std::vector<RenderMatrix> matrices{World(0), World(.25f)};
  const auto publish = [&](uint64_t tick, bool active = true) {
    Require(registry.Publish(id, 0, matrices) && registry.Transfer(id, 0, 1, matrices.size()) &&
            registry.ObserveRenderTick(id, tick, active), "completed render tick publication");
    return registry.Read(id, 1);
  };
  auto first = publish(10);
  Require(registry.ReadRender(first, {10,100,.5f,true}) == first, "first pose snaps without invented history");
  matrices[0] = World(1); matrices[1] = World(1.25f);
  auto current = publish(11);
  const NativePosePhase phase{11,101,.25f,true};
  auto quarter = registry.ReadRender(current, phase);
  Require(quarter && quarter != current && quarter->transforms[0][12] == .25f &&
          quarter->transforms[1][12] == .5f, "whole native pose interpolates the completed endpoints");
  const auto bytes = registry.Stats().bytes;
  Require(registry.ReadRender(current, phase) == quarter && registry.Stats().bytes == bytes,
          "culling and both draw views reuse exactly one frame pose");
  Require(!registry.ReadRender(current, {11,101,.75f,true}) &&
          !registry.ReadRender(current, {11,100,.25f,true}), "conflicting or older frame request refuses");
  auto later = registry.ReadRender(current, {11,102,.75f,true});
  Require(later && later->transforms[0][12] == .75f && quarter->transforms[0][12] == .25f &&
          registry.Read(id,1)->transforms[0][12] == 1, "render phases never mutate source comparison or pinned poses");
  NativeObjectPrimitive<int> packet; packet.pose = current; packet.node = 1; packet.world = matrices[1];
  Require(BindNativeRenderPose(packet, quarter) && packet.pose == quarter && packet.world[12] == .5f,
          "production packet binds matching rigid world and skin pose before planning");
  auto foreign = std::make_shared<NativeInstancePose>(*quarter); ++foreign->instance;
  Require(!BindNativeRenderPose(packet, foreign) && packet.pose == quarter, "foreign render pose cannot replace a packet");
  foreign = std::make_shared<NativeInstancePose>(*quarter); ++foreign->model_generation;
  Require(!BindNativeRenderPose(packet, foreign), "render packet generation must match");
  foreign = std::make_shared<NativeInstancePose>(*quarter); foreign->transforms.pop_back();
  Require(!BindNativeRenderPose(packet, foreign), "incomplete render pose cannot bind a packet");
  foreign = std::make_shared<NativeInstancePose>(*current);
  Require(!registry.ReadRender(foreign, {11,103,.5f,true}), "matching IDs do not impersonate the exact completed publication");
  matrices[0] = World(1.5f); matrices[1] = World(1.75f);
  current = publish(11); // another completed writer in the SAME logic tick
  auto late = registry.ReadRender(current, {11,103,.5f,true});
  Require(late && late->transforms[0][12] == .75f,
          "same-tick late writes replace current without advancing previous");
  Require(registry.ObserveRenderTick(id,11,true) && registry.ReadRender(current,{11,103,.5f,true}) == late,
          "repeated unchanged handoff preserves the frame lease");
  current = publish(12);
  Require(registry.ReadRender(current,{12,104,.5f,true}) == current, "unchanged tick endpoints share storage");
  matrices[0] = World(1.75f); current = publish(14);
  Require(registry.ReadRender(current,{14,105,.5f,true}) == current, "skipped ticks snap rather than borrow stale history");
  matrices[0] = World(2); current = publish(15);
  Require(registry.ReadRender(current,{15,106,.5f,true})->transforms[0][12] == 1.875f, "contiguous history recovers");
  matrices[0] = World(2.25f); current = publish(13);
  Require(registry.ReadRender(current,{13,107,.5f,true}) == current, "clock rewind snaps");
  current = publish(13,false);
  Require(registry.ReadRender(current,{13,108,0,false}) == current, "disabled interpolation uses current authored pose");
  matrices[0] = World(2.5f); current = publish(14);
  Require(registry.ReadRender(current,{14,109,.5f,true}) == current, "re-enable does not interpolate across a coupled event");
  matrices[0] = World(20); current = publish(15);
  Require(registry.ReadRender(current,{15,110,.5f,true}) == current, "teleport snaps the whole pose");
  matrices[0] = World(20.25f); matrices[1][15] = 2; current = publish(16);
  Require(registry.ReadRender(current,{16,111,.5f,true}) == current, "nonaffine endpoint is never blended");
  Require(!registry.ReadRender(current,{16,112,std::numeric_limits<float>::quiet_NaN(),true}) &&
          !registry.ReadRender(current,{16,112,-.1f,true}) && !registry.ReadRender(current,{16,112,1.1f,true}),
          "invalid render phase refuses");
  Require(registry.ReadRender(current,{16,112,0,true}) == current &&
          registry.ReadRender(current,{16,113,1,true}) == current, "existing alpha-zero and completed-phase contract");
  registry.Invalidate(id,1);
  Require(!registry.ReadRender(current,{16,114,.5f,true}), "invalidated render publication cannot retain a current interpolation");
  Require(packet.pose->transforms[0][12] == .25f, "queued pose remains immutable after invalidation");
  registry.Retire(id);
  Require(!registry.ReadRender(current,{16,115,.5f,true}) && registry.Stats().bytes > 0,
          "retired in-flight render poses stay charged but cannot be rediscovered");
  packet.pose.reset(); first.reset(); current.reset(); quarter.reset(); later.reset(); late.reset(); foreign.reset();
  Require(registry.Stats().bytes == 0, "source, history and render leases retire without duplicate accounting");

  NativeInstanceRegistry probe;
  const auto probe_id = probe.Create(1);
  auto value = World(0);
  Require(probe.Publish(probe_id,0,{&value,1}), "render budget probe");
  const auto pose_bytes = probe.Stats().bytes - NativeInstanceRegistry::kEntryBytes;
  NativeInstanceRegistry bounded(NativeInstanceRegistry::kEntryBytes + 3*pose_bytes);
  const auto bounded_id = bounded.Create(1);
  Require(bounded.Publish(bounded_id,0,{&value,1}) && bounded.Transfer(bounded_id,0,1,1) &&
          bounded.ObserveRenderTick(bounded_id,1,true), "bounded first render endpoint");
  value = World(1);
  Require(bounded.Publish(bounded_id,0,{&value,1}) && bounded.Transfer(bounded_id,0,1,1) &&
          bounded.ObserveRenderTick(bounded_id,2,true), "bounded second render endpoint");
  auto endpoint = bounded.Read(bounded_id,1);
  auto pinned = bounded.ReadRender(endpoint,{2,2,.5f,true});
  Require(pinned && bounded.Stats().bytes == NativeInstanceRegistry::kEntryBytes + 3*pose_bytes,
          "history plus rendered result fits the exact shared byte cap");
  Require(!bounded.ReadRender(endpoint,{2,3,.75f,true}) && pinned->transforms[0][12] == .5f,
          "pinned frame overlap refuses instead of exceeding budget or mutating a GPU lease");
  pinned.reset();
  auto recovered = bounded.ReadRender(endpoint,{2,3,.75f,true});
  Require(recovered && recovered->transforms[0][12] == .75f, "render budget recovers after actual reader release");
  bounded.Retire(bounded_id); endpoint.reset(); recovered.reset();
  Require(bounded.Stats().bytes == 0, "bounded render history and outputs release once");
}
}
void TestNativeInstances() {
  TestSourceHandoff();
  TestRenderPoses();
  NativeInstanceRegistry registry;
  Require(!registry.Create(0), "instance needs a published native model generation");
  const auto first = registry.Create(100), second = registry.Create(100);
  Require(first && second && first != second, "shared model has distinct native instances");
  std::vector<RenderMatrix> source{World(2), World(4)};
  Require(registry.Publish(first, 0, source), "initial pose");
  auto pose = registry.Read(first, 0);
  Require(pose && pose->instance == first && pose->model_generation == 100, "native identity");
  Require(registry.Publish(first, 0, source) && registry.Read(first, 0) == pose,
          "unchanged pose reuses immutable storage");
  source[0] = World(9);
  Require(registry.Publish(first, 1, source), "separate update lane");
  source.clear(); source.shrink_to_fit();
  Require(pose->transforms[0][12] == 2 && registry.Read(first, 1)->transforms[0][12] == 9,
          "source-free consumers and independent thread lanes");
  Require(!registry.Read(second, 0), "model sharing cannot share mutable instance pose");
  Require(!registry.Publish(first, 2, {&pose->transforms[0], 1}), "lane bound");
  auto invalid = World(0); invalid[3] = std::numeric_limits<float>::infinity();
  Require(!registry.Publish(first, 0, {&invalid, 1}) && !registry.Read(first, 0),
          "invalid update cannot leave a stale current pose");
  Require(pose->transforms[0][12] == 2, "invalid publication cannot damage existing lease");
  registry.Retire(first); registry.Retire(second);
  Require(!registry.Read(first, 1) && registry.Stats().bytes > 0, "retired lease stays charged");
  pose.reset();
  Require(registry.Stats().bytes == 0, "retired last reader releases pose storage");
  const auto replacement = registry.Create(101);
  Require(replacement > second && !registry.Read(first, 0), "retired IDs never alias a reload");
  registry.Retire(replacement);

  NativeInstanceRegistry count_limit(4096, 1);
  const auto only = count_limit.Create(1);
  Require(only && !count_limit.Create(1), "instance count backpressure");
  count_limit.Retire(only);
  Require(count_limit.Create(1) > only, "slot reuse has a fresh ID");
  NativeInstanceRegistry tiny(NativeInstanceRegistry::kEntryBytes - 1);
  Require(!tiny.Create(1), "bookkeeping is part of byte budget");

  NativeInstanceRegistry size_probe;
  const auto probe = size_probe.Create(1);
  auto value = World(1);
  Require(size_probe.Publish(probe, 0, {&value, 1}), "budget size probe");
  NativeInstanceRegistry bounded(size_probe.Stats().bytes);
  const auto id = bounded.Create(1);
  Require(bounded.Publish(id, 0, {&value, 1}), "exact bounded publication");
  auto pinned = bounded.Read(id, 0);
  value = World(3);
  Require(!bounded.Publish(id, 0, {&value, 1}) && !bounded.Read(id, 0),
          "pinned replacement overlap cannot exceed budget or retain stale current pose");
  pinned.reset();
  Require(bounded.Publish(id, 0, {&value, 1}), "backpressure recovers after reader releases");
  bounded.Invalidate(id, 0);
  Require(!bounded.Read(id, 0), "producer invalidation removes publication");
  const std::vector<RenderMatrix> oversized(NativeInstanceRegistry::kMaxTransforms + 1, World(0));
  Require(!bounded.Publish(id, 0, oversized), "transform count bound before copying");

  std::shared_ptr<const NativeInstancePose> surviving;
  {
    NativeInstanceRegistry temporary;
    const auto t = temporary.Create(7);
    Require(temporary.Publish(t, 0, {&value, 1}), "temporary owner publication");
    surviving = temporary.Read(t, 0);
  }
  Require(surviving->model_generation == 7 && surviving->transforms[0][12] == 3,
          "pose lease outlives registry");
  surviving.reset();

  const auto concurrent = registry.Create(2);
  Require(registry.Publish(concurrent, 0, {&value, 1}), "concurrent setup");
  std::barrier rendezvous(2);
  bool preserved = false;
  std::thread reader([&] {
    const auto old = registry.Read(concurrent, 0);
    rendezvous.arrive_and_wait(); rendezvous.arrive_and_wait();
    preserved = old->transforms[0][12] == 3 &&
        registry.Read(concurrent, 0)->transforms[0][12] == 8;
  });
  rendezvous.arrive_and_wait();
  value = World(8);
  const bool updated = registry.Publish(concurrent, 0, {&value, 1});
  rendezvous.arrive_and_wait(); reader.join();
  Require(updated && preserved, "render lease and producer update remain independent");
  registry.Retire(concurrent);
  Require(registry.Stats().bytes == 0, "all native instance ownership retired");
  std::cout << "native instance identity, lanes, source-free poses, reload and backpressure passed\n";
}
