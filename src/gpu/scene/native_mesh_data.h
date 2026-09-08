/**
 * @file    native_mesh_data.h
 * @brief   Portable native mesh payloads, independent of guest memory and Vulkan.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license   BSD 3-Clause License
 */
#pragma once
#include "gpu/scene/native_bounds.h"

#include <cstdint>
#include <span>
#include <vector>

namespace bd::gpu::scene {

enum class MeshTopology { Triangles, Strip };

// The import boundary accepts big-endian indices; everything after it is a
// native triangle list. Restart resets parity; degenerate strips advance it.
bool ImportMeshIndices(std::span<const uint8_t> source, bool index32,
                       MeshTopology topology, std::vector<uint32_t> &triangles);

struct NativeMeshStream {
  uint32_t slot = 0;
  uint32_t stride = 0;
  std::vector<uint8_t> bytes;
};

// BDMESH v2 rigid vertices: named float4 attributes in one interleaved stream.
// Values and all file metadata are little-endian. These are asset semantics,
// not shader locations, console declaration types or decoder masks.
enum class MeshSemantic : uint32_t {
  Position = 1, Normal = 2, Tangent = 3, Binormal = 4, TexCoord = 5, Color = 6,
  // v3: joint-local xyz (w=0), paired at indices 0..2. Joints/Weights are
  // float4 for portable vertex fetch: exact model-local uint16 IDs, normalized
  // nonnegative weights, unused lanes zero. No encoded register offsets.
  SkinPosition = 7, SkinNormal = 8, SkinJoints = 9, SkinWeights = 10
};
struct NativeMeshAttribute {
  MeshSemantic semantic = MeshSemantic::Position;
  uint32_t index = 0, offset = 0;
  bool operator==(const NativeMeshAttribute &) const = default;
};
uint64_t NativeMeshLayoutId(std::span<const NativeMeshAttribute> attributes);
uint32_t NativeMeshSkinInfluences(std::span<const NativeMeshAttribute> attributes);

struct NativeMeshData {
  // Empty attributes identify the transitional v1 packed payload. Nonempty
  // attributes are self-describing v2 (rigid) or v3 (skin); layout derives from
  // that schema. Skin assets deliberately have no rigid Position semantic.
  uint64_t layout = 0;
  int32_t base_vertex = 0;
  std::vector<uint32_t> indices;
  std::vector<NativeMeshStream> streams;
  std::vector<NativeMeshAttribute> attributes;
};

constexpr uint64_t kNativeMeshMaxBytes = 64ull << 20;
bool ValidateNativeMesh(const NativeMeshData &mesh);
// Derive once from checked v2 indexed positions, including signed base vertex.
// Reuses persisted bytes/identity; v1 has no native position contract and refuses.
std::optional<NativeBounds> BuildNativeMeshBounds(const NativeMeshData &mesh);
// Indexed COLOR0 red magnitude used by native water displacement. No assumed
// [0,1] colour range and no unused vertex may enlarge/shrink this mesh's bound.
std::optional<float> BuildNativeMeshWaveWeight(const NativeMeshData &mesh);
// Stable identity of a valid self-describing payload; zero rejects v1/invalid.
uint64_t NativeMeshContentId(const NativeMeshData &mesh);
bool EncodeNativeMesh(const NativeMeshData &mesh, std::vector<uint8_t> &file);
bool DecodeNativeMesh(std::span<const uint8_t> file, NativeMeshData &mesh);

} // namespace bd::gpu::scene
