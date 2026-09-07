/**
 * @brief Native pipeline selection and ownership without a device or source data.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "gpu/pipeline/native_pipeline_program.h"
#include <cassert>

namespace {
using namespace bd::gpu;
struct Shader final : plume::RenderShader {
  void setName(const std::string &) override {}
};
struct State {
  const NativePipelineProgram *native_program = nullptr;
  const void *vertexShader = nullptr, *pixelShader = nullptr, *vertexDeclaration = nullptr;
  const void *native_vertex_input = nullptr;
  uint32_t specConstants = 0, colorWriteEnable = 15;
  bool instancing = false, occlusionCounting = false;
  plume::RenderFormat renderTargetFormat = plume::RenderFormat::R8G8B8A8_UNORM;
};
}

void TestNativePipelineProgram() {
  using namespace bd::gpu;
  auto layout = std::make_shared<plume::RenderPipelineLayout>();
  auto vertex = std::make_shared<Shader>(), pixel = std::make_shared<Shader>();
  std::weak_ptr<plume::RenderPipelineLayout> layout_alive = layout;
  std::weak_ptr<Shader> vertex_alive = vertex, pixel_alive = pixel;
  scene::NativeVertexInputHandle input;
  {
    scene::NativeVertexInputLibrary inputs;
    char semantic[] = "POSITION";
    plume::RenderInputElement element{};
    element.semanticName = semantic;
    element.format = plume::RenderFormat::R32G32B32A32_FLOAT;
    input = inputs.Resolve({&element, 1}, 1, {});
    assert(input);
  } // input library and source name have retired before program creation/use
  std::weak_ptr<const scene::NativeVertexInput> input_alive = input;
  std::array<plume::RenderSpecConstant, 3> specs{{{7, 42}, {0, 0}, {99, 3}}};
  auto program = NativePipelineProgram::Create(layout, vertex, pixel, input, specs);
  assert(program);
  specs[0] = {123, 999}; // caller's specialization array is not borrowed
  assert(!NativePipelineProgram::Create({}, vertex, pixel, input));
  assert(!NativePipelineProgram::Create(layout, {}, pixel, input));
  assert(!NativePipelineProgram::Create(layout, vertex, pixel, {}));
  std::array<plume::RenderSpecConstant, 9> oversized{};
  assert(!NativePipelineProgram::Create(layout, vertex, pixel, input, oversized));
  std::array<plume::RenderSpecConstant, 2> repeated{{{7, 1}, {7, 2}}};
  assert(!NativePipelineProgram::Create(layout, vertex, pixel, input, repeated));
  auto depth = NativePipelineProgram::Create(layout, vertex, {}, input);
  assert(depth);

  State state{program.get()};
  assert(NativePipelineStateValid(state));
  auto mixed = state;
  mixed.vertexShader = &state; assert(!NativePipelineStateValid(mixed));
  mixed = state; mixed.pixelShader = &state; assert(!NativePipelineStateValid(mixed));
  mixed = state; mixed.vertexDeclaration = &state; assert(!NativePipelineStateValid(mixed));
  mixed = state; mixed.native_vertex_input = input.get(); assert(!NativePipelineStateValid(mixed));
  mixed = state; mixed.specConstants = 1; assert(!NativePipelineStateValid(mixed));
  mixed = state; mixed.instancing = true; assert(!NativePipelineStateValid(mixed));
  mixed = state; mixed.occlusionCounting = true; assert(!NativePipelineStateValid(mixed));
  mixed.native_program = nullptr; assert(NativePipelineStateValid(mixed)); // legacy path is unchanged

  State shadow{depth.get()};
  assert(!NativePipelineStateValid(shadow)); // missing PS cannot silently omit colour
  shadow.renderTargetFormat = plume::RenderFormat::UNKNOWN;
  assert(NativePipelineStateValid(shadow));
  shadow.renderTargetFormat = state.renderTargetFormat; shadow.colorWriteEnable = 0;
  assert(NativePipelineStateValid(shadow));
  plume::RenderGraphicsPipelineDesc depth_desc;
  depth_desc.pixelShader = pixel.get(); // no accidental inherited PS
  ApplyNativePipelineProgram(*depth, depth_desc);
  assert(!depth_desc.pixelShader && !depth_desc.specConstants && !depth_desc.specConstantsCount);
  depth.reset();

  // Same lease operation as the async work item and cache entry: loader teardown
  // cannot invalidate queued work, nor may a completed job unpin a cached PSO.
  auto pending = state.native_program->Lease();
  program.reset(); layout.reset(); vertex.reset(); pixel.reset(); input.reset();
  assert(!layout_alive.expired() && !vertex_alive.expired() && !pixel_alive.expired() && !input_alive.expired());
  plume::RenderGraphicsPipelineDesc desc;
  ApplyNativePipelineProgram(*pending, desc);
  assert(desc.pipelineLayout == pending->Layout() && desc.vertexShader == pending->Vertex());
  assert(desc.pixelShader == pending->Pixel() && desc.inputElementsCount == 1);
  assert(std::string(desc.inputElements[0].semanticName) == "POSITION");
  assert(desc.specConstantsCount == 3 && desc.specConstants[0].index == 7 && desc.specConstants[0].value == 42);
  assert(desc.specConstants[1].index == 0 && desc.specConstants[1].value == 0); // explicit zero is retained
  auto cached = pending;
  pending.reset();
  assert(!layout_alive.expired() && cached->Vertex());
  cached.reset();
  assert(layout_alive.expired() && vertex_alive.expired() && pixel_alive.expired() && input_alive.expired());
}
