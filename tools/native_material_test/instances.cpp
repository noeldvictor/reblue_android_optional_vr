#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_instance_source.h"
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
}
void TestNativeInstances() {
  TestSourceHandoff();
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
