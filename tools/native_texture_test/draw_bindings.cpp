/**
 * @brief Native binding emission, grouping and caller restoration fixtures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "gpu/draw_bindings.h"
#include "gpu/draw_bindings_bridge.h"
#include <atomic>
#include <cassert>
#include <memory>
#include <vector>

// Opaque backend objects; no Vulkan, SDK, device, file or source-memory input.
namespace plume { struct RenderPipelineLayout {}; struct RenderDescriptorSet {}; }
namespace {
using namespace bd::gpu;
struct Bind {
  const plume::RenderPipelineLayout *layout;
  plume::RenderDescriptorSet *set;
  uint32_t slot;
  std::vector<uint32_t> offsets;
};
struct Commands {
  const plume::RenderPipelineLayout *layout = nullptr;
  std::vector<Bind> binds;
  uint32_t layout_calls = 0;
  void setGraphicsPipelineLayout(const plume::RenderPipelineLayout *value) { layout = value; ++layout_calls; }
  void setGraphicsDescriptorSet(plume::RenderDescriptorSet *set, uint32_t slot) {
    assert(layout); binds.push_back({layout, set, slot, {}});
  }
  void setGraphicsDescriptorSetDynamic(plume::RenderDescriptorSet *set, uint32_t slot,
                                      const uint32_t *offsets, uint32_t count) {
    assert(layout); binds.push_back({layout, set, slot, {offsets, offsets + count}});
  }
};
}

void TestGraphicsBindings() {
  using namespace bd::gpu;
  plume::RenderPipelineLayout engine_layout, native_layout;
  plume::RenderDescriptorSet images, samplers, constants, native_constants;
  GraphicsBindings engine;
  engine.layout = &engine_layout; engine.set_count = 3;
  engine.sets = {&images, &samplers, &constants};
  engine.dynamic_counts[2] = 3;
  engine.offsets = {256, 512, 768};
  Commands commands;
  GraphicsBindingState state;
  assert(ApplyGraphicsBindings(commands, engine, state));
  assert(commands.layout_calls == 1 && commands.binds.size() == 3);
  assert(commands.binds.back().offsets == (std::vector<uint32_t>{256, 512, 768}));
  assert(ApplyGraphicsBindings(commands, engine, state) && commands.binds.size() == 3);

  auto shifted = engine;
  shifted.offsets[0] = 1024;
  assert(!engine.Matches(shifted) && engine.Matches(shifted, 0));
  assert(engine.Key() != shifted.Key() && engine.Key(0) == shifted.Key(0));
  assert(ApplyGraphicsBindings(commands, shifted, state));
  assert(commands.binds.size() == 4 && commands.binds.back().offsets[0] == 1024);
  // Every non-instance binding must break batching, even for identical offsets.
  for (uint32_t set = 0; set < engine.set_count; ++set) {
    auto other = engine; other.sets[set] = &native_constants;
    assert(!engine.Matches(other, 0) && engine.Key(0) != other.Key(0));
  }
  for (uint32_t offset = 1; offset < 3; ++offset) {
    auto other = engine; ++other.offsets[offset];
    assert(!engine.Matches(other, 0) && engine.Key(0) != other.Key(0));
  }
  auto another_layout = engine; another_layout.layout = &native_layout;
  assert(!engine.Matches(another_layout, 0) && engine.Key(0) != another_layout.Key(0));
  assert(ApplyGraphicsBindings(commands, another_layout, state));
  assert(commands.layout_calls == 2 && commands.binds.size() == 7);

  // Native shader: one uniform, no VS/PS/shared convention and no engine heaps.
  GraphicsBindings native;
  native.layout = &native_layout; native.set_count = 1;
  native.sets[0] = &native_constants; native.dynamic_counts[0] = 1; native.offsets[0] = 4096;
  assert(ApplyGraphicsBindings(commands, native, state));
  assert(commands.binds.size() == 8 && commands.binds.back().offsets == std::vector<uint32_t>{4096});
  assert(ApplyGraphicsBindings(commands, engine, state));
  assert(commands.layout_calls == 3 && commands.binds.size() == 11);
  assert(state.current.Matches(engine)); // exact caller offsets, not zeros

  // Two dynamic sets, including redistribution of the flattened offset array.
  auto split = engine; split.dynamic_counts = {1, 0, 2};
  assert(ApplyGraphicsBindings(commands, split, state));
  assert(commands.binds[11].slot == 0 && commands.binds[11].offsets == std::vector<uint32_t>{256});
  assert(commands.binds[12].slot == 2 && commands.binds[12].offsets == (std::vector<uint32_t>{512, 768}));
  assert(!split.Matches(engine) && split.Key() != engine.Key());
  auto ignored_tail = split;
  ignored_tail.offsets[7] = 999; ignored_tail.sets[3] = &native_constants; ignored_tail.dynamic_counts[3] = 7;
  assert(split.Matches(ignored_tail) && split.Key() == ignored_tail.Key());

  // Refusal is atomic: no layout/set side effects, no replacement of old state.
  auto expect_invalid = [&](GraphicsBindings bad) {
    const auto before = commands.binds.size();
    const auto layouts = commands.layout_calls;
    assert(!bad.Valid() && !bad.Matches(split) && bad.Key() == 0);
    assert(!ApplyGraphicsBindings(commands, bad, state));
    assert(commands.binds.size() == before && commands.layout_calls == layouts && state.current.Matches(split));
  };
  expect_invalid({});
  auto invalid = split; invalid.set_count = 5; expect_invalid(invalid);
  invalid = split; invalid.dynamic_counts[0] = 7; expect_invalid(invalid);
  invalid = split; invalid.sets[0] = nullptr; expect_invalid(invalid);

  GraphicsBindings no_descriptors; no_descriptors.layout = &native_layout;
  assert(ApplyGraphicsBindings(commands, no_descriptors, state));
  auto sparse = engine; sparse.sets[0] = nullptr;
  assert(ApplyGraphicsBindings(commands, sparse, state));
  assert(state.current.Matches(sparse));
  assert(ApplyGraphicsBindings(commands, engine, state)); // previously omitted set must bind again
  assert(commands.binds.back().slot == 0);

  struct Producer {
    std::unique_ptr<plume::RenderPipelineLayout> pipeline_layout = std::make_unique<plume::RenderPipelineLayout>();
    std::unique_ptr<plume::RenderDescriptorSet> texture_descriptor_set = std::make_unique<plume::RenderDescriptorSet>();
    std::unique_ptr<plume::RenderDescriptorSet> sampler_descriptor_set = std::make_unique<plume::RenderDescriptorSet>();
    std::unique_ptr<plume::RenderDescriptorSet> constant_descriptor_set = std::make_unique<plume::RenderDescriptorSet>();
    std::array<std::unique_ptr<plume::RenderDescriptorSet>, 2> occlusion_descriptor_set;
    uint32_t constant_dyn_offsets[3]{32, 64, 96};
    bool occlusion_counting = false;
    std::atomic<uint32_t> frame{1};
  } producer;
  producer.occlusion_descriptor_set[1] = std::make_unique<plume::RenderDescriptorSet>();
  const auto captured = EngineGraphicsBindings(producer);
  producer.constant_dyn_offsets[0] = 12345;
  producer.occlusion_counting = true;
  const auto occlusion = EngineGraphicsBindings(producer);
  assert(captured.offsets[0] == 32 && captured.set_count == 3);
  assert(occlusion.sets[3] == producer.occlusion_descriptor_set[1].get() && occlusion.set_count == 4);
  assert(!captured.Matches(occlusion, 0));
  assert(ApplyGraphicsBindings(commands, captured, state));
  assert(commands.binds.back().offsets[0] == 32); // consume the snapshot, not live producer state
}
