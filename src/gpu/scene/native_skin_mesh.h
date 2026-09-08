/**
 * @brief Source-free joint-local deformation and indexed animated bounds.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_mesh_data.h"
#include "gpu/scene/native_transform.h"
#include <algorithm>
#include <bit>
#include <limits>

namespace bd::gpu::scene {
struct NativeSkinVertex {
  std::array<std::array<float,3>,3> positions{}, normals{};
  std::array<uint16_t,3> joints{};
  std::array<float,3> weights{};
};
struct NativeSkinnedVertex {
  std::array<float,3> world{}, normal{};
};

// Palette matrices already map each joint's local space into world space.
// Do not apply the object's world matrix again. Normal vectors retain the
// authored weighted linear transform; normalization belongs to the lit shader.
inline std::optional<NativeSkinnedVertex> DeformNativeSkinVertex(
    const NativeSkinVertex &vertex, std::span<const RenderMatrix> palette) {
  NativeSkinnedVertex result;
  float sum = 0;
  for (size_t n = 0; n < 3; ++n) {
    const float weight = vertex.weights[n];
    if (!std::isfinite(weight) || weight < 0 || weight > 1) return {};
    sum += weight;
    if (weight == 0) continue;
    if (vertex.joints[n] >= palette.size()) return {};
    const auto &matrix = palette[vertex.joints[n]];
    for (float value : matrix) if (!std::isfinite(value)) return {};
    if (matrix[3] != 0 || matrix[7] != 0 || matrix[11] != 0 || matrix[15] != 1) return {};
    const auto &p = vertex.positions[n], &normal = vertex.normals[n];
    for (size_t axis = 0; axis < 3; ++axis) {
      const float world = p[0]*matrix[axis] + p[1]*matrix[4+axis] +
          p[2]*matrix[8+axis] + matrix[12+axis];
      const float transformed_normal = normal[0]*matrix[axis] + normal[1]*matrix[4+axis] +
          normal[2]*matrix[8+axis];
      result.world[axis] += world*weight;
      result.normal[axis] += transformed_normal*weight;
    }
  }
  if (std::abs(sum-1.f) > 1e-5f) return {};
  for (float value : result.world) if (!std::isfinite(value)) return {};
  for (float value : result.normal) if (!std::isfinite(value)) return {};
  return result;
}

// Reference/culling producer from persistent native data and an owned pose.
// Only indexed vertices contribute, with the same signed base vertex as draws.
// No rigid bind-pose bound or guessed radius may cull an animated skin.
inline std::optional<NativeBounds> BuildNativeSkinBounds(
    const NativeMeshData &mesh, std::span<const RenderMatrix> palette) {
  if (!ValidateNativeMesh(mesh) || !NativeMeshSkinInfluences(mesh.attributes)) return {};
  NativeBounds bounds;
  bounds.min.fill(std::numeric_limits<float>::infinity());
  bounds.max.fill(-std::numeric_limits<float>::infinity());
  const auto &stream = mesh.streams[0];
  for (uint32_t index : mesh.indices) {
    const size_t offset = size_t(int64_t(index)+mesh.base_vertex)*stream.stride;
    NativeSkinVertex vertex;
    for (const auto &a : mesh.attributes) {
      if (a.semantic < MeshSemantic::SkinPosition) continue;
      for (size_t lane = 0; lane < 3; ++lane) {
        uint32_t bits = 0;
        for (size_t b = 0; b < 4; ++b)
          bits |= uint32_t(stream.bytes[offset+a.offset+lane*4+b]) << (8*b);
        const float value = std::bit_cast<float>(bits);
        switch (a.semantic) {
        case MeshSemantic::SkinPosition: vertex.positions[a.index][lane] = value; break;
        case MeshSemantic::SkinNormal: vertex.normals[a.index][lane] = value; break;
        case MeshSemantic::SkinJoints: vertex.joints[lane] = uint16_t(value); break;
        case MeshSemantic::SkinWeights: vertex.weights[lane] = value; break;
        default: return {};
        }
      }
    }
    const auto transformed = DeformNativeSkinVertex(vertex,palette);
    if (!transformed) return {};
    for (size_t axis = 0; axis < 3; ++axis) {
      bounds.min[axis] = (std::min)(bounds.min[axis],transformed->world[axis]);
      bounds.max[axis] = (std::max)(bounds.max[axis],transformed->world[axis]);
    }
  }
  return bounds.Valid() ? std::optional(bounds) : std::nullopt;
}
} // namespace bd::gpu::scene
