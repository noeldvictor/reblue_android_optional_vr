/**
 * @brief Queue-to-immediate handoff using real Plume view representations.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "gpu/draw_geometry_bindings.h"
#include <array>
#include <cassert>
#include <vector>

namespace {
using namespace plume;
struct Commands {
  const RenderPipeline *pipeline = nullptr;
  std::array<RenderVertexBufferView,16> views{};
  std::array<RenderInputSlot,16> slots{};
  RenderIndexBufferView index;
  std::vector<std::pair<uint32_t,uint32_t>> runs;
  uint32_t pipeline_calls = 0, index_calls = 0;
  void setPipeline(const RenderPipeline *value) { pipeline = value; ++pipeline_calls; }
  void setVertexBuffers(uint32_t first, const RenderVertexBufferView *values,
                        uint32_t count, const RenderInputSlot *inputs) {
    assert(count && first + count <= 16);
    runs.emplace_back(first,count);
    for (uint32_t i=0;i<count;++i) {
      assert(values[i].buffer.ref);
      views[first+i] = values[i]; slots[first+i] = inputs[i];
    }
  }
  void setIndexBuffer(const RenderIndexBufferView *value) { index = *value; ++index_calls; }
};
}

void TestImmediateGeometryBindings() {
  using namespace bd::gpu;
  // Opaque identities only; the recorder never dereferences backend objects.
  const auto *engine = reinterpret_cast<const RenderPipeline *>(uintptr_t(1));
  const auto *native = reinterpret_cast<const RenderPipeline *>(uintptr_t(2));
  auto *buffer = reinterpret_cast<RenderBuffer *>(uintptr_t(3));
  auto *foreign = reinterpret_cast<RenderBuffer *>(uintptr_t(4));
  std::array<RenderVertexBufferView,14> views{};
  std::array<RenderInputSlot,14> slots{};
  for (uint32_t i : {0u,1u,3u,13u}) {
    views[i] = RenderVertexBufferView({buffer,64u+i*32u},128);
    slots[i] = RenderInputSlot(i+2,16u+i*4u);
  }
  const RenderIndexBufferView index({buffer,32},12,RenderFormat::R16_UINT);
  Commands commands;
  const auto apply = [&] { return ApplyImmediateGeometryBindings(commands,engine,2,views,slots,&index); };
  assert(apply());
  // Queue emission changed physical state; the logical producer above has not
  // changed at all. In particular slot15 no longer belongs to a pulled dummy.
  commands.setPipeline(native);
  for (uint32_t i=0;i<16;++i) {
    commands.views[i] = RenderVertexBufferView({foreign,0},64);
    commands.slots[i] = RenderInputSlot(i,0);
  }
  const RenderIndexBufferView foreign_index({foreign,0},24,RenderFormat::R32_UINT);
  commands.setIndexBuffer(&foreign_index);
  commands.runs.clear();
  assert(apply());
  assert(commands.pipeline == engine && commands.pipeline_calls == 3);
  assert((commands.runs == std::vector<std::pair<uint32_t,uint32_t>>{{2,2},{5,1},{15,1}}));
  for (uint32_t i : {0u,1u,3u,13u}) {
    const auto &actual = commands.views[i+2];
    assert(actual.buffer.ref == buffer && actual.buffer.offset == views[i].buffer.offset && actual.size == 128);
    assert(commands.slots[i+2].stride == slots[i].stride);
  }
  assert(commands.views[0].buffer.ref == foreign && commands.views[4].buffer.ref == foreign);
  assert(commands.index.buffer.ref == buffer && commands.index.buffer.offset == 32);
  assert(commands.index.size == 12 && commands.index.format == RenderFormat::R16_UINT);
  assert(commands.index_calls == 3);

  // Range/null-pipeline refusal must not partially bind anything.
  const auto before = commands.pipeline_calls;
  assert(!ApplyImmediateGeometryBindings(commands,nullptr,2,views,slots,&index));
  assert(!ApplyImmediateGeometryBindings(commands,engine,3,views,slots,&index));
  assert(!ApplyImmediateGeometryBindings(commands,engine,17,{},{},&index));
  assert(!ApplyImmediateGeometryBindings(commands,engine,2,views,{},&index));
  assert(commands.pipeline_calls == before && commands.index_calls == 3 && commands.runs.size() == 3);
  // Vertex-ID/nonindexed draws need no buffer bind; no null dereference/unbind.
  const RenderIndexBufferView empty;
  assert(ApplyImmediateGeometryBindings(commands,engine,16,{},{},&empty));
  assert(ApplyImmediateGeometryBindings(commands,engine,0,{},{},nullptr));
  assert(commands.index_calls == 3 && commands.runs.size() == 3);
}
