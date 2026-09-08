/**
 * @brief Production shader programs and descriptor schema for rigid scene/casters.
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
  explicit NativeRigidDescriptorSchema(bool skin = false) {
    sets[0].begin();
    sets[0].addStructuredBuffer(0); // NativeRigidInstanceGPU[], indexed by SV_InstanceID
    if (skin) {
      sets[0].addStructuredBuffer(1); // Row-vector joint-to-world matrices
      sets[0].addStructuredBuffer(2); // Per-instance {first joint,count,reserved,reserved}
    }
    sets[0].end();
    // Explicit TEXTURE_2D_ARRAY sampled views: three albedo layers and mono sun
    // depth, both using layer zero. Do not bind a texture's default 2D view.
    sets[1].begin(); sets[2].begin();
    for (uint32_t layer = 0; layer < 4; ++layer) {
      sets[1].addTexture(layer); sets[2].addSampler(layer);
    }
    sets[1].end(); sets[2].end();
  }
  NativeRigidDescriptorSchema(const NativeRigidDescriptorSchema &) = delete;
  NativeRigidDescriptorSchema &operator=(const NativeRigidDescriptorSchema &) = delete;
};
struct NativeRigidPrograms { NativePipelineHandle scene, shadow, shadow_cutout; };
// Caller retains/reuses the returned immutable programs per compatible vertex input;
// this function does not accumulate a global cache or read any source resources.
NativeRigidPrograms CreateNativeRigidPrograms(plume::RenderDevice &device,
                                              NativeVertexInputHandle input);
NativePipelineHandle CreateNativeSkinShadowProgram(plume::RenderDevice &device, NativeVertexInputHandle input,
                                                  bool cutout = false);

inline NativeVertexInputHandle NativeSkinShadowVertexInput(const NativeMeshData &mesh,
                                                          NativeVertexInputLibrary &library, bool cutout = false) {
  const auto influences = NativeMeshSkinInfluences(mesh.attributes);
  if (!influences || !ValidateNativeMesh(mesh) || mesh.streams[0].stride > 255) return {};
  std::array<plume::RenderInputElement,6> elements{};
  const uint32_t count = cutout ? 6 : 5;
  for (uint32_t n = 0; n < count; ++n) {
    const auto semantic = n < 3 ? MeshSemantic::SkinPosition : n == 3 ? MeshSemantic::SkinJoints :
        n == 4 ? MeshSemantic::SkinWeights : MeshSemantic::TexCoord;
    // Inactive lanes have zero weights. Reuse a valid position fetch rather
    // than advertise an unbound input or carry a console missing-stream rule.
    const uint32_t index = n < influences ? n : 0;
    const auto a = std::find_if(mesh.attributes.begin(),mesh.attributes.end(),[&](const auto &a) {
      return a.semantic == semantic && a.index == (n < 3 ? index : 0);
    });
    if (a == mesh.attributes.end()) return {};
    elements[n] = {n < 3 ? "POSITION" : n == 3 ? "BLENDINDICES" : n == 4 ? "BLENDWEIGHT" : "TEXCOORD",
        n < 3 ? n : 0,n,plume::RenderFormat::R32G32B32A32_FLOAT,0,a->offset};
  }
  return library.Resolve(std::span(elements).first(count),1,{});
}

// Native shader locations come from the named asset schema, not the translated
// shader signature. Layer 2 additionally requires the asset's TexCoord2;
// unsupported/missing layouts remain ineligible, never inferred from a template.
inline NativeVertexInputHandle NativeRigidVertexInput(const NativeMeshData &mesh,
                                                     NativeVertexInputLibrary &library,
                                                     bool layered = false) {
  if (mesh.attributes.empty() || mesh.streams.size() != 1 || mesh.streams[0].slot != 0 ||
      !mesh.streams[0].stride || mesh.streams[0].stride > 255) return {};
  std::array<plume::RenderInputElement, 5> elements{};
  const MeshSemantic semantics[]{MeshSemantic::Position, MeshSemantic::Normal, MeshSemantic::TexCoord, MeshSemantic::Color, MeshSemantic::TexCoord};
  const char *names[]{"POSITION", "NORMAL", "TEXCOORD", "COLOR", "TEXCOORD"};
  const uint32_t count = layered ? 5 : 4;
  for (uint32_t n = 0; n < count; ++n) {
    bool found = false;
    const uint32_t index = n == 4 ? 2 : 0;
    for (const auto &attribute : mesh.attributes) if (attribute.semantic == semantics[n] && attribute.index == index) {
      if (found || attribute.offset > mesh.streams[0].stride || mesh.streams[0].stride - attribute.offset < 16) return {};
      found = true;
      elements[n].semanticName = names[n]; elements[n].location = n;
      elements[n].semanticIndex = index;
      elements[n].format = plume::RenderFormat::R32G32B32A32_FLOAT;
      elements[n].alignedByteOffset = attribute.offset;
    }
    if (!found) return {};
  }
  return library.Resolve(std::span(elements).first(count), 1, {});
}
} // namespace bd::gpu::scene
