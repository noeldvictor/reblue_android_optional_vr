/**
 * @brief Production shader programs and descriptor schema for opaque rigid objects.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/pipeline/native_pipeline_program.h"
#include "gpu/scene/native_mesh_data.h"
#include <plume_render_interface_builders.h>

namespace bd::gpu::scene {
struct NativeRigidDescriptorSchema {
  std::array<plume::RenderDescriptorSetBuilder, 3> sets;
  NativeRigidDescriptorSchema() {
    sets[0].begin();
    sets[0].addConstantBufferDynamic(0); // NativeRigidObjectGPU
    sets[0].addConstantBufferDynamic(1); // NativeRigidPassGPU
    sets[0].end();
    sets[1].begin(); sets[1].addTexture(0); sets[1].addTexture(1); sets[1].end();
    sets[2].begin(); sets[2].addSampler(0); sets[2].addSampler(1); sets[2].end();
  }
  NativeRigidDescriptorSchema(const NativeRigidDescriptorSchema &) = delete;
  NativeRigidDescriptorSchema &operator=(const NativeRigidDescriptorSchema &) = delete;
};
struct NativeRigidPrograms { NativePipelineHandle scene, shadow; };
// Caller retains/reuses the returned immutable pair per compatible vertex input;
// this function does not accumulate a global cache or read any source resources.
NativeRigidPrograms CreateNativeRigidPrograms(plume::RenderDevice &device,
                                              NativeVertexInputHandle input);

// Native shader locations come from the named asset schema, not the translated
// shader signature. This first family requires all four values explicitly;
// unsupported/missing layouts remain ineligible, never inferred from a template.
inline NativeVertexInputHandle NativeRigidVertexInput(const NativeMeshData &mesh,
                                                     NativeVertexInputLibrary &library) {
  if (mesh.attributes.empty() || mesh.streams.size() != 1 || mesh.streams[0].slot != 0 ||
      !mesh.streams[0].stride || mesh.streams[0].stride > 255) return {};
  std::array<plume::RenderInputElement, 4> elements{};
  const MeshSemantic semantics[]{MeshSemantic::Position, MeshSemantic::Normal, MeshSemantic::TexCoord, MeshSemantic::Color};
  const char *names[]{"POSITION", "NORMAL", "TEXCOORD", "COLOR"};
  for (uint32_t n = 0; n < 4; ++n) {
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
} // namespace bd::gpu::scene
