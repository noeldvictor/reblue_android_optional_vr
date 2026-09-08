/**
 * @brief Native water shader and explicit sampled-image/mesh interface.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/pipeline/native_pipeline_program.h"
#include "gpu/scene/native_mesh_data.h"
#include <plume_render_interface_builders.h>

namespace bd::gpu::scene {
struct NativeWaterDescriptorSchema {
  std::array<plume::RenderDescriptorSetBuilder, 3> sets;
  NativeWaterDescriptorSchema() {
    sets[0].begin(); sets[0].addStructuredBuffer(0); sets[0].end();
    sets[1].begin(); sets[2].begin();
    // Bump, planar reflection, pre-water snapshot, bottom depth: 2D arrays;
    // environment: cube; sun: depth array. Even mono 2D inputs use array views.
    for (uint32_t n = 0; n < 6; ++n) { sets[1].addTexture(n); sets[2].addSampler(n); }
    sets[1].end(); sets[2].end();
  }
  NativeWaterDescriptorSchema(const NativeWaterDescriptorSchema &) = delete;
  NativeWaterDescriptorSchema &operator=(const NativeWaterDescriptorSchema &) = delete;
};
inline NativeVertexInputHandle NativeWaterVertexInput(const NativeMeshData &mesh, NativeVertexInputLibrary &library) {
  if (mesh.streams.size() != 1 || mesh.streams[0].slot != 0 ||
      !mesh.streams[0].stride || mesh.streams[0].stride > 255) return {};
  constexpr MeshSemantic semantics[]{MeshSemantic::Position, MeshSemantic::Normal, MeshSemantic::TexCoord,
                                    MeshSemantic::Color, MeshSemantic::Tangent};
  const char *names[]{"POSITION", "NORMAL", "TEXCOORD", "COLOR", "TANGENT"};
  std::array<plume::RenderInputElement, 5> elements{};
  for (uint32_t n = 0; n < elements.size(); ++n) {
    bool found = false;
    for (const auto &attribute : mesh.attributes) if (attribute.semantic == semantics[n] && attribute.index == 0) {
      if (found || attribute.offset > mesh.streams[0].stride || mesh.streams[0].stride - attribute.offset < 16) return {};
      found = true;
      elements[n].semanticName = names[n]; elements[n].location = n;
      elements[n].format = plume::RenderFormat::R32G32B32A32_FLOAT;
      elements[n].alignedByteOffset = attribute.offset;
    }
    if (!found) return {};
  }
  return library.Resolve(elements, 1, {});
}
NativePipelineHandle CreateNativeWaterProgram(plume::RenderDevice &device, NativeVertexInputHandle input);
} // namespace bd::gpu::scene
