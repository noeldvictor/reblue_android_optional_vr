/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_water_program.h"
#if defined(REBLUE_D3D12)
#include "src/gpu/shaders/hlsl/native_water_vs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_water_ps.hlsl.dxil.h"
#define WATER_BLOB(name) g_##name##_dxil, sizeof(g_##name##_dxil)
#else
#include "src/gpu/shaders/hlsl/native_water_vs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_water_ps.hlsl.spirv.h"
#define WATER_BLOB(name) g_##name##_spirv, sizeof(g_##name##_spirv)
#endif
namespace bd::gpu::scene {
NativePipelineHandle CreateNativeWaterProgram(plume::RenderDevice &device, NativeVertexInputHandle input) {
  if (!input || input->Elements().size() != 5 || input->Streams() != 1) return {};
  const char *names[]{"POSITION", "NORMAL", "TEXCOORD", "COLOR", "TANGENT"};
  for (uint32_t n = 0; n < 5; ++n) {
    const auto &element = input->Elements()[n];
    if (element.location != n || element.slotIndex || element.semanticIndex ||
        element.format != plume::RenderFormat::R32G32B32A32_FLOAT || std::strcmp(element.semanticName, names[n])) return {};
  }
  NativeWaterDescriptorSchema schema;
  plume::RenderPipelineLayoutBuilder builder;
  builder.begin(false, true);
  for (const auto &set : schema.sets) builder.addDescriptorSet(set);
  builder.end();
  std::shared_ptr<plume::RenderPipelineLayout> layout = builder.create(&device);
#if defined(REBLUE_D3D12)
  constexpr auto format = plume::RenderShaderFormat::DXIL;
#else
  constexpr auto format = plume::RenderShaderFormat::SPIRV;
#endif
  std::shared_ptr<plume::RenderShader> vertex = device.createShader(WATER_BLOB(native_water_vs), "main", format);
  std::shared_ptr<plume::RenderShader> pixel = device.createShader(WATER_BLOB(native_water_ps), "main", format);
  if (!layout || !vertex || !pixel) return {};
  return NativePipelineProgram::Create(std::move(layout), std::move(vertex), std::move(pixel), std::move(input));
}
} // namespace bd::gpu::scene
