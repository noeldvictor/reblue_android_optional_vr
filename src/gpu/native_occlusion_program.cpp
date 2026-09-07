/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/native_occlusion_program.h"
#if defined(REBLUE_D3D12)
#include "src/gpu/shaders/hlsl/native_occ_proxy_vs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/occ_proxy_ps.hlsl.dxil.h"
#define OCC_BLOB(name) g_##name##_dxil, sizeof(g_##name##_dxil)
#else
#include "src/gpu/shaders/hlsl/native_occ_proxy_vs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/occ_proxy_ps.hlsl.spirv.h"
#define OCC_BLOB(name) g_##name##_spirv, sizeof(g_##name##_spirv)
#endif
namespace bd::gpu {
NativeOcclusionProgram CreateNativeOcclusionProgram(plume::RenderDevice &device) {
  plume::RenderPipelineLayoutBuilder builder;
  builder.begin(false, true);
  builder.addPushConstant(0, 0, sizeof(NativeOcclusionPacket), plume::RenderShaderStageFlag::VERTEX);
  builder.end();
#if defined(REBLUE_D3D12)
  constexpr auto format = plume::RenderShaderFormat::DXIL;
#else
  constexpr auto format = plume::RenderShaderFormat::SPIRV;
#endif
  return {builder.create(&device), device.createShader(OCC_BLOB(native_occ_proxy_vs), "main", format),
      device.createShader(OCC_BLOB(occ_proxy_ps), "main", format)};
}
std::unique_ptr<plume::RenderPipeline> CreateNativeOcclusionPipeline(
    plume::RenderDevice &device, const NativeOcclusionProgram &program, const NativeTargetShape &shape) {
  // Stereo needs explicit per-eye query inputs; never apply a mono test to both eyes.
  if (!program || shape.layers != 1 || !shape.Bytes(512ull << 20) ||
      shape.format != plume::RenderFormat::R16G16B16A16_FLOAT) return {};
  plume::RenderGraphicsPipelineDesc desc;
  desc.pipelineLayout = program.layout.get();
  desc.vertexShader = program.vertex.get(); desc.pixelShader = program.pixel.get();
  desc.depthFunction = plume::RenderComparisonFunction::LESS_EQUAL;
  desc.depthEnabled = true; desc.depthWriteEnabled = false;
  desc.primitiveTopology = plume::RenderPrimitiveTopology::TRIANGLE_LIST;
  desc.cullMode = plume::RenderCullMode::NONE;
  desc.renderTargetCount = 1; desc.renderTargetFormat[0] = shape.format;
  desc.renderTargetBlend[0] = plume::RenderBlendDesc::Copy();
  desc.renderTargetBlend[0].renderTargetWriteMask = 0;
  desc.depthTargetFormat = plume::RenderFormat::D32_FLOAT_S8_UINT;
  desc.multisampling.sampleCount = shape.samples;
  return device.createGraphicsPipeline(desc);
}
} // namespace bd::gpu
