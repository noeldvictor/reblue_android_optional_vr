/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_rigid_program.h"
#if defined(REBLUE_D3D12)
#include "src/gpu/shaders/hlsl/native_rigid_vs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_rigid_layered_vs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_rigid_ps.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_vs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_cutout_vs.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_cutout_ps.hlsl.dxil.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_alpha_ps.hlsl.dxil.h"
#define RIGID_BLOB(name) g_##name##_dxil, sizeof(g_##name##_dxil)
#else
#include "src/gpu/shaders/hlsl/native_rigid_vs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_rigid_layered_vs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_rigid_ps.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_vs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_cutout_vs.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_cutout_ps.hlsl.spirv.h"
#include "src/gpu/shaders/hlsl/native_rigid_shadow_alpha_ps.hlsl.spirv.h"
#define RIGID_BLOB(name) g_##name##_spirv, sizeof(g_##name##_spirv)
#endif
namespace bd::gpu::scene {
NativeRigidPrograms CreateNativeRigidPrograms(plume::RenderDevice &device, NativeVertexInputHandle input) {
  if (!input || (input->Elements().size() != 4 && input->Elements().size() != 5) || input->Streams() != 1) return {};
  const bool layered = input->Elements().size() == 5;
  const char *names[]{"POSITION", "NORMAL", "TEXCOORD", "COLOR", "TEXCOORD"};
  for (uint32_t n = 0; n < input->Elements().size(); ++n) {
    const auto &element = input->Elements()[n];
    if (element.location != n || element.slotIndex != 0 || element.semanticIndex != (n == 4 ? 2 : 0) ||
        element.format != plume::RenderFormat::R32G32B32A32_FLOAT ||
        std::strcmp(element.semanticName, names[n])) return {};
  }
  // The depth-only shader consumes position alone. Keep its owned layout exact,
  // without advertising unused normal/UV/colour inputs to the backend.
  NativeVertexInputLibrary shadow_inputs(2 * NativeVertexInputLibrary::kOwnerBytes, 2);
  auto shadow_input = shadow_inputs.Resolve(input->Elements().first(1), 1, {});
  const plume::RenderInputElement cutout_elements[]{input->Elements()[0], input->Elements()[2], input->Elements()[3]};
  auto cutout_input = shadow_inputs.Resolve(cutout_elements, 1, {});
  if (!shadow_input || !cutout_input) return {};
  NativeRigidDescriptorSchema schema;
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
  std::shared_ptr<plume::RenderShader> vertex = layered
      ? device.createShader(RIGID_BLOB(native_rigid_layered_vs), "main", format)
      : device.createShader(RIGID_BLOB(native_rigid_vs), "main", format);
  std::shared_ptr<plume::RenderShader> pixel = device.createShader(RIGID_BLOB(native_rigid_ps), "main", format);
  std::shared_ptr<plume::RenderShader> shadow = device.createShader(RIGID_BLOB(native_rigid_shadow_vs), "main", format);
  std::shared_ptr<plume::RenderShader> cutout_vertex = device.createShader(RIGID_BLOB(native_rigid_shadow_cutout_vs), "main", format);
  std::shared_ptr<plume::RenderShader> cutout_pixel = device.createShader(RIGID_BLOB(native_rigid_shadow_cutout_ps), "main", format);
  std::shared_ptr<plume::RenderShader> alpha_pixel = device.createShader(RIGID_BLOB(native_rigid_shadow_alpha_ps), "main", format);
  if (!layout || !vertex || !pixel || !shadow || !cutout_vertex || !cutout_pixel || !alpha_pixel) return {};
  NativeRigidPrograms result;
  result.scene = NativePipelineProgram::Create(layout, vertex, pixel, input);
  result.shadow = NativePipelineProgram::Create(layout, shadow, {}, std::move(shadow_input));
  result.shadow_alpha = NativePipelineProgram::Create(layout, cutout_vertex, alpha_pixel, cutout_input);
  result.shadow_cutout = NativePipelineProgram::Create(layout, cutout_vertex, cutout_pixel, std::move(cutout_input));
  return result;
}
} // namespace bd::gpu::scene
